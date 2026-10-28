# Progress Log

Use this file to record where work stopped and what to do next. Newest entry at the top.

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
