#include "stdafx.h"
#include "engineworker.h"

EngineWorker::EngineWorker(const Chess::GameAptr& game, QObject* parent)
    : QObject(parent), _game(game)
{}

// Runs on the engine thread. ChessEnginePlayer (and its QProcess) are created here
// so they are owned by and always accessed from the engine thread.
void EngineWorker::killEngine()
{
    if (_enginePlayer)
        _enginePlayer->KillEngine();
}

void EngineWorker::doMove()
{
    if (!_enginePlayer)
        _enginePlayer = std::make_shared<Chess::ChessEnginePlayer>(_game);

    try {
        _enginePlayer->DoMove();
        emit moveComplete();
    } catch (const std::exception& e) {
        emit moveError(QString::fromStdString(e.what()));
    }
}
