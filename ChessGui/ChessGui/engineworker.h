#pragma once
#include <QObject>
#include "Game.h"
#include "ChessEnginePlayer.h"
#include "EngineLevel.h"

// Runs on a dedicated QThread so that QProcess (inside UciConnector) is
// created and used only from its owning thread, satisfying Qt's threading rules.
class EngineWorker : public QObject
{
    Q_OBJECT
    Chess::GameAptr _game;
    Chess::ChessEnginePlayerAptr _enginePlayer;
    Chess::EngineLevel _level;
    Chess::PositionMode _mode;

public:
    explicit EngineWorker(const Chess::GameAptr& game,
                          Chess::EngineLevel level = Chess::EngineLevel{3},
                          Chess::PositionMode mode = Chess::PositionMode::FenWindow,
                          QObject* parent = nullptr);

    void killEngine();

public slots:
    void doMove();

signals:
    void moveComplete();
    void moveError(QString message);
};
