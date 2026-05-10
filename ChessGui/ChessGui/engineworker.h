#pragma once
#include <QObject>
#include "Game.h"
#include "ChessEnginePlayer.h"

// Runs on a dedicated QThread so that QProcess (inside UciConnector) is
// created and used only from its owning thread, satisfying Qt's threading rules.
class EngineWorker : public QObject
{
    Q_OBJECT
    Chess::GameAptr _game;
    Chess::ChessEnginePlayerAptr _enginePlayer;
    int _difficulty;

public:
    explicit EngineWorker(const Chess::GameAptr& game, int difficulty = 10, QObject* parent = nullptr);

    void killEngine();

public slots:
    void doMove();

signals:
    void moveComplete();
    void moveError(QString message);
};
