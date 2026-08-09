#pragma once

#include "core/credential_store.h"
#include "core/audio_engine.h"
#include "core/session_client.h"

#include <QVariantList>

namespace ftremote {

class AppController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool transmitting READ transmitting NOTIFY transmittingChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString server READ server WRITE setServer NOTIFY loginFieldsChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY loginFieldsChanged)
    Q_PROPERTY(QString savedPassword READ savedPassword NOTIFY loginFieldsChanged)
    Q_PROPERTY(bool rememberPassword READ rememberPassword NOTIFY loginFieldsChanged)
    Q_PROPERTY(QString pendingFingerprint READ pendingFingerprint NOTIFY certificateTrustRequired)
    Q_PROPERTY(QString pendingCertificateSubject READ pendingCertificateSubject NOTIFY certificateTrustRequired)
    Q_PROPERTY(QVariantList spectrum READ spectrum NOTIFY spectrumChanged)
    Q_PROPERTY(QVariantList waterfallRow READ waterfallRow NOTIFY spectrumChanged)
    Q_PROPERTY(qint64 frequency READ frequency NOTIFY stationChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY stationChanged)
    Q_PROPERTY(bool audioRunning READ audioRunning NOTIFY audioRunningChanged)
    Q_PROPERTY(QString safetyConfirmation READ safetyConfirmation NOTIFY safetyPrepared)
    Q_PROPERTY(QString safetyExpiresAt READ safetyExpiresAt NOTIFY safetyPrepared)
    Q_PROPERTY(QString safetyDigest READ safetyDigest NOTIFY safetyPrepared)

public:
    explicit AppController(QObject *parent = nullptr);

    bool connected() const { return m_session.connected(); }
    bool transmitting() const { return m_session.transmitting(); }
    QString status() const { return m_session.status(); }
    QString server() const { return m_server; }
    QString username() const { return m_username; }
    QString savedPassword() const { return m_savedPassword; }
    bool rememberPassword() const { return m_rememberPassword; }
    QString pendingFingerprint() const { return m_pendingFingerprint; }
    QString pendingCertificateSubject() const { return m_pendingSubject; }
    QVariantList spectrum() const { return m_spectrum; }
    QVariantList waterfallRow() const { return m_waterfallRow; }
    qint64 frequency() const { return m_session.frequency(); }
    QString mode() const { return m_session.mode(); }
    bool audioRunning() const { return m_audio.running(); }
    QString safetyConfirmation() const { return m_safetyConfirmation; }
    QString safetyExpiresAt() const { return m_safetyExpiresAt; }
    QString safetyDigest() const { return m_safetyDigest; }

    void setServer(const QString &server);
    void setUsername(const QString &username);

    Q_INVOKABLE void login(const QString &server, const QString &username, const QString &password, bool remember);
    Q_INVOKABLE void trustPendingCertificate();
    Q_INVOKABLE void rejectPendingCertificate();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void setFrequency(const QString &frequencyHz);
    Q_INVOKABLE void pressPtt();
    Q_INVOKABLE void releasePtt();
    Q_INVOKABLE void sendCat(const QString &command, const QString &operation, const QString &value);
    Q_INVOKABLE void confirmSafety();
    Q_INVOKABLE void cancelSafety();
    Q_INVOKABLE void startAudio();
    Q_INVOKABLE void stopAudio();
    Q_INVOKABLE void setRememberPassword(bool remember);

signals:
    void connectedChanged();
    void transmittingChanged();
    void statusChanged();
    void loginFieldsChanged();
    void certificateTrustRequired();
    void loginError(const QString &message);
    void stationChanged();
    void spectrumChanged();
    void audioRunningChanged();
    void safetyPrepared();

private:
    void loadSettings();
    void handleSpectrum(const SpectrumFrame &frame, quint32 sequence, quint64 timestampUs);

    SessionClient m_session;
    CredentialStore m_credentials;
    AudioEngine m_audio;
    QString m_server;
    QString m_username;
    QString m_savedPassword;
    bool m_rememberPassword = false;
    QString m_pendingFingerprint;
    QString m_pendingSubject;
    QVariantList m_spectrum;
    QVariantList m_waterfallRow;
    QString m_safetyConfirmation;
    QString m_safetyExpiresAt;
    QString m_safetyDigest;
};

} // namespace ftremote
