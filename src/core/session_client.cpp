#include "session_client.h"

#include "cat_catalog.h"
#include "media_parser.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QUuid>

namespace ftremote {

namespace {
QString fingerprintFor(const QSslCertificate &certificate)
{
    return QString::fromLatin1(certificate.digest(QCryptographicHash::Sha256).toHex(':')).toUpper();
}
}

SessionClient::SessionClient(QObject *parent)
    : QObject(parent)
{
    m_heartbeatTimer.setInterval(1000);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &SessionClient::sendHeartbeat);
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &SessionClient::attemptReconnect);
    connect(&m_network, &QNetworkAccessManager::sslErrors, this, &SessionClient::onSslErrors);
    connect(&m_control, &QWebSocket::connected, this, &SessionClient::onControlConnected);
    connect(&m_control, &QWebSocket::textMessageReceived, this, &SessionClient::onControlText);
    connect(&m_control, &QWebSocket::disconnected, this, &SessionClient::onControlDisconnected);
    connect(&m_control, &QWebSocket::sslErrors, this, &SessionClient::onControlSslErrors);
    connect(&m_media, &QWebSocket::connected, this, &SessionClient::onMediaConnected);
    connect(&m_media, &QWebSocket::textMessageReceived, this, &SessionClient::onMediaText);
    connect(&m_media, &QWebSocket::binaryMessageReceived, this, &SessionClient::onMediaBinary);
    connect(&m_media, &QWebSocket::disconnected, this, &SessionClient::onMediaDisconnected);
    connect(&m_media, &QWebSocket::sslErrors, this, &SessionClient::onMediaSslErrors);
}

QString SessionClient::mode() const
{
    static const QStringList names{QStringLiteral("LSB"), QStringLiteral("USB"), QStringLiteral("CW"),
                                   QStringLiteral("FM"), QStringLiteral("AM"), QStringLiteral("DATA")};
    return (m_station.radioMode >= 0 && m_station.radioMode < names.size())
               ? names.at(m_station.radioMode)
               : QStringLiteral("Unknown");
}

QString SessionClient::origin() const
{
    QUrl url = m_baseUrl;
    url.setPath(QString());
    url.setQuery(QString());
    url.setFragment(QString());
    return url.toString(QUrl::FullyEncoded).toLower();
}

QUrl SessionClient::apiUrl(const QString &path) const
{
    QUrl url = m_baseUrl;
    url.setPath(path);
    return url;
}

QUrl SessionClient::webSocketUrl(const QString &path) const
{
    QUrl url = apiUrl(path);
    url.setScheme(url.scheme() == QStringLiteral("https") ? QStringLiteral("wss") : QStringLiteral("ws"));
    return url;
}

