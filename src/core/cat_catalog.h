#pragma once

#include "protocol_types.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace ftremote {

struct CatCommandSpec {
    QString command;
    QString label;
    bool canRead = true;
    bool canSet = true;
    bool requiresConfirmation = false;
};

class CatCatalog final
{
public:
    static QVector<CatCommandSpec> commonCommands();
    static bool isHighRisk(const QString &command);
    static std::optional<Envelope> makeRequest(const QString &command,
                                                const QString &operation,
                                                const QJsonObject &parameters,
                                                const QString &requestId,
                                                QString *error = nullptr);
};

} // namespace ftremote
