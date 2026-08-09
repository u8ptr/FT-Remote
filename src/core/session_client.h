#pragma once

#include "protocol_types.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>
#include <QSslCertificate>
#include <QSslError>

namespace ftremote {

class SessionClient final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool transmitting READ transmitting NOTIFY transmittingChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(qint64 frequency READ frequency NOTIFY stationChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY stationChanged)

public:
    explicit SessionClient(QObject *parent = nullptr);

    bool connected() const { return m_connected; }
    bool transmitting() const { return m_transmitting; }
    QString status() const { return m_status; }
    qint64 frequency() const { return m_station.vfoAFrequencyHz; }
    QString mode() const;
    StationState station() const { return m_station; }

    Q_INVOKABLE void login(const QString &server, const QString &username, const QString &password);
    Q_INVOKABLE void trustCertificate(const QString &fingerprint);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void setFrequency(qint64 frequencyHz);
    Q_INVOKABLE void sendCat(const QString &command, const QString &operation, const QJsonObject &parameters);
    Q_INVOKABLE void prepareSafety(const QString &command, const QString &operation, const QJsonObject &parameters);
    Q_INVOKABLE void confirmSafety();
    Q_INVOKABLE void clearSafety();
    Q_INVOKABLE void pressPtt();
    Q_INVOKABLE void releasePtt();
    void sendTxPcm(const QByteArray &pcm);

signals:
    void connectedChanged();
    void transmittingChanged();
    void statusChanged();
    void stationChanged();
    void capabilitiesChanged(const ftremote::Capabilities &capabilities);
    void spectrumFrame(const ftremote::SpectrumFrame &frame, quint32 sequence, quint64 timestampUs);
    void certificateTrustRequired(const QString &origin,
                                  const QString &fingerprint,
                                  const QString &subject,
                                  const QString &issuer);
    void loginFailed(const QString &message);
    void protocolError(const QString &code, const QString &message, bool retryable);
    void mediaReady(const QString &codec);
    void rxPcm(const QByteArray &pcm);
    void safetyPrepared(const QString &confirmationId, const QString &expiresAt, const QString &actionDigest);

private slots:
    void onLoginFinished();
    void onControlConnected();
    void onControlText(const QString &message);
    void onControlDisconnected();
    void onMediaConnected();
    void onMediaText(const QString &message);
    void onMediaBinary(const QByteArray &message);
    void onMediaDisconnected();
    void sendHeartbeat();
    void attemptReconnect();
    void onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void onControlSslErrors(const QList<QSslError> &errors);
    void onMediaSslErrors(const QList<QSslError> &errors);

private:
    void setStatus(const QString &status);
    void fetchCapabilities();
    void openControl();
    void openMedia();
    void applyStation(const QJsonObject &payload);
    void sendEnvelope(const Envelope &envelope);
    void handleSslCertificate(const QSslCertificate &certificate);
    bool certificateMatchesPin(const QSslCertificate &certificate) const;
    QString origin() const;
    QUrl apiUrl(const QString &path) const;
    QUrl webSocketUrl(const QString &path) const;

    QNetworkAccessManager m_network;
    QNetworkReply *m_loginReply = nullptr;
    QUrl m_baseUrl;
    QString m_username;
    QString m_password;
    QString m_token;
    QString m_leaseId;
    QString m_pendingFingerprint;
    QString m_status;
    Capabilities m_capabilities;
    StationState m_station;
    QWebSocket m_control;
    QWebSocket m_media;
    QTimer m_heartbeatTimer;
    QTimer m_reconnectTimer;
    int m_idleHeartbeatTicks = 0;
    int m_heartbeatSeconds = 10;
    int m_reconnectAttempt = 0;
    bool m_connected = false;
    bool m_transmitting = false;
    bool m_waitingForCertificate = false;
    std::optional<Envelope> m_pendingSafetyRequest;
    QString m_pendingSafetyConfirmation;
    QByteArray m_txPcmBuffer;
    quint32 m_txSequence = 0;
};

} // namespace ftremote
