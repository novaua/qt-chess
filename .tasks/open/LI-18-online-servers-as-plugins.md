---
id: LI-18
title: "Online servers as plugins: Lichess and rookrelay"
type: task
status: in_progress
priority: high
---

# LI-18 Online servers as plugins: Lichess and rookrelay

The owner (2026-11-27): play online on **rookrelay**, our own server, as on Lichess: challenge a friend, quick
pairing, the game itself. rookrelay answers Lichess's Board API with the same paths and JSON
(`docs/rookrelay/chesspp-connection.md`). Most of the code is shared; what differs is kept apart; "a
plugin-like solution, so the Lichess or the rookrelay connector could be removed without breaking the others."

**Plan:** [`docs/LI-18-OnlineServersPlan.md`](../../docs/LI-18-OnlineServersPlan.md): an `OnlineCore` DLL (the
Board API client, shared), one Qt plugin per server (`LichessOnline`, `RookrelayOnline`) for what differs (addresses,
sign-in, links, clocks, variants, pages, wording, as *traits*), an `OnlineHub` that loads them, accounts per server
in a profile with the token bound to its server's address. Six steps, one PR each; steps 2 and 3 change no
behaviour. Its "To decide" list waits for the owner.

Replaces the connection doc's C1 (`--server`, one server per data folder); its C2–C4 are parts of step 5.

Related: TSK-13 (fast games on a service of our own), LI-15, LI-01B.

## Resolution

### 2026-11-27 — step 2: OnlineCore extracted (branch: features/li18OnlineCore_271126)

- New DLL `OnlineCore`: `LichessClient` → `OnlineClient`, `LichessStream` → `OnlineStream`, `LichessPlayer` →
  `OnlinePlayer` (moved with `git mv`), and the interfaces `OnlineServer` and `OnlineAuth`. The client asks its
  server for the addresses, the links, `canPlay` / `playsVariant`, the seek's 429 wording and its name in the
  messages ("Lichess didn't answer" and the rest read as before).
- `LiChessConnector` keeps Lichess's own: `LichessServer`, `LichessAuth` (now an `OnlineAuth`), and `LichessClient`
  (an `OnlineClient` with a `LichessServer`: the app's and the tests'). `UserManager` signs in through the
  server's `createAuth` (`setOnlineServer`).
- ChessGui's C++ uses the `Online…` types; QML, `ChessConnector`'s `Lichess…` members and the singleton's name
  are step 3's (the plan's table says so now). `OnlineCore.dll` in `release.yml`, `Build-Installer.ps1`,
  `Test-Installer.ps1`.
- Checked: the solution builds in Debug and Release without warnings; ChessQtTests 359/359 (Debug); UI tests
  (Debug) 13 files, 0 failed. Screenshots not compared: no QML changed, and the messages read as before.
