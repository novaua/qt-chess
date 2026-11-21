---
id: BL-31
title: "Keep the app log from growing forever"
type: task
status: open
priority: low
---

# BL-31 Keep the app log from growing forever

Found by the async review of 2026-11-21.

**Found:** `logs/ChessPlusPlus_AppDebug.log` (`main.cpp:492`) is opened in append mode (`applog.cpp:26`) and never rotated. `qDebug` is on in Release, and every Lichess stream line is logged, keep-alives included (`LichessStream.cpp:84`): about 40–50 KB an hour per open stream (estimate), plus every request (`makeRequest`).

**Fix:** at start, a log over ~5 MB is renamed to `.1` (an older `.1` replaced); empty keep-alive lines aren't logged (or only under a debug category).

**Test:** a log over the limit is rotated when opened, and the new one starts empty (ChessQtTests).

Also seen (cleanup, no ticket): `ChessGui/ChessGui/networkplayer.cpp/.h` (UDP multicast) is in no project: dead code.
