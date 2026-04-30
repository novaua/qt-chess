#include "stdafx.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include "chessconnector.h"
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("chessConnector", new ChessConnector(&engine));

    engine.load(QUrl(QStringLiteral("qrc:/ChessGame.qml")));

    return app.exec();
}
