#include "cat_catalog.h"

#include <QRegularExpression>

namespace ftremote {

QVector<CatCommandSpec> CatCatalog::commonCommands()
{
    return {
        {QStringLiteral("FA"), QStringLiteral("VFO A frequency"), true, true, false},
        {QStringLiteral("FB"), QStringLiteral("VFO B frequency"), true, true, false},
        {QStringLiteral("MD"), QStringLiteral("Operating mode"), true, true, false},
        {QStringLiteral("AG"), QStringLiteral("AF gain"), true, true, false},
        {QStringLiteral("RG"), QStringLiteral("RF gain"), true, true, false},
        {QStringLiteral("SQ"), QStringLiteral("Squelch"), true, true, false},
        {QStringLiteral("PA"), QStringLiteral("RF power"), true, true, false},
        {QStringLiteral("NB"), QStringLiteral("Noise blanker"), true, true, false},
        {QStringLiteral("NR"), QStringLiteral("Noise reduction"), true, true, false},
        {QStringLiteral("RA"), QStringLiteral("Preamp"), true, true, false},
        {QStringLiteral("AT"), QStringLiteral("Attenuator/tuner"), true, true, true},
        {QStringLiteral("TX"), QStringLiteral("Transmit coordinator"), true, true, true},
        {QStringLiteral("MX"), QStringLiteral("MOX"), true, true, true},
    };
}

bool CatCatalog::isHighRisk(const QString &command)
{
    const QStringList riskCommands{QStringLiteral("AC"), QStringLiteral("GP"), QStringLiteral("KY"),
                                   QStringLiteral("MT"), QStringLiteral("MW"), QStringLiteral("PS")};
    return riskCommands.contains(command.toUpper());
}

std::optional<Envelope> CatCatalog::makeRequest(const QString &command,
                                                  const QString &operation,
                                                  const QJsonObject &parameters,
                                                  const QString &requestId,
                                                  QString *error)
{
    const QString normalized = command.trimmed().toUpper();
    if (!QRegularExpression(QStringLiteral("^[A-Z]{2}$")).match(normalized).hasMatch()) {
        if (error)
            *error = QStringLiteral("CAT command must contain two uppercase letters");
        return std::nullopt;
    }
    if (operation != QStringLiteral("read") && operation != QStringLiteral("set") &&
        operation != QStringLiteral("action")) {
        if (error)
            *error = QStringLiteral("unsupported CAT operation");
        return std::nullopt;
    }

    QJsonObject validated = parameters;
    if (normalized == QStringLiteral("FA") || normalized == QStringLiteral("FB")) {
        if (operation == QStringLiteral("set")) {
            const auto value = parameters.value(QStringLiteral("frequency_hz"));
            if (!value.isDouble() || value.toInteger() < 30000 || value.toInteger() > 75000000) {
                if (error)
                    *error = QStringLiteral("frequency must be between 30000 and 75000000 Hz");
                return std::nullopt;
            }
            validated = {{QStringLiteral("frequency_hz"), value.toInteger()}};
        } else if (!parameters.isEmpty()) {
            if (error)
                *error = QStringLiteral("read FA/FB does not accept parameters");
            return std::nullopt;
        }
    } else if (normalized == QStringLiteral("EX")) {
        const QString selector = parameters.value(QStringLiteral("selector")).toString();
        if (!QRegularExpression(QStringLiteral("^[0-9]{6}$")).match(selector).hasMatch()) {
            if (error)
                *error = QStringLiteral("EX selector must be six decimal digits");
            return std::nullopt;
        }
    } else if (operation != QStringLiteral("read")) {
        const QString value = parameters.value(QStringLiteral("value")).toString();
        if (value.isEmpty() || value.toUtf8().size() > 64 || value.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1f\\x7f;]")))) {
            if (error)
                *error = QStringLiteral("CAT value contains forbidden bytes or is too long");
            return std::nullopt;
        }
    }

    Envelope request;
    request.type = QStringLiteral("cat.execute");
    request.requestId = requestId;
    request.payload = {{QStringLiteral("command"), normalized},
                       {QStringLiteral("operation"), operation},
                       {QStringLiteral("parameters"), validated}};
    return request;
}

} // namespace ftremote
