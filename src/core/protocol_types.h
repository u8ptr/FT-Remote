#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QJsonObject>
#include <QMetaType>
#include <QString>
#include <QStringList>

#include <optional>

namespace ftremote {

struct ErrorInfo {
    QString code;
    QString message;
    bool retryable = false;
    QJsonValue details;
};

struct Envelope {
    int version = 1;
    QString type;
    QString requestId;
    QJsonObject payload;

    QJsonObject toJson() const;
    static std::optional<Envelope> fromJson(const QByteArray &json, QString *error = nullptr);
};

struct StationState {
    qint64 revision = 0;
    bool catConnected = false;
    bool spectrumAvailable = false;
    QString spectrumStatus;
    bool audioRxAvailable = false;
    bool audioTxAvailable = false;
    bool transmitting = false;
    qint64 vfoAFrequencyHz = 0;
    std::optional<qint64> vfoBFrequencyHz;
    int radioMode = 0;
    int scopeSpanCode = 0;

    static StationState fromJson(const QJsonObject &object);
};

struct Capabilities {
    int protocolVersion = 1;
    QStringList catCommands;
    QJsonObject media;
    QJsonObject hardware;
};

struct SpectrumFrame {
    quint8 source = 0;
    quint8 scopeMode = 0;
    quint8 spanCode = 0;
    quint8 radioMode = 0;
    quint8 preamp = 0;
    quint8 attenuator = 0;
    quint8 sMeter = 0;
    quint64 stateRevision = 0;
    quint64 centerFrequencyHz = 0;
    quint64 startFrequencyHz = 0;
    quint32 spanHz = 0;
    QByteArray traceA;
    QByteArray traceB;
};

struct MediaHeader {
    quint8 version = 0;
    quint8 kind = 0;
    quint16 flags = 0;
    quint32 sequence = 0;
    quint64 timestampUs = 0;
    quint32 payloadLength = 0;
};

} // namespace ftremote

Q_DECLARE_METATYPE(ftremote::Capabilities)
Q_DECLARE_METATYPE(ftremote::SpectrumFrame)
