# Progress Log

Use this file to record where work stopped and what to do next. Newest entry at the top.

---

## 2026-10-29 — BL-02 P4 Game chrome done (branch: gameChrome_291026); next: P3

### What was done
- **Player cards** (`PlayerCard.qml`) above and below the board: avatar, name (`ChessConnector.PlayerName` / `OpponentName`), the robot's level and Elo, the turn (accent bar and avatar pulse), and a **captured strip** (`CapturedStrip.qml`). The strip groups pieces by type (P N B R Q), overlaps pieces of a type, and shows "+N" (`MaterialBalance`, counted from the board, so promotions count). Captures and material follow the reviewed position. When the board is flipped, the cards swap.
- **Moves:** a side panel (`MovesPanel` + `MoveList`) at width ≥ 960 or aspect > 1.15; otherwise a **bottom sheet** whose handle shows "Moves 12. Nf3", the status, and Back to live while reviewing. Esc closes the sheet first. The move list has two columns and a move-number column, piece images in the text colour, and tabular figures. **Click a move** to review it (`reviewTo(ply)`; `ShownPly` is highlighted). While reviewing, a slim InfoBar "Viewing 5. Nf3 · Back to live" sits at the bottom of the panel. The blinking button and "Reviewing" are gone.
- **Result:** one `ResultCard` over the dimmed board in every layout, with Rematch, New game (the setup dialog of the same mode; online goes Home → Lichess dialog) and Review moves. `GameResultDialog`, `GameResultCard`, `MoveHistoryPanel` and `CapturedPanel` were deleted. Their statistics moved to a "Statistics" line in the profile dialog until P3's Profile page.
- **PGN:** Copy PGN and Export PGN… are in the panel and under More. Export uses a `FileDialog` that suggests "White vs Black yyyy-MM-dd.pgn" and writes with `QSaveFile`; an InfoBar confirms. The tags are Event, Site (the lichess.org URL online), Date, Round, White, Black and Result.
- **ChessCore:** `FormatHistorySan()` replays the game and marks '#' from the real mate event, no longer from the result string. `FormatPgn()` escapes tag values and wraps at 80 columns. `PieceValue()`/`MaterialBalance()` are in `Board.h`. `ChessTests` has 80 tests, 13 new (history SAN, PGN, material). En passant now counts in the captured strip.
- **Scenes:** `review`, `result` and `moves` are new; `game` and `continue` work as before. `Take-Screenshot.ps1 -Offscreen` works when the screen is locked, dark only (see Notes). Debug and Release build OK; no QML warnings in the scene logs.

### Notes
- **Locked screen:** while Windows is locked, a real window renders no frames. Animations freeze (half-closed dialogs, empty squares) and the mate scene times out, on `develop` too. Use `-Offscreen` there: Fluent is always dark offscreen, so `Theme.dark` follows it. Dialog backgrounds don't render offscreen. The after shots in `docs/design/after-p4/` are dark for this reason.
- Board coordinates are still outside the squares (the inside/outside setting is P5).

### Where to continue
1. With the screen unlocked: `.\scripts\Take-Screenshot.ps1 -Scene game,review,result,moves` (light) plus `-Size 1400x900`, then add the light shots to `docs/design/after-p4/`.
2. P3 (Users and Lichess), on a new branch from `origin/develop` once this PR is merged.

---

## 2026-10-29 — BL-02 P2 Shell and navigation done (branch: shellNavigation_291026); next: P3 or P4

