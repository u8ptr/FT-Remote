#pragma once

#include "protocol_types.h"

#include <QByteArray>

#include <optional>

namespace ftremote {

class MediaParser final
{
public:
    static constexpr qsizetype HeaderSize = 24;
    static std::optional<MediaHeader> parseHeader(const QByteArray &message, QString *error = nullptr);
    static std::optional<SpectrumFrame> parseSpectrum(const QByteArray &message, QString *error = nullptr);

private:
    static quint16 u16(const char *data);
    static quint32 u32(const char *data);
    static quint64 u64(const char *data);
};

} // namespace ftremote
