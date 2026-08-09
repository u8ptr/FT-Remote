#include "core/media_parser.h"

#include <QTest>

using namespace ftremote;

namespace {
void put16(QByteArray &data, qsizetype offset, quint16 value)
{
    data[offset] = char((value >> 8) & 0xff);
    data[offset + 1] = char(value & 0xff);
}
void put32(QByteArray &data, qsizetype offset, quint32 value)
{
    for (int i = 3; i >= 0; --i)
        data[offset + 3 - i] = char((value >> (i * 8)) & 0xff);
}
void put64(QByteArray &data, qsizetype offset, quint64 value)
{
    for (int i = 7; i >= 0; --i)
        data[offset + 7 - i] = char((value >> (i * 8)) & 0xff);
}
}

class MediaParserTest final : public QObject
{
    Q_OBJECT
private slots:
    void parsesSpectrum();
    void rejectsWrongLength();
};

void MediaParserTest::parsesSpectrum()
{
    QByteArray message(24 + 1740, '\0');
    message.replace(0, 4, QByteArrayLiteral("FTB1"));
    message[4] = 1;
    message[5] = 1;
    put32(message, 8, 17);
    put64(message, 12, 123456789);
    put32(message, 20, 1740);
    auto *payload = message.data() + 24;
    payload[0] = 1;
    payload[1] = 2;
    payload[2] = 6;
    put16(message, 24 + 8, 850);
    put64(message, 24 + 12, 8);
    put64(message, 24 + 20, 14250000);
    put64(message, 24 + 28, 13750000);
    put32(message, 24 + 36, 1000000);
    message[24 + 40] = char(0x12);
    message[24 + 890] = char(0x34);

    QString error;
    const auto frame = MediaParser::parseSpectrum(message, &error);
    QVERIFY2(frame.has_value(), qPrintable(error));
    QCOMPARE(frame->spanCode, quint8(6));
    QCOMPARE(frame->traceA.size(), 850);
    QCOMPARE(quint8(frame->traceA.at(0)), quint8(0x12));
    const auto header = MediaParser::parseHeader(message, &error);
    QCOMPARE(header->sequence, quint32(17));
}

void MediaParserTest::rejectsWrongLength()
{
    QByteArray message(24 + 1740, '\0');
    message.replace(0, 4, QByteArrayLiteral("FTB1"));
    message[4] = 1;
    message[5] = 1;
    put32(message, 20, 10);
    QString error;
    QVERIFY(!MediaParser::parseHeader(message, &error));
    QVERIFY(error.contains(QStringLiteral("payload")) || error.contains(QStringLiteral("length")));
}

QTEST_MAIN(MediaParserTest)
#include "test_media_parser.moc"
