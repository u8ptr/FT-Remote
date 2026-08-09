#include "credential_store.h"

#ifdef FTREMOTE_HAS_QTKEYCHAIN
#include <qt6keychain/keychain.h>
#endif

namespace ftremote {

CredentialStore::CredentialStore(QObject *parent)
    : QObject(parent)
{
}

QString CredentialStore::key(const QString &serverOrigin, const QString &username) const
{
    return serverOrigin.trimmed().toLower() + QStringLiteral("/") + username;
}

void CredentialStore::readPassword(const QString &serverOrigin, const QString &username)
{
#ifdef FTREMOTE_HAS_QTKEYCHAIN
    auto *job = new QKeychain::ReadPasswordJob(QStringLiteral("FT Remote"), this);
    job->setKey(key(serverOrigin, username));
    connect(job, &QKeychain::Job::finished, this, [this, job] {
        if (job->error() == QKeychain::Error::NoError)
            emit passwordRead(job->textData());
        else
            emit passwordRead(QString());
        job->deleteLater();
    });
    job->start();
#else
    Q_UNUSED(serverOrigin)
    Q_UNUSED(username)
    emit unavailable(QStringLiteral("QtKeychain is not installed; password persistence is disabled"));
    emit passwordRead(QString());
#endif
}

void CredentialStore::writePassword(const QString &serverOrigin,
                                    const QString &username,
                                    const QString &password)
{
#ifdef FTREMOTE_HAS_QTKEYCHAIN
    auto *job = new QKeychain::WritePasswordJob(QStringLiteral("FT Remote"), this);
    job->setKey(key(serverOrigin, username));
    job->setTextData(password);
    connect(job, &QKeychain::Job::finished, this, [this, job] {
        emit passwordWritten(job->error() == QKeychain::Error::NoError, job->errorString());
        job->deleteLater();
    });
    job->start();
#else
    Q_UNUSED(serverOrigin)
    Q_UNUSED(username)
    Q_UNUSED(password)
    emit passwordWritten(false, QStringLiteral("QtKeychain is not installed"));
#endif
}

void CredentialStore::deletePassword(const QString &serverOrigin, const QString &username)
{
#ifdef FTREMOTE_HAS_QTKEYCHAIN
    auto *job = new QKeychain::DeletePasswordJob(QStringLiteral("FT Remote"), this);
    job->setKey(key(serverOrigin, username));
    connect(job, &QKeychain::Job::finished, this, [this, job] {
        emit passwordDeleted(job->error() == QKeychain::Error::NoError, job->errorString());
        job->deleteLater();
    });
    job->start();
#else
    Q_UNUSED(serverOrigin)
    Q_UNUSED(username)
    emit passwordDeleted(false, QStringLiteral("QtKeychain is not installed"));
#endif
}

} // namespace ftremote
