#include "stdafx.h"
#include "engineworker.h"

EngineWorker::EngineWorker(Chess::EngineLevel level, Chess::PositionMode mode, QObject* parent)
    : QObject(parent), _level(level), _mode(mode)
{}

// Called from the GUI thread to stop a request: a blocking findMove() returns because the engine
// process is terminated through its Win32 handle (never through QProcess, which belongs to the
// engine thread), and a request that has not reached the engine yet gives up (BL-15).
void EngineWorker::killEngine()
{
    _abandoned.store(true);
    if (auto* connector = _connector.load())
        connector->Kill();
}

// Runs on the engine thread. ChessEnginePlayer (and its QProcess) are created here
// so they are owned by and always accessed from the engine thread.
void EngineWorker::findMove(int request, const Chess::MovesHistory& history)
{
    if (_abandoned.load())
        return; // the game was left before this request started
    ENGINE_TRACE("worker: request" << request << "started," << history.size() << "plies, thread" << QThread::currentThreadId());
    try {
        if (!_enginePlayer) {
            _enginePlayer = std::make_shared<Chess::ChessEnginePlayer>(_level, _mode);
            _connector.store(_enginePlayer->Connector());
        }
        if (_abandoned.load())
            return; // stopped while the engine was starting
        const auto move = QString::fromStdString(_enginePlayer->FindMove(history));
        ENGINE_TRACE("worker: request" << request << "answers" << move);
        emit moveFound(request, move);
    } catch (const std::exception& e) {
        if (_abandoned.load())
            return; // the error is from the process we just terminated
        ENGINE_TRACE("worker: request" << request << "failed:" << e.what());
        QString details;
        try {
            if (_enginePlayer)
                details = QString::fromStdString(_enginePlayer->DebugDump(history));
        } catch (const std::exception& dumpError) {
            details = QString("(debug dump failed: %1)").arg(dumpError.what());
        }
        emit moveError(request, QString::fromStdString(e.what()), details);
    }
}