### What was done
- **Pages:** `ChessGame.qml` (986 → 491 lines) is the window: a `StackView` holding `UserLoginScreen`, `HomePage` or `GamePage`. `route()` picks the page from `UserManager.isLoggedIn` and `ChessConnector.InGame`. All dialogs live in the window and are opened with `open()`; pages emit signals for them. The `_show*` flags and `screen_1…4` states are gone.
- **Home:** an account button (avatar and name; menu with Profile and Switch user), Settings, a **Continue game** card (`ChessConnector.ContinueDescription`, e.g. "vs Bob"), and cards for Play vs computer, Two players, Play online (dimmed until Lichess is linked; then it opens Profile) and Open saved game (always enabled; it explains when there is nothing to open). The Exit button is gone.
- **Setup dialogs:** `ComputerGameDialog` (White/Black/Random and level; its info button swaps to the levels help and back, so dialogs never stack) and `TwoPlayerDialog` (side, and the opponent picked by profile **id** or Guest). Both use the new shared `ChoiceRow`.
- **Game command bar:** Home, title (`ChessConnector.GameTitle`: "vs Robot · Level 3", "Alice vs Bob", "Lichess · vs X", "Saved game · …"), Prev/Next (replay), Undo/Takeback, **Flip board** (new), Save, Offer draw, Resign (with `ConfirmDialog`), More (Computer levels, Settings, Profile, Switch user). Undo, Draw and Resign are no longer duplicated in the move panel.
- **InfoBar** replaces `Notificator` and the three banners: one message bar (Check, Game saved, errors, rematch declined) plus bars bound to `ChessConnector.PendingOffer` (draw or takeback) and `OpponentGone`/`ClaimWinSeconds`. Castling is no longer announced.
- **Game flows in C++:** `startComputerGame(level, side)`, `startLocalGame(white, opponentUserId)`, `switchUser()` (autosave, end the game, log out) and `refreshBoard()` (a new board view fills itself in). Lichess game start and end are handled in `ChessConnector`, which sets the result text, flags the king and sets the opponent's avatar. Game-end text is now `ResultText`, with names: "Bob wins by checkmate", "Alice resigned". `GameSaveInfo.opponentName` is stored for Continue and the title. The avatar is set per mode (the `newGameStarted → randomize` connection in `main.cpp` is gone), so a rematch keeps the opponent's avatar.
- **Typed singletons:** `qmlRegisterSingletonInstance("ChessPlusPlus", …)` for `ChessConnector`, `UserManager`, `LichessClient` and `AvatarProvider`, parented to the app so they outlive the engine. No `setContextProperty` is left.
- **Shortcuts** (off while a dialog is open): Esc Home, F flip, Ctrl+S, Ctrl+Z, ←/→/Home/End review, F1 levels, Ctrl+, settings, Alt+S move input. Minimum window size 640×560; the window title shows the game.
- **Scenes:** `home`, `continue`, `game` (Tester vs Bob), `computer`; `menu` and `board` still work as aliases. After shots: `docs/design/after-p2/`. Debug and Release builds OK; `ChessTests` 67/67; no QML warnings in the scene logs.

### Not checked live
- Keyboard shortcuts and Enter/Esc in a real window. Driving the window with SendKeys didn't reach the Qt controls, so the skill now says to add a scene instead.
- Lichess flows (challenge, game start and end, draw/takeback InfoBars, opponent gone) need a connected account. Add them to the Lichess live test in TASKS.

### Where to continue
P3 (Users and Lichess) and P4 (Game chrome) can run in parallel; each needs a new branch from `origin/develop` once this PR is merged. P3 replaces `UserLoginScreen` with the Welcome page and profile picker, and moves `CreateUserDialog`'s Lichess part to a Profile page. P4 replaces `CapturedPanel`, `GameResultDialog`/`GameResultCard` and `MoveHistoryPanel`.

---

## 2026-10-29 — BL-02 P1 Foundation done (branch: fluentFoundation_281026); next: P2

### What was done
- **Style:** `QQuickStyle::setStyle("FluentWinUI3")` in `main.cpp`. The palette block, inline `Button`/`ModeButton` and every `background:`/`contentItem:` override are gone. `Button.qml`, `ActionButton.qml`, `CustomSlider.qml`, `NetworkPlayers.qml` and `menu_icon.svg` were deleted.
- **`qml/Theme.qml`** singleton (`qml/qmldir`): colour roles, board, type ramp, spacing and motion as in `docs/design/ThemeTokens.md`; §8 there lists the differences. No `Window.window.color` and no hex colour outside `Theme.qml`.
- **System / Light / Dark:** `appSettings.theme` (Settings → "App theme"). It sets `Application.styleHints.colorScheme`; until it is set, the old `darkMode` decides. "System" follows Windows; checked live.
- **Icons:** `pics/icons/*.svg` are in `qml.qrc` under `qrc:/icons/` (`Theme.icon("undo")`). New `IconButton.qml` (icon-only `ToolButton` with tooltip and `Accessible.name`) is used in the command bar and the move panel.
- **`AppDialog.qml`:** a modal Fluent `Dialog` on the overlay with the `Theme.scrim`. Enter runs `accept()` and Esc runs `reject()`; footers are `DialogButtonBox`es. Settings, Profile/Create profile, Levels, Lichess, Challenge, Select player and Apply moves use it. Shown with `visible: _showX; onClosed: _showX = false`.
- **Board:** the `Glow` is replaced by an accent tint on the selected piece's square, cleared after a move. Legal-move dots use `Theme.legalMoveDot`, and coordinates use `textSecondary`. The photo shows only behind the start menu and login, faint.
- **Screenshots:** `-platform offscreen` can't force light/dark, and Fluent falls back to its dark palette there. Screenshot mode now uses a real window that is transparent, frameless, ignores input and has no taskbar button, at 100% scale. `Take-Screenshot.ps1` dropped `-Windowed`. New scenes: `online`, `players`. After shots: `docs/design/after-p1/`.
- Keyboard checked live: Enter creates a profile, F1 opens levels, Esc closes the dialog. `windeployqt` deploys the FluentWinUI3 plugin and its assets. Debug + Release build OK; `ChessTests` 67/67.

