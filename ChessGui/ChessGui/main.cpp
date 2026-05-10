#include "stdafx.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include "chessconnector.h"
#include <QQmlContext>
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/pics/chess_icon.svg"));

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("chessConnector", new ChessConnector(&engine));

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/chessgui/ChessGame.qml")));

    return app.exec();
}
