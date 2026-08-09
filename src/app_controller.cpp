#include "app_controller.h"

#include "core/cat_catalog.h"

#include <QSettings>
#include <QRegularExpression>

namespace ftremote {

AppController::AppController(QObject *parent)
    : QObject(parent), m_session(this), m_credentials(this), m_audio(this)
{
    connect(&m_session, &SessionClient::connectedChanged, this, &AppController::connectedChanged);
    connect(&m_session, &SessionClient::transmittingChanged, this, &AppController::transmittingChanged);
    connect(&m_session, &SessionClient::statusChanged, this, &AppController::statusChanged);
    connect(&m_session, &SessionClient::stationChanged, this, &AppController::stationChanged);
    connect(&m_session, &SessionClient::loginFailed, this, &AppController::loginError);
    connect(&m_session, &SessionClient::spectrumFrame, this, &AppController::handleSpectrum);
    connect(&m_session, &SessionClient::rxPcm, &m_audio, &AudioEngine::feedRxPcm);
    connect(&m_session, &SessionClient::safetyPrepared, this,
            [this](const QString &confirmation, const QString &expiresAt, const QString &digest) {
                m_safetyConfirmation = confirmation;
                m_safetyExpiresAt = expiresAt;
                m_safetyDigest = digest;
                emit safetyPrepared();
            });
    connect(&m_audio, &AudioEngine::txPcmReady, &m_session, &SessionClient::sendTxPcm);
    connect(&m_audio, &AudioEngine::runningChanged, this, &AppController::audioRunningChanged);
    connect(&m_session, &SessionClient::certificateTrustRequired, this,
            [this](const QString &, const QString &fingerprint, const QString &subject, const QString &) {
                m_pendingFingerprint = fingerprint;
                m_pendingSubject = subject;
                emit certificateTrustRequired();
            });
    loadSettings();
}

void AppController::loadSettings()
{
    QSettings settings;
    m_server = settings.value(QStringLiteral("login/server")).toString();
    m_username = settings.value(QStringLiteral("login/username")).toString();
    m_rememberPassword = settings.value(QStringLiteral("login/remember"), false).toBool();
    if (m_rememberPassword && !m_server.isEmpty() && !m_username.isEmpty()) {
        connect(&m_credentials, &CredentialStore::passwordRead, this, [this](const QString &password) {
            m_savedPassword = password;
            emit loginFieldsChanged();
        }, Qt::SingleShotConnection);
        m_credentials.readPassword(m_server, m_username);
    }
}

void AppController::setServer(const QString &server)
{
    if (m_server == server)
        return;
    m_server = server;
    emit loginFieldsChanged();
}

void AppController::setUsername(const QString &username)
{
    if (m_username == username)
        return;
    m_username = username;
    emit loginFieldsChanged();
}

void AppController::setRememberPassword(bool remember)
{
    if (m_rememberPassword == remember)
        return;
    m_rememberPassword = remember;
    emit loginFieldsChanged();
}

void AppController::login(const QString &server, const QString &username, const QString &password, bool remember)
{
    m_server = server.trimmed();
    m_username = username.trimmed();
    m_rememberPassword = remember;
    QSettings settings;
    settings.setValue(QStringLiteral("login/server"), m_server);
    settings.setValue(QStringLiteral("login/username"), m_username);
    settings.setValue(QStringLiteral("login/remember"), remember);
    if (remember)
        m_credentials.writePassword(m_server, m_username, password);
    else
        m_credentials.deletePassword(m_server, m_username);
    emit loginFieldsChanged();
    m_session.login(m_server, m_username, password);
}

void AppController::trustPendingCertificate()
{
    if (m_pendingFingerprint.isEmpty())
        return;
    m_session.trustCertificate(m_pendingFingerprint);
    m_pendingFingerprint.clear();
    m_pendingSubject.clear();
    emit certificateTrustRequired();
}

void AppController::rejectPendingCertificate()
{
    m_pendingFingerprint.clear();
    m_pendingSubject.clear();
    m_session.logout();
    emit certificateTrustRequired();
}

void AppController::logout()
{
    m_audio.stop();
    m_session.logout();
}

void AppController::setFrequency(const QString &frequencyHz)
{
    bool ok = false;
    QString normalizedFrequency = frequencyHz;
    normalizedFrequency.remove(QRegularExpression(QStringLiteral("[^0-9]")));
    const qint64 value = normalizedFrequency.toLongLong(&ok);
    if (ok)
        m_session.setFrequency(value);
}

void AppController::pressPtt()
{
    m_session.pressPtt();
}

void AppController::releasePtt()
{
    m_session.releasePtt();
}

void AppController::sendCat(const QString &command, const QString &operation, const QString &value)
{
    QJsonObject parameters;
    if (operation != QStringLiteral("read"))
        parameters.insert(QStringLiteral("value"), value);
    if (CatCatalog::isHighRisk(command))
        m_session.prepareSafety(command, operation, parameters);
    else
        m_session.sendCat(command, operation, parameters);
}

void AppController::confirmSafety()
{
    m_session.confirmSafety();
    cancelSafety();
}

void AppController::cancelSafety()
{
    m_session.clearSafety();
    m_safetyConfirmation.clear();
    m_safetyExpiresAt.clear();
    m_safetyDigest.clear();
    emit safetyPrepared();
}

void AppController::startAudio()
{
    m_audio.start();
}

void AppController::stopAudio()
{
    m_audio.stop();
}

void AppController::handleSpectrum(const SpectrumFrame &frame, quint32, quint64)
{
    m_spectrum.clear();
    m_waterfallRow.clear();
    for (const auto byte : frame.traceA) {
        const double value = double(quint8(byte)) / 255.0;
        m_spectrum.append(value);
        m_waterfallRow.append(value);
    }
    emit spectrumChanged();
}

} // namespace ftremote