### Where to continue: P2 Shell and navigation (new branch from `origin/develop`)
Spec: plan §6 and §8 (P2 row). `StackView` pages, command bar, account menu, Home, InfoBar replaces `Notificator` and the banners, `_show*` flags removed, services as typed singletons, game flows into `ChessConnector`.

### Notes
- Not seen in a scene yet: the connected Lichess dialog, the Challenge dialog, the draw/takeback banners and the two resign confirmations. Check them during the Lichess live test (TASKS).
- `windeployqt --qmldir ChessGui\ChessGui` locally still lists Qt5Compat, but only because stale Qt VS Tools caches (`ChessGui\ChessGui\x64\*\qt\qmake\*\import.qml`) import it. CI starts clean.
- The board's own animations (elastic flip, InCirc piece rotation) are unchanged; motion tokens come in P6.

---

## 2026-10-28 — App modernisation planned (BL-02/BL-03); next: Phase 1

### What was done
- **#76 Screenshot mode:** `ChessGui.exe --screenshot <png> --scene <name>` (with `--moves`, `--black`, `--dark`, `--size`, `--delay`) on a throwaway profile; `--data-dir <dir>` for interactive runs. `scripts\Take-Screenshot.ps1 -Scene all [-Dark] [-Windowed] [-Build]` writes to `test-results\screens\`. The `run-chess-app` skill explains it.
- **#77 Modernisation plan:** `docs/AppModernizationPlan.md` holds the screen-by-screen review, target flows, roadmap P1–P6 and the mockup link (§10).
  - Design kit: 38 icons in `ChessGui/ChessGui/pics/icons/` (not in `qml.qrc` yet), `docs/design/ThemeTokens.md`, and "before" screenshots in `docs/design/before/`.
  - Decisions: Windows 11 Fluent (`FluentWinUI3` style, system theme and accent); local profiles with a linked Lichess account; profile export/import plus backups.
  - QML ↔ C++ rule (CLAUDE.md "Key Conventions"): game flows go through `ChessConnector`, one call per action; `UserManager` and `LichessClient` stay separate services and become typed singletons in P2.
- `TASKS.md`: BL-02/BL-03 is at the top of *In Progress*, with one sub-item per phase.

### Where to continue: P1 Foundation (new branch from `origin/develop`, e.g. `fluentFoundation_<ddmmyy>`)
Plan first (`/plan`), then implement. Spec: plan §6 and §8, `docs/design/ThemeTokens.md`.
1. `main.cpp`: `QQuickStyle::setStyle("FluentWinUI3")`. Remove the per-file `background:`/`contentItem:` overrides that fight the style: inline `Button` and `ModeButton` in `ChessGame.qml`, `Button.qml`, `ActionButton.qml`, `CustomSlider.qml`, the primary-button copies in the dialogs. Prefer plain Fluent controls (`highlighted: true` for the primary action).
2. `qml/Theme.qml` singleton (`qmldir` + `qml.qrc`). It replaces the 16 `Window.window.color` dark-mode blocks. System/Light/Dark sets `Application.styleHints.colorScheme`; migrate `appSettings.darkMode`.
3. Icons: add `pics/icons/*.svg` to `qml.qrc` and use them via `icon.source` + `icon.color`. This replaces the `ColorOverlay` menu icon and glyph labels ("⚑", "↩", "|<<").
4. `AppDialog` base: modal, with a scrim and Enter/Esc. Move the existing dialogs onto it (Settings, Profile, Levels, Lichess, Challenge, Select player, Apply moves).
5. Remove `Qt5Compat.GraphicalEffects` (`ColorOverlay` in `ChessGame.qml`; `Glow` in `ChessFigures.qml`, which becomes an accent square tint) and the unused `NetworkPlayers.qml`. That closes BL-01.
6. Keep every screenshot scene working (`setUpScreenshotScene()` in `ChessGame.qml`). Before/after shots, light and dark, go in the PR.

