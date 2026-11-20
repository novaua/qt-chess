#pragma once
#include <QObject>
#include <atomic>
#include "ChessEnginePlayer.h"
#include "EngineLevel.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QThread>

// The robot's requests and answers in the app log: if the board stays on "Robot is thinking", the
// log shows where the answer stopped. Category chess.engine: on in Debug builds and in --screenshot
// mode (main.cpp: the CI's scenes, BL-28), else QT_LOGGING_RULES="chess.engine.debug=true".
// Use: ENGINE_TRACE("asked" << request);
Q_DECLARE_LOGGING_CATEGORY(chessEngine)
#define ENGINE_TRACE(...) qCDebug(chessEngine).noquote() << "[Engine trace]" << __VA_ARGS__ // a statement (Qt's for-loop)

// Runs on a dedicated QThread so that QProcess (inside UciConnector) is
// created and used only from its owning thread, satisfying Qt's threading rules.
// It never sees the app's Game (the GUI thread reads and changes it): findMove() gets a
// copy of the moves, and ChessConnector plays the move it returns.
class EngineWorker : public QObject
{
    Q_OBJECT
    Chess::ChessEnginePlayerAptr _enginePlayer;
    Chess::EngineLevel _level;
    Chess::PositionMode _mode;
    Chess::StartPosition _start; // the game's set-up (Chess960: the engine is told, LI-01A)

    // killEngine() runs on the GUI thread while findMove() may be creating the player here: keep
    // the pieces it needs in atomics. _playerForKill is owned by _enginePlayer (stable until the
    // worker is destroyed on this thread after it stops).
    std::atomic<bool> _abandoned{false};
    std::atomic<Chess::ChessEnginePlayer*> _playerForKill{nullptr};

public:
    explicit EngineWorker(Chess::EngineLevel level = Chess::EngineLevel{3},
                          Chess::PositionMode mode = Chess::PositionMode::FenWindow,
                          const Chess::StartPosition& start = Chess::StartPosition(),
                          QObject* parent = nullptr);

    void killEngine();

    // Runs on the engine thread. `request` comes back with the answer, so the caller can
    // drop the answer to a request it no longer waits for. maxMoveMs: the robot's clock (0: none).
    void findMove(int request, const Chess::MovesHistory& history, int maxMoveMs = 0);

signals:
    void moveFound(int request, QString move); // UCI, e.g. "e7e8q"
    void moveError(int request, QString message, QString details); // details: multi-line DebugDump(), may be empty
};
