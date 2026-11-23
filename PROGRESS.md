# Active Handover

- **Last completed:** LI-14's review fixes (`bugs/li14ReviewFixes_231126`): a move held past a Lichess game's end completes its record; statistics, More's Analyse, the Chess960 PGN Event and the Games page's order of Lichess games fixed. LI-14 itself closed in #252.
- **Current focus:** None. BL-28 waits for a next occurrence in CI.
- **Next up:** BL-36 (the leave flow out of `ChessGame.qml`); LI-16 (the end of a Lichess game: New opponent; Analyse is LI-14's); LI-15 (recent Lichess games from the server); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12; BL-33.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`, `lichess-result`, `games-lichess`; `games-empty`'s line changed (LI-14). `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).
