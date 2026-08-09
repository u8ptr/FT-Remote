#include "core/cat_catalog.h"
#include "core/protocol_types.h"

#include <QJsonDocument>
#include <QTest>

using namespace ftremote;

class ProtocolTest final : public QObject
{
    Q_OBJECT
private slots:
    void envelopeRoundTrip();
    void rejectsMalformedEnvelope();
    void validatesFrequencyAndGenericValues();
};

void ProtocolTest::envelopeRoundTrip()
{
    Envelope source;
    source.type = QStringLiteral("session.heartbeat");
    source.requestId = QStringLiteral("hb-1");
    source.payload = {{QStringLiteral("ok"), true}};
    const auto encoded = QJsonDocument(source.toJson()).toJson(QJsonDocument::Compact);
    QString error;
    const auto decoded = Envelope::fromJson(encoded, &error);
    QVERIFY2(decoded.has_value(), qPrintable(error));
    QCOMPARE(decoded->version, 1);
    QCOMPARE(decoded->type, source.type);
    QCOMPARE(decoded->requestId, source.requestId);
    QCOMPARE(decoded->payload, source.payload);
}

void ProtocolTest::rejectsMalformedEnvelope()
{
    QString error;
    QVERIFY(!Envelope::fromJson(QByteArrayLiteral("{}"), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!Envelope::fromJson(QByteArrayLiteral("{\"version\":1,\"type\":\"x\",\"payload\":{},\"request_id\":\"\u0001\"}"), &error));
}

void ProtocolTest::validatesFrequencyAndGenericValues()
{
    QString error;
    QVERIFY(CatCatalog::makeRequest(QStringLiteral("FA"), QStringLiteral("set"),
                                    {{QStringLiteral("frequency_hz"), 14250000}}, QStringLiteral("id"), &error));
    QVERIFY(!CatCatalog::makeRequest(QStringLiteral("FA"), QStringLiteral("set"),
                                     {{QStringLiteral("frequency_hz"), 100}}, QStringLiteral("id"), &error));
    QVERIFY(!CatCatalog::makeRequest(QStringLiteral("MD"), QStringLiteral("set"),
                                     {{QStringLiteral("value"), QStringLiteral("bad;value")}}, QStringLiteral("id"), &error));
    QVERIFY(CatCatalog::isHighRisk(QStringLiteral("KY")));
}

QTEST_MAIN(ProtocolTest)
#include "test_protocol.moc"
