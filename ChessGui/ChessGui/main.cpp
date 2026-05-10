#include "stdafx.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include "chessconnector.h"
#include <QQmlContext>
int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("ChessPlusPlus");
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/app/pics/chess_icon.svg"));

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("chessConnector", new ChessConnector(&engine));

    engine.load(QUrl(QStringLiteral("qrc:/qml/ChessGame.qml")));

    return app.exec();
}