void SessionClient::setStatus(const QString &status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void SessionClient::login(const QString &server, const QString &username, const QString &password)
{
    if (m_loginReply)
        m_loginReply->abort();
    m_control.close();
    m_media.close();
    m_heartbeatTimer.stop();
    m_token.clear();
    m_leaseId.clear();
    m_username = username.trimmed();
    m_password = password;
    m_connected = false;
    m_transmitting = false;
    m_station.revision = -1;
    emit connectedChanged();
    emit transmittingChanged();

    m_baseUrl = QUrl::fromUserInput(server.trimmed());
    if (!m_baseUrl.isValid() || m_baseUrl.host().isEmpty() || m_username.isEmpty() ||
        (m_baseUrl.scheme() != QStringLiteral("https") &&
         !(m_baseUrl.scheme() == QStringLiteral("http") &&
           (m_baseUrl.host() == QStringLiteral("localhost") || m_baseUrl.host() == QStringLiteral("127.0.0.1"))))) {
        emit loginFailed(QStringLiteral("请输入有效的 HTTPS 服务器和用户名；仅允许本机 HTTP 开发地址"));
        return;
    }
    setStatus(QStringLiteral("Connecting"));

    QNetworkRequest request(apiUrl(QStringLiteral("/api/v1/auth/token")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    const QJsonObject body{{QStringLiteral("username"), m_username},
                           {QStringLiteral("password"), m_password},
                           {QStringLiteral("client_name"), QStringLiteral("ftremote")},
                           {QStringLiteral("client_version"), QStringLiteral("0.1.0")}};
    m_waitingForCertificate = false;
    m_loginReply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_loginReply, &QNetworkReply::finished, this, &SessionClient::onLoginFinished);
}

void SessionClient::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    if (!reply || errors.isEmpty())
        return;
    const QSslCertificate certificate = reply->sslConfiguration().peerCertificate();
    if (certificateMatchesPin(certificate)) {
        reply->ignoreSslErrors();
    } else if (reply == m_loginReply) {
        handleSslCertificate(certificate);
        m_waitingForCertificate = true;
    } else {
        reply->abort();
        emit loginFailed(QStringLiteral("TLS 证书未通过已固定的服务器指纹校验"));
    }
}

void SessionClient::handleSslCertificate(const QSslCertificate &certificate)
{
    if (certificate.isNull())
        return;
    const QString fingerprint = fingerprintFor(certificate);
    QSettings settings;
    const QString pin = settings.value(QStringLiteral("tls/pins/%1").arg(origin())).toString();
    if (!pin.isEmpty() && pin != fingerprint) {
        emit loginFailed(QStringLiteral("服务器证书指纹已变化，连接已阻止"));
        return;
    }
    if (pin.isEmpty()) {
        m_pendingFingerprint = fingerprint;
        emit certificateTrustRequired(origin(), fingerprint, certificate.subjectDisplayName(),
                                      certificate.issuerDisplayName());
    }
}

bool SessionClient::certificateMatchesPin(const QSslCertificate &certificate) const
{
    QSettings settings;
    return !certificate.isNull() &&
           settings.value(QStringLiteral("tls/pins/%1").arg(origin())).toString() == fingerprintFor(certificate);
}

void SessionClient::trustCertificate(const QString &fingerprint)
{
    if (fingerprint.isEmpty() || fingerprint != m_pendingFingerprint)
        return;
    QSettings settings;
    settings.setValue(QStringLiteral("tls/pins/%1").arg(origin()), fingerprint);
    m_pendingFingerprint.clear();
    m_waitingForCertificate = false;
    login(m_baseUrl.toString(), m_username, m_password);
}

void SessionClient::onLoginFinished()
{
    auto *reply = m_loginReply;
    m_loginReply = nullptr;
    if (!reply)
        return;
    const QByteArray body = reply->readAll();
    const bool waiting = m_waitingForCertificate;
    if (reply->error() != QNetworkReply::NoError) {
        if (!waiting)
            emit loginFailed(reply->errorString());
        reply->deleteLater();
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    const QString token = document.object().value(QStringLiteral("access_token")).toString();
    if (parseError.error != QJsonParseError::NoError || token.isEmpty()) {
        emit loginFailed(QStringLiteral("服务器返回的登录响应无效"));
        reply->deleteLater();
        return;
    }
    if (m_baseUrl.scheme() == QStringLiteral("https")) {
        const QSslCertificate certificate = reply->sslConfiguration().peerCertificate();
        if (!certificate.isNull() && !certificateMatchesPin(certificate)) {
            QSettings settings;
            const QString configuredPin = settings.value(QStringLiteral("tls/pins/%1").arg(origin())).toString();
            if (configuredPin.isEmpty()) {
                handleSslCertificate(certificate);
                m_waitingForCertificate = true;
                reply->deleteLater();
                return;
            }
            emit loginFailed(QStringLiteral("服务器证书指纹已变化，连接已阻止"));
            reply->deleteLater();
            return;
        }
    }
    m_token = token;
    reply->deleteLater();
    fetchCapabilities();
}

void SessionClient::fetchCapabilities()
{
    QNetworkRequest request(apiUrl(QStringLiteral("/api/v1/capabilities")));
    request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_token.toUtf8());
    auto *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit loginFailed(reply->errorString());
            reply->deleteLater();
            return;
        }
        const QJsonObject object = QJsonDocument::fromJson(body).object();
        m_capabilities.protocolVersion = object.value(QStringLiteral("protocol_version")).toInt();
        for (const auto &value : object.value(QStringLiteral("cat_commands")).toArray())
            m_capabilities.catCommands.append(value.toString());
        m_capabilities.media = object.value(QStringLiteral("media")).toObject();
        m_capabilities.hardware = object.value(QStringLiteral("hardware")).toObject();
        emit capabilitiesChanged(m_capabilities);
        reply->deleteLater();
        openControl();
    });
}

