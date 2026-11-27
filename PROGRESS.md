# Active Handover

- **Last completed:** LI-18 plan (`docs/onlineServersPlan_271126`): online servers as plugins (Lichess, rookrelay), `docs/LI-18-OnlineServersPlan.md`; waits for the owner's decisions.
- **Current focus:** LI-18: the plan's "To decide" list, then step 2 (OnlineCore extracted). LI-16: coded; its hand tests on lichess.org (`docs/LI-16-ManualTests.md`) wait for the owner, then it closes. BL-28 waits for a next occurrence in CI.
- **Next up:** LI-18 step 2; LI-15 (a plan first, against LI-18's `OnlineClient`). The owner's hand tests: LI-16, LI-09 (1, 3–9).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`; `starting`, `timing`, `levels`, the message, challenge and profile scenes renewed from `develop` on 2026-11-22); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`, `lichess-result`, `lichess-new-opponent`, `games-lichess`; `games-empty`'s line changed (LI-14). `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).
  - A rookrelay server for hand tests: ask the owner for its address (the connection doc, section 2).
