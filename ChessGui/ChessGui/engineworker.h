#pragma once
#include <QObject>
#include <atomic>
#include "ChessEnginePlayer.h"
#include "EngineLevel.h"

// The robot's requests and answers in the app log, in Debug builds only: if the board stays on
// "Robot is thinking", the log shows where the answer stopped. Use: ENGINE_TRACE("asked" << request);
#ifdef _DEBUG
#include <QDebug>
#include <QThread>
#define ENGINE_TRACE(...) (qDebug().noquote() << "[Engine trace]" << __VA_ARGS__)
#else
#define ENGINE_TRACE(...) ((void)0)
#endif

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

    // killEngine() runs on the GUI thread while findMove() may be creating the player here: keep
    // the pieces it needs in atomics. _connector points into _enginePlayer (stable until the worker
    // is destroyed on this thread after it stops).
    std::atomic<bool> _abandoned{false};
    std::atomic<UciConnector*> _connector{nullptr};

public:
    explicit EngineWorker(Chess::EngineLevel level = Chess::EngineLevel{3},
                          Chess::PositionMode mode = Chess::PositionMode::FenWindow,
                          QObject* parent = nullptr);

    void killEngine();

    // Runs on the engine thread. `request` comes back with the answer, so the caller can
    // drop the answer to a request it no longer waits for.
    void findMove(int request, const Chess::MovesHistory& history);

signals:
    void moveFound(int request, QString move); // UCI, e.g. "e7e8q"
    void moveError(int request, QString message, QString details); // details: multi-line DebugDump(), may be empty
};
