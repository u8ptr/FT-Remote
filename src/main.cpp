#include "app_controller.h"

#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("FT Remote"));
    QCoreApplication::setApplicationName(QStringLiteral("FT Remote"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QTranslator translator;
    if (QLocale::system().language() == QLocale::Chinese)
        translator.load(QStringLiteral(":/i18n/ftremote_zh_CN.qm"));
    app.installTranslator(&translator);

    ftremote::AppController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    engine.loadFromModule(QStringLiteral("FTRemote"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
