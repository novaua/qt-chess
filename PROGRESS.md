# Active Handover

- **Last completed:** BL-32 step 3, ChessGui (`docs/bl32ChessGui_221126`): `ChessGui/ChessGui/CLAUDE.md`, `.claude/rules/chessgui-cpp.md`, `qml/CLAUDE.md`; root 142 lines, 11.6k characters. BL-33 filed (unbuilt `networkplayer`). Before it: step 2, Lichess (#230).
- **Current focus:** BL-32: the owner reviews the Invariants lists (step 3); then step 4 (`ChessGuiTests/CLAUDE.md`, `run-chess-app` split into `SKILL.md` and `scenes.md`) (`docs/BL-32-ClaudeMdSplitPlan.md`). BL-28 waits for a next occurrence in CI.
- **Next up:** BL-32 steps 4–6; then LI-14 (plan first); LI-09's remaining hand tests (scenarios 1, 3–9); LI-12; BL-33.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items; no shader effects (the software renderer: pieces drawn without their glow; dialogs' backgrounds not drawn). `// ui-test-engine: none` runs a test file without an engine.
  - Screenshots in a real window time out while the screen is off (popup scenes too): `-Offscreen`. Local baselines are stale (e.g. `settings`, `about`); new scenes without one: `capture`, `capture-ep`, `premove-capture`, `online-game-menu`. `-Offscreen` pictures of the move list or a captured strip differ from older ones (their figures now drawn).
