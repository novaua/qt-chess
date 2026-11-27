# Active Handover

- **Last completed:** LI-18 step 2 (`features/li18OnlineCore_271126`): the `OnlineCore` DLL (`OnlineClient`, `OnlineStream`, the `OnlineServer` / `OnlineAuth` interfaces); `LiChessConnector` keeps `LichessServer`, `LichessAuth`, `LichessClient`. No behaviour change.
- **Current focus:** LI-18 step 3 (plugins: `OnlineHub`, `QPluginLoader`, QML `LichessClient` → `Online`). LI-16: coded; its hand tests on lichess.org (`docs/LI-16-ManualTests.md`) wait for the owner, then it closes. BL-28 waits for a next occurrence in CI.
- **Next up:** LI-18 step 3; LI-15 (a plan first, against `OnlineClient`). The owner's hand tests: LI-16, LI-09 (1, 3–9).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`, `lichess-result`, `lichess-new-opponent`, `games-lichess`; `games-empty`'s line changed (LI-14). `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).
  - A rookrelay server for LI-18's hand tests: the owner's local one (ask for its address; the connection doc, section 2).
