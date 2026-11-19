---
id: BL-22
title: "Broadcast: watch the game in a browser, like a chess TV"
type: task
status: open
priority: medium
---

# BL-22 Broadcast: watch the game in a browser, like a chess TV

<!-- migrated from TASKS.md:112 (In Progress / Backlog) -->

- [ ] BL-22 Broadcast: watch the game in a browser, like a chess TV [Medium], **not started; needs a plan** (the owner, 2026-11-19).
  - **What it is:** the app hosts a small HTTP server. A browser that opens its address sees the game on the board live:
    - the board, with the last move and a check marked;
    - the players' names, and the clocks when the game has them;
    - the move list;
    - the result when the game ends;
    - the FEN of the current position on request.
  - **Off by default:** a Settings section "Broadcast" (or under Advanced) turns it on; until then no server runs and no port is open.
  - **What is shown:** the game on the board in every mode: vs the robot, two players, a Lichess game, maybe a replay. Between games the page says "No game on the board" and keeps the last result. Only one game, the one in the window: there's no channel list.
  - **Read only:** a viewer can't move, offer anything or change anything. No profile data beyond the shown names, never the Lichess token.
  - **Addresses** (to fix in the plan):
    - `/` the viewer page, from the app's resources (qrc): HTML, CSS and a little JS, the board drawn with the app's own piece images, light and dark, fitting a phone too;
    - `/fen` the current FEN as `text/plain`;
    - `/pgn` the game so far (`GamePgn`);
    - `/api/game` the state as JSON (players, side to move, moves in SAN and UCI, FEN, clocks, result);
    - `/events` the live updates as **Server-Sent Events** (`text/event-stream`, the browser's `EventSource`, which reconnects on its own). It is simpler than WebSockets, and one direction is all that is needed.
  - **The server, to choose in the plan:**
    - **Qt HTTP Server** (`QHttpServer`, a Qt add-on module, as Multimedia is): it would have to be installed, built in and deployed (the installer, `Deploy-Qt.ps1`);
    - **a small server of our own on `QTcpServer`**, as `FakeLichess` is in the tests: GET only, a few routes, no new dependency. Recommended, since the needs are small.
  - The server runs on the GUI thread or its own thread. It only reads a snapshot that `ChessConnector` publishes after each change, so viewers never touch the game.
  - **Settings:**
    - the switch;
    - the port (default e.g. 8765; "in use" reported, not a silent failure);
    - **who can watch**: this computer only (`127.0.0.1`, the default) or the local network (all addresses, Windows' firewall will ask; a warning);
    - for the network, perhaps a random access code in the URL;
    - the address shown with Copy and Open in browser, and how many are watching.
  - **Fair play, a question for the owner:** broadcasting a running Lichess game lets someone watching help the player, which breaks Lichess's rules. Options:
    - no broadcast of a running Lichess game;
    - a delay (e.g. N moves or seconds, as Lichess TV delays);
    - a warning only.
  - **C++:** a `BroadcastServer` class (ChessGui, its source also in `ChessGuiTests.vcxproj`), owned by `AppServices`, started and stopped by the setting; `ChessConnector` gives the snapshot and a change signal (moves, takebacks, result, a new game, the clock each second).
  - **Tests:**
    - `ChessQtTests` with `QNetworkAccessManager` against the server: `/fen` after moves, `/api/game`, an update on `/events` after a move, a takeback, an unknown route 404, the server off when the setting is off;
    - a UI test that turns it on and reads `/fen`;
    - the viewer page checked by hand in Edge and on a phone in the same network;
    - a screenshot of the Settings section.
