#include "stdafx.h"
#include "engineworker.h"

EngineWorker::EngineWorker(const Chess::GameAptr& game, int difficulty, QObject* parent)
    : QObject(parent), _game(game), _difficulty(difficulty)
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
        _enginePlayer = std::make_shared<Chess::ChessEnginePlayer>(_game, _difficulty);

    try {
        _enginePlayer->DoMove();
        emit moveComplete();
    } catch (const std::exception& e) {
        emit moveError(QString::fromStdString(e.what()));
    }
}
