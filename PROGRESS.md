# Active Handover

- **Last completed:** BL-32 step 2, Lichess (`docs/bl32Lichess_221126`): `LiChessConnector/CLAUDE.md` and `ChessQtTests/CLAUDE.md`; the token rule in the root's Key Conventions. Before it: step 1, Core (#229).
- **Current focus:** BL-32, split CLAUDE.md per project: step 3 (ChessGui: `ChessGui/ChessGui/CLAUDE.md`, `.claude/rules/chessgui-cpp.md`, `qml/CLAUDE.md`; the owner reviews the Invariants) next (`docs/BL-32-ClaudeMdSplitPlan.md`). BL-28 waits for a next occurrence in CI.
- **Next up:** BL-32 steps 3–6; then LI-14 (plan first); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).
