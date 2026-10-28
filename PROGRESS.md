# Progress Log

Use this file to record where work stopped and what to do next. Newest entry at the top.

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