void SessionClient::openControl()
{
    setStatus(QStringLiteral("Opening control channel"));
    QNetworkRequest request(webSocketUrl(QStringLiteral("/api/v1/ws/control")));
    request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_token.toUtf8());
    m_control.open(request);
}

void SessionClient::onControlConnected()
{
    m_reconnectTimer.stop();
    m_reconnectAttempt = 0;
    setStatus(QStringLiteral("Control connected"));
}

void SessionClient::onControlText(const QString &message)
{
    QString error;
    const auto envelope = Envelope::fromJson(message.toUtf8(), &error);
    if (!envelope) {
        emit protocolError(QStringLiteral("invalid_message"), error, false);
        return;
    }
    if (envelope->type == QStringLiteral("server.hello")) {
        m_leaseId = envelope->payload.value(QStringLiteral("lease_id")).toString();
        m_heartbeatSeconds = qBound(1, envelope->payload.value(QStringLiteral("heartbeat_seconds")).toInt(10), 30);
        m_connected = true;
        m_idleHeartbeatTicks = 0;
        m_heartbeatTimer.start();
        emit connectedChanged();
        openMedia();
    } else if (envelope->type == QStringLiteral("station.snapshot")) {
        applyStation(envelope->payload.value(QStringLiteral("state")).toObject());
    } else if (envelope->type == QStringLiteral("station.patch")) {
        applyStation(envelope->payload.value(QStringLiteral("changes")).toObject());
    } else if (envelope->type == QStringLiteral("error")) {
        const auto payload = envelope->payload;
        emit protocolError(payload.value(QStringLiteral("code")).toString(),
                           payload.value(QStringLiteral("message")).toString(),
                           payload.value(QStringLiteral("retryable")).toBool());
    } else if (envelope->type == QStringLiteral("safety.prepared")) {
        m_pendingSafetyConfirmation = envelope->payload.value(QStringLiteral("confirmation_id")).toString();
        emit safetyPrepared(m_pendingSafetyConfirmation,
                            envelope->payload.value(QStringLiteral("expires_at")).toString(),
                            envelope->payload.value(QStringLiteral("action_digest")).toString());
    }
}

void SessionClient::applyStation(const QJsonObject &payload)
{
    const auto state = StationState::fromJson(payload);
    if (m_station.revision >= 0 && state.revision <= m_station.revision)
        return;
    m_station = state;
    if (m_transmitting != state.transmitting) {
        m_transmitting = state.transmitting;
        emit transmittingChanged();
    }
    emit stationChanged();
}

void SessionClient::openMedia()
{
    QNetworkRequest request(webSocketUrl(QStringLiteral("/api/v1/ws/media")));
    request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_token.toUtf8());
    m_media.open(request);
}

void SessionClient::onMediaConnected()
{
    Envelope attach;
    attach.type = QStringLiteral("media.attach");
    attach.requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    attach.payload = {{QStringLiteral("lease_id"), m_leaseId},
                      // PCM is the interoperable baseline; Opus can be enabled once the optional
                      // codec dependency is present in the deployment package.
                      {QStringLiteral("codecs"), QJsonArray{QStringLiteral("pcm_s16le")}}};
    m_media.sendTextMessage(QString::fromUtf8(QJsonDocument(attach.toJson()).toJson(QJsonDocument::Compact)));
}

void SessionClient::onMediaText(const QString &message)
{
    QString error;
    const auto envelope = Envelope::fromJson(message.toUtf8(), &error);
    if (!envelope) {
        emit protocolError(QStringLiteral("invalid_message"), error, true);
        return;
    }
    if (envelope->type == QStringLiteral("media.ready"))
        emit mediaReady(envelope->payload.value(QStringLiteral("codec")).toString());
    else if (envelope->type == QStringLiteral("error"))
        emit protocolError(envelope->payload.value(QStringLiteral("code")).toString(),
                           envelope->payload.value(QStringLiteral("message")).toString(),
                           envelope->payload.value(QStringLiteral("retryable")).toBool());
}

