#include "media_parser.h"

namespace ftremote {

quint16 MediaParser::u16(const char *data)
{
    return (quint16(quint8(data[0])) << 8) | quint16(quint8(data[1]));
}

quint32 MediaParser::u32(const char *data)
{
    return (quint32(quint8(data[0])) << 24) | (quint32(quint8(data[1])) << 16) |
           (quint32(quint8(data[2])) << 8) | quint32(quint8(data[3]));
}

quint64 MediaParser::u64(const char *data)
{
    quint64 value = 0;
    for (int i = 0; i < 8; ++i)
        value = (value << 8) | quint64(quint8(data[i]));
    return value;
}

std::optional<MediaHeader> MediaParser::parseHeader(const QByteArray &message, QString *error)
{
    if (message.size() < HeaderSize) {
        if (error)
            *error = QStringLiteral("media message is shorter than 24-byte header");
        return std::nullopt;
    }
    if (message.left(4) != QByteArrayLiteral("FTB1")) {
        if (error)
            *error = QStringLiteral("invalid media magic");
        return std::nullopt;
    }

    MediaHeader header;
    header.version = quint8(message[4]);
    header.kind = quint8(message[5]);
    header.flags = u16(message.constData() + 6);
    header.sequence = u32(message.constData() + 8);
    header.timestampUs = u64(message.constData() + 12);
    header.payloadLength = u32(message.constData() + 20);

    if (header.version != 1 || header.kind < 1 || header.kind > 3 || header.flags & ~quint16(1) ||
        quint64(message.size()) != quint64(HeaderSize) + header.payloadLength) {
        if (error)
            *error = QStringLiteral("invalid media header fields or payload length");
        return std::nullopt;
    }
    return header;
}

std::optional<SpectrumFrame> MediaParser::parseSpectrum(const QByteArray &message, QString *error)
{
    const auto header = parseHeader(message, error);
    if (!header || header->kind != 1 || header->payloadLength != 1740) {
        if (error && error->isEmpty())
            *error = QStringLiteral("spectrum payload must be 1740 bytes");
        return std::nullopt;
    }

    const char *payload = message.constData() + HeaderSize;
    if (quint8(payload[0]) != 1 || u16(payload + 8) != 850) {
        if (error)
            *error = QStringLiteral("unsupported spectrum source or bin count");
        return std::nullopt;
    }

    SpectrumFrame frame;
    frame.source = quint8(payload[0]);
    frame.scopeMode = quint8(payload[1]);
    frame.spanCode = quint8(payload[2]);
    frame.radioMode = quint8(payload[3]);
    frame.preamp = quint8(payload[4]);
    frame.attenuator = quint8(payload[5]);
    frame.sMeter = quint8(payload[6]);
    frame.stateRevision = u64(payload + 12);
    frame.centerFrequencyHz = u64(payload + 20);
    frame.startFrequencyHz = u64(payload + 28);
    frame.spanHz = u32(payload + 36);
    frame.traceA = QByteArray(payload + 40, 850);
    frame.traceB = QByteArray(payload + 890, 850);
    return frame;
}

} // namespace ftremote
