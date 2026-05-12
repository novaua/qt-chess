#include "stdafx.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "chessconnector.h"
#include "AvatarProvider.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("ChessPlusPlus");
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/app/pics/chess_icon.svg"));

    QQmlApplicationEngine engine;

    auto* connector      = new ChessConnector(&engine);
    auto* avatarProvider = new AvatarProvider(&engine);

    QObject::connect(connector, &ChessConnector::newGameStarted,
                     avatarProvider, &AvatarProvider::randomize);

    engine.rootContext()->setContextProperty("chessConnector",  connector);
    engine.rootContext()->setContextProperty("avatarProvider",  avatarProvider);

    engine.load(QUrl(QStringLiteral("qrc:/qml/ChessGame.qml")));

    return app.exec();
}
