#pragma once

#include <QObject>
#include <QString>

namespace ftremote {

class CredentialStore final : public QObject
{
    Q_OBJECT
public:
    explicit CredentialStore(QObject *parent = nullptr);

    void readPassword(const QString &serverOrigin, const QString &username);
    void writePassword(const QString &serverOrigin, const QString &username, const QString &password);
    void deletePassword(const QString &serverOrigin, const QString &username);

signals:
    void passwordRead(const QString &password);
    void passwordWritten(bool success, const QString &error);
    void passwordDeleted(bool success, const QString &error);
    void unavailable(const QString &reason);

private:
    QString key(const QString &serverOrigin, const QString &username) const;
};

} // namespace ftremote