void SessionClient::onMediaBinary(const QByteArray &message)
{
    QString error;
    const auto frame = MediaParser::parseSpectrum(message, &error);
    if (frame) {
        const auto header = MediaParser::parseHeader(message);
        emit spectrumFrame(*frame, header->sequence, header->timestampUs);
    } else {
        const auto header = MediaParser::parseHeader(message, &error);
        if (header && header->kind == 2 && header->payloadLength >= 12) {
            const char *payload = message.constData() + MediaParser::HeaderSize;
            const quint8 codec = quint8(payload[0]);
            const quint16 samples = (quint16(quint8(payload[8])) << 8) | quint16(quint8(payload[9]));
            const qsizetype dataLength = message.size() - MediaParser::HeaderSize - 12;
            if (codec == 2 && samples == 960 && dataLength == 1920)
                emit rxPcm(QByteArray(payload + 12, int(dataLength)));
            else
                emit protocolError(QStringLiteral("unsupported_audio"), QStringLiteral("only PCM 48 kHz/20 ms is enabled"), true);
        } else if (!error.isEmpty()) {
            emit protocolError(QStringLiteral("invalid_media"), error, true);
        }
    }
}

void SessionClient::onControlDisconnected()
{
    m_heartbeatTimer.stop();
    m_media.close();
    if (m_transmitting) {
        m_transmitting = false;
        emit transmittingChanged();
    }
    m_connected = false;
    emit connectedChanged();
    if (m_control.closeCode() == 4409) {
        setStatus(QStringLiteral("Station is busy (another operator owns the lease)"));
        logout();
        return;
    }
    setStatus(QStringLiteral("Control disconnected; server will return to RX safely"));
    if (!m_token.isEmpty() && !m_reconnectTimer.isActive()) {
        m_reconnectAttempt = 0;
        m_reconnectTimer.start(500);
    }
}

void SessionClient::attemptReconnect()
{
    if (m_token.isEmpty() || m_connected)
        return;
    if (m_reconnectAttempt >= 4) {
        setStatus(QStringLiteral("Reconnect grace window expired"));
        logout();
        return;
    }
    ++m_reconnectAttempt;
    setStatus(QStringLiteral("Reconnecting (%1/4)").arg(m_reconnectAttempt));
    openControl();
    m_reconnectTimer.start(500 * (1 << m_reconnectAttempt));
}

void SessionClient::onMediaDisconnected()
{
    if (m_transmitting)
        releasePtt();
}

void SessionClient::sendEnvelope(const Envelope &envelope)
{
    if (m_control.state() != QAbstractSocket::ConnectedState)
        return;
    m_control.sendTextMessage(QString::fromUtf8(QJsonDocument(envelope.toJson()).toJson(QJsonDocument::Compact)));
}

void SessionClient::sendHeartbeat()
{
    if (!m_connected)
        return;
    ++m_idleHeartbeatTicks;
    if (m_transmitting || m_idleHeartbeatTicks >= m_heartbeatSeconds) {
        Envelope heartbeat;
        heartbeat.type = QStringLiteral("session.heartbeat");
        heartbeat.requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        sendEnvelope(heartbeat);
        m_idleHeartbeatTicks = 0;
    }
}

void SessionClient::setFrequency(qint64 frequencyHz)
{
    QJsonObject parameters{{QStringLiteral("frequency_hz"), frequencyHz}};
    sendCat(QStringLiteral("FA"), QStringLiteral("set"), parameters);
}

void SessionClient::sendCat(const QString &command, const QString &operation, const QJsonObject &parameters)
{
    QString error;
    const auto request = CatCatalog::makeRequest(command, operation, parameters,
                                                  QUuid::createUuid().toString(QUuid::WithoutBraces), &error);
    if (!request) {
        emit protocolError(QStringLiteral("invalid_request"), error, false);
        return;
    }
    sendEnvelope(*request);
}

void SessionClient::prepareSafety(const QString &command,
                                  const QString &operation,
                                  const QJsonObject &parameters)
{
    QString error;
    const auto request = CatCatalog::makeRequest(command, operation, parameters,
                                                  QUuid::createUuid().toString(QUuid::WithoutBraces), &error);
    if (!request) {
        emit protocolError(QStringLiteral("invalid_request"), error, false);
        return;
    }
    m_pendingSafetyRequest = request;
    m_pendingSafetyConfirmation.clear();
    Envelope prepare;
    prepare.type = QStringLiteral("safety.prepare");
    prepare.requestId = request->requestId;
    prepare.payload = {{QStringLiteral("request"), request->payload}};
    sendEnvelope(prepare);
}

