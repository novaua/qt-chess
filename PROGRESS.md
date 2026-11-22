# Active Handover

- **Last completed:** LI-14 step 1 (`features/li14LichessRecord_221126`): the record of a finished Lichess game (`kind: "lichess"`, `LichessGameInfo`, id `lichess-<id>`), its statistics and PGN; nothing writes one yet.
- **Current focus:** LI-14 (plan `docs/LI-14-LichessAnalysisPlan.md`): step 2 next (kept when the result comes, Analyse on the board), then step 3 (the Games page). BL-28 waits for a next occurrence in CI.
- **Next up:** LI-14 steps 2–3; BL-36 (the leave flow out of `ChessGame.qml`); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12; BL-33.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).
