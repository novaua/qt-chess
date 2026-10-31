# 2026-10-31 — Debug crash: QRingBuffer assert on the engine thread

**Status:** fixed on branch `logRaceFix_311026` (mutex in the message handler).
**Screenshot:** [`Screenshot 2026-10-31 013022.png`](Screenshot%202026-10-31%20013022.png)
**Dump:** `%LOCALAPPDATA%\CrashDumps\ChessGui.exe.417660.dmp`

## What happened

During a normal robot game (level 1, the player was pressing Undo after the robot's moves), the
**Debug** build showed a Microsoft Visual C++ Runtime "Debug Error":

```
Program: D:\Tools\Qt\6.12.0\msvc2022_64\bin\Qt6Cored.dll
ASSERT: "bytes <= bufferSize" in file C:\Users\qt\work\qt\qtbase\src\corelib\tools\qringbuffer.cpp, line 76
```

Windows Error Reporting: faulting application `x64\Debug\ChessGui.exe`, exception `0x80000003`
(a debug breakpoint / Q_ASSERT) at `Qt6Cored.dll+0x12924c`, 01:33:12 (the dialog was left open
from 01:30:22 until Abort).

The assert is `Q_ASSERT(bytes <= bufferSize)` in `QRingBuffer::free()` — a **buffer was over-freed**,
which only happens with corrupted or concurrently changed buffer state.

## Evidence

**The dump**: the crashing thread is `0x476c0` — the **engine worker thread** (the app log prints the
same id for the robot's requests). Its stack contains `EngineWorker::findMove`, the
`ChessConnector::askEngineMove` queued-lambda frames, and the Qt read path (no `stopEngineThread` /
`killEngine` frames). So the assert fired while the engine thread was reading Stockfish's output.

**The log** (`ChessPlusPlus_AppDebug.log`): the last line is `ask: request 37` / `worker: request 37
started` at 01:29:57.786. In all 1,985 lines of the log there is exactly **one duplicated consecutive
line**, and it is at that moment:

```
[01:29:57.786] [Engine trace] ask: request 37 posted
[01:29:57.786] [Engine trace] ask: request 37 posted
[01:29:57.786] [Engine trace] worker: request 37 started, 36 plies, thread 0x476c0
```

The GUI thread (`posted`) and the engine thread (`worker: started`) were writing the log at the same
time. Requests 4, 8 and 11 are missing from the numbering: those are the engine-request bumps of the
Undo pressed while a robot answer was held — each Undo cleared the answer and asked again.

## Root cause

`customLogMessageHandler()` in `main.cpp` writes the shared `g_logFile` with a `QTextStream`.
**Qt calls a message handler from any thread and does not serialize the calls** (`qlogging.cpp` uses
only a `thread_local` recursion guard; the documentation says a handler "might be called from
different threads, in parallel"). Two `QTextStream`s writing the same `QFile` at once corrupt the
heap. The corruption hit the engine thread's `QProcess` read buffer (`QRingBuffer`), which is why the
assert pointed at a completely different part of the program. In Release there is no `Q_ASSERT`, so
the same corruption is silent.

### Reproduced in isolation

A standalone Qt console program with the same handler and two threads logging 20,000 messages each:

| | result |
|---|---|
| handler as in the app | exits `0xC0000374` (**STATUS_HEAP_CORRUPTION**) after 4 lines, with one duplicated line — the same fingerprint as the app |
| handler with a `QMutex` around the write + flush | 40,001 intact lines, no duplicates, exit 0 |

The repro (not part of the build; Qt 6.12 Debug, link `Qt6Cored.lib`):

```cpp
static QFile* g_logFile; static QMutex g_logMutex; static bool useMutex;
void handler(QtMsgType t, const QMessageLogContext&, const QString& msg) {
    if (useMutex) { QMutexLocker lock(&g_logMutex);
        QTextStream(g_logFile) << msg << '\n'; g_logFile->flush();
    } else { QTextStream(g_logFile) << msg << '\n'; }
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    useMutex = getenv("MUTEX");
    g_logFile = new QFile("logstress.txt");
    g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    qInstallMessageHandler(handler);
    std::thread a([]{ for (int i = 0; i < 20000; ++i) qDebug() << "A" << i; });
    std::thread b([]{ for (int i = 0; i < 20000; ++i) qDebug() << "B" << i; });
    a.join(); b.join();
}
```

## Fix

`main.cpp`: a `QMutex g_logMutex`, a `QMutexLocker` around the `QTextStream` write, and
`g_logFile->flush()` per line (so a crash does not lose the last lines either). Nothing else in the
app writes that file.

To check: play the Debug build against the robot (Undo often); the duplicated log lines and the
assert should be gone. The stress program above is the regression test.

## Related, not fixed here

`ChessConnector::stopEngineThread()` calls `EngineWorker::killEngine()` from the GUI thread, which
reaches `QProcess::kill()` / `QProcess::state()` while the engine thread uses the same `QProcess`;
Qt objects are thread-affine, so this is a separate cross-thread hazard (as is the unsynchronized
`_enginePlayer` shared_ptr), and `stopEngineThread()` deletes the `QThread` even if `wait(3000)`
times out. Logged as **BL-15** in `TASKS.md`.
