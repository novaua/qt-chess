#include <QApplication>
#include <QQmlApplicationEngine>
#include "chessconnector.h"
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;

    auto chessConnectorPtr = new ChessConnector() ;
    engine.rootContext()->setContextProperty("chessConnector", chessConnectorPtr);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));


    return app.exec();
}