void SessionClient::confirmSafety()
{
    if (!m_pendingSafetyRequest || m_pendingSafetyConfirmation.isEmpty())
        return;
    m_pendingSafetyRequest->payload.insert(QStringLiteral("confirmation_id"), m_pendingSafetyConfirmation);
    sendEnvelope(*m_pendingSafetyRequest);
    clearSafety();
}

void SessionClient::clearSafety()
{
    m_pendingSafetyRequest.reset();
    m_pendingSafetyConfirmation.clear();
}

void SessionClient::pressPtt()
{
    Envelope request;
    request.type = QStringLiteral("ptt.press");
    request.requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    sendEnvelope(request);
}

void SessionClient::releasePtt()
{
    Envelope request;
    request.type = QStringLiteral("ptt.release");
    request.requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    sendEnvelope(request);
}

void SessionClient::sendTxPcm(const QByteArray &pcm)
{
    if (!m_transmitting || m_media.state() != QAbstractSocket::ConnectedState)
        return;
    m_txPcmBuffer.append(pcm);
    constexpr qsizetype packetBytes = 1920;
    while (m_txPcmBuffer.size() >= packetBytes) {
        const QByteArray samples = m_txPcmBuffer.left(packetBytes);
        m_txPcmBuffer.remove(0, packetBytes);
        QByteArray message(24 + 12 + packetBytes, '\0');
        message.replace(0, 4, QByteArrayLiteral("FTB1"));
        message[4] = 1;
        message[5] = 3;
        const quint32 sequence = m_txSequence++;
        message[8] = char((sequence >> 24) & 0xff);
        message[9] = char((sequence >> 16) & 0xff);
        message[10] = char((sequence >> 8) & 0xff);
        message[11] = char(sequence & 0xff);
        const quint64 timestamp = quint64(QDateTime::currentMSecsSinceEpoch()) * 1000;
        for (int i = 0; i < 8; ++i)
            message[12 + i] = char((timestamp >> ((7 - i) * 8)) & 0xff);
        const quint32 payloadLength = 12 + packetBytes;
        message[20] = char((payloadLength >> 24) & 0xff);
        message[21] = char((payloadLength >> 16) & 0xff);
        message[22] = char((payloadLength >> 8) & 0xff);
        message[23] = char(payloadLength & 0xff);
        message[24] = 2; // PCM S16LE
        message[25] = 1;
        message[26] = 0;
        message[27] = 20;
        message[28] = 0;
        message[29] = 0;
        message[30] = 0xbb;
        message[31] = 0x80;
        message[32] = 0x03;
        message[33] = 0xc0;
        message[34] = 0x00;
        message[35] = 0x00;
        message.replace(36, packetBytes, samples);
        m_media.sendBinaryMessage(message);
    }
}

void SessionClient::onControlSslErrors(const QList<QSslError> &errors)
{
    Q_UNUSED(errors)
    handleSslCertificate(m_control.sslConfiguration().peerCertificate());
    if (certificateMatchesPin(m_control.sslConfiguration().peerCertificate()))
        m_control.ignoreSslErrors();
}

void SessionClient::onMediaSslErrors(const QList<QSslError> &errors)
{
    Q_UNUSED(errors)
    handleSslCertificate(m_media.sslConfiguration().peerCertificate());
    if (certificateMatchesPin(m_media.sslConfiguration().peerCertificate()))
        m_media.ignoreSslErrors();
}

void SessionClient::logout()
{
    if (!m_token.isEmpty()) {
        QNetworkRequest request(apiUrl(QStringLiteral("/api/v1/auth/logout")));
        request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_token.toUtf8());
        m_network.post(request, nullptr);
    }
    m_control.close();
    m_media.close();
    m_heartbeatTimer.stop();
    m_reconnectTimer.stop();
    m_reconnectAttempt = 0;
    m_token.clear();
    m_leaseId.clear();
    clearSafety();
    m_connected = false;
    m_transmitting = false;
    emit connectedChanged();
    emit transmittingChanged();
    setStatus(QStringLiteral("Logged out"));
}

} // namespace ftremote