Out of scope for P1: the page split and navigation (P2), and piece moving/drag (BL-04).

### Notes
- After P1, offscreen screenshots should match windowed ones, because Qt5Compat effects are what rendered blank offscreen.
- Login cards are left-aligned (`UserLoginScreen.qml` `Flow`); this is fixed in P3, not P1.
- Manual checks still open (no code): VC++ runtime install, Stockfish install log, Lichess rematch live test. They're listed under *In Progress* in `TASKS.md`.

---

## 2026-10-28 — Lichess Rematch (branch: feLichessRematch_281026)

### What was done
- Merged earlier today: computer levels rework + levels help (#68), promotion dialog rotation fix (#69).
- Analysed why Rematch doesn't work vs Lichess: the button is hidden for online games and the Board API has no rematch endpoint. Plan written to `docs/LichessRematchPlan.md`: Rematch creates a new challenge (direct to a registered opponent, open link for anonymous).
- Implemented steps 1–4 (see `TASKS.md`): `LichessClient::createRematch()` / `parseRematchInfo()`, `ChessConnector::CanRematch`, Rematch button for online games, `ChallengeDialog` direct mode. Direct challenges use `keepAliveStream=true` (otherwise they expire after 20 s).
- Debug + Release build OK; `LichessStaticTests` 21/21 (4 new); app starts without QML errors.
- Testing showed an opponent's resignation was received (`status:"resign"`) but nothing was shown: result and Rematch lived only in the landscape side panels. Added step 4b (result card, notification, red flag on the losing king, frozen board, `opponentGone` + Claim victory).
- Note for testers: resigning on lichess.org needs the flag **and** the ✓ confirm within ~3 s; otherwise nothing reaches the server.

- Code review fixes (step 4d) and PR #70 opened. Live check OK for resign from the app and from an anonymous Lichess opponent.

- Merged: #70 (rematch + game end) and #71 (test host crash fix). `ChessQtTests` engine tests now run reliably (10/10).

### Where to continue
1. Play a game against a second Lichess account and run the manual scenarios in plan §5 (accept, decline, cancel, anonymous link, vs Robot unchanged, restart, leave while pending).
2. Watch the log for `LichessClient: challenge stream line:`. It confirms the keep-alive challenge JSON and the final `{"done":...}` shape this code expects.
3. If a live scenario fails, fix it on a new branch from `develop`.

### Notes
- If the opponent clicks Rematch in their browser, the Board API client probably never sees it; the app must send the challenge.

---

## 2026-10-27 — Release 0.1.0 preparation (branch: feRelease010_271026)

### What was done
- CI/CD re-checked. It is MSBuild-based, not CMake: `tests.yml` (Debug build + `ChessTests` on every push/PR) and `release.yml` (Release build, windeployqt, signing, Inno Setup installer). Last runs on `develop` and the PR installer build (2026-06-07) were green.
- `release.yml`: release step now uses `docs/release-notes/<tag>.md` as the body (generated notes appended) and fails if no installer is found.
- `installer/ChessSetup.iss`: `AppPublisherURL` corrected to `novaua/qt-chess`.
- Added `docs/release-notes/v0.1.0.md`; README links to the first release.

### Where to continue
1. Merge this branch to `develop` once CI is green.
2. Tag `v0.1.0` on `develop` and push the tag; the Build Installer workflow publishes the GitHub Release with `ChessSetup-0.1.0.exe`.
3. Check the release page, then install the EXE on a clean machine and run it.
4. The release-notes link to the EXE is `.../releases/download/v0.1.0/ChessSetup-0.1.0.exe`; confirm it resolves.

### Notes
- The old CMake pipeline (feMakeInstaller_0430a) was replaced by the MSBuild workflow; there is no `CMakeLists.txt`.
- The app does not display its version anywhere; the version only lives in the tag and installer name.
