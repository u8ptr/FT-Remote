#include "protocol_types.h"

#include <QJsonDocument>
#include <QJsonValue>

#include <algorithm>

namespace ftremote {

QJsonObject Envelope::toJson() const
{
    QJsonObject object{{QStringLiteral("version"), version},
                       {QStringLiteral("type"), type},
                       {QStringLiteral("payload"), payload}};
    if (!requestId.isEmpty())
        object.insert(QStringLiteral("request_id"), requestId);
    return object;
}

std::optional<Envelope> Envelope::fromJson(const QByteArray &json, QString *error)
{
    if (json.size() > 64 * 1024) {
        if (error)
            *error = QStringLiteral("protocol message exceeds 64 KiB");
        return std::nullopt;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error)
            *error = parseError.errorString();
        return std::nullopt;
    }

    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("version")).toInt() != 1 ||
        !object.value(QStringLiteral("type")).isString() ||
        !object.value(QStringLiteral("payload")).isObject()) {
        if (error)
            *error = QStringLiteral("invalid protocol envelope");
        return std::nullopt;
    }

    Envelope envelope;
    envelope.version = object.value(QStringLiteral("version")).toInt();
    envelope.type = object.value(QStringLiteral("type")).toString();
    envelope.requestId = object.value(QStringLiteral("request_id")).toString();
    const QByteArray requestIdBytes = envelope.requestId.toUtf8();
    const bool requestIdAscii = std::all_of(requestIdBytes.cbegin(), requestIdBytes.cend(), [](char byte) {
        const auto value = static_cast<unsigned char>(byte);
        return value >= 0x21 && value <= 0x7e;
    });
    if (!envelope.requestId.isEmpty() &&
        (requestIdBytes.size() > 64 || requestIdBytes.isEmpty() || !requestIdAscii)) {
        if (error)
            *error = QStringLiteral("invalid request_id");
        return std::nullopt;
    }
    envelope.payload = object.value(QStringLiteral("payload")).toObject();
    return envelope;
}

StationState StationState::fromJson(const QJsonObject &object)
{
    StationState state;
    state.revision = object.value(QStringLiteral("revision")).toInteger();
    state.catConnected = object.value(QStringLiteral("cat_connected")).toBool();
    state.spectrumAvailable = object.value(QStringLiteral("spectrum_available")).toBool();
    state.spectrumStatus = object.value(QStringLiteral("spectrum_status")).toString();
    state.audioRxAvailable = object.value(QStringLiteral("audio_rx_available")).toBool();
    state.audioTxAvailable = object.value(QStringLiteral("audio_tx_available")).toBool();
    state.transmitting = object.value(QStringLiteral("transmitting")).toBool();
    state.vfoAFrequencyHz = object.value(QStringLiteral("vfo_a_frequency_hz")).toInteger();
    if (object.value(QStringLiteral("vfo_b_frequency_hz")).isDouble())
        state.vfoBFrequencyHz = object.value(QStringLiteral("vfo_b_frequency_hz")).toInteger();
    state.radioMode = object.value(QStringLiteral("radio_mode")).toInt();
    state.scopeSpanCode = object.value(QStringLiteral("scope_span_code")).toInt();
    return state;
}

} // namespace ftremote
