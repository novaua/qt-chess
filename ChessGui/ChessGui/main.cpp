#include "stdafx.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include "chessconnector.h"
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;

    auto chessConnectorPtr = std::make_unique<ChessConnector>() ;
    engine.rootContext()->setContextProperty("chessConnector", chessConnectorPtr.get());

    engine.load(QUrl(QStringLiteral("qrc:/ChessGame.qml")));

    return app.exec();
}
