#include "stdafx.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include "chessconnector.h"
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Fusion");
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("chessConnector", new ChessConnector(&engine));

    engine.load(QUrl(QStringLiteral("qrc:/ChessGame.qml")));

    return app.exec();
}
