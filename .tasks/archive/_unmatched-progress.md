# PROGRESS.md entries with no matching ticket

Moved here from PROGRESS.md by the `.tasks/` migration; review and file them under a ticket.

## 2026-11-18 — Bug: a Lichess sign-in that can't be decrypted said Connected (branch: bugs/lichessTokenUndecryptable_181126)

### Done
- Found while planning BL-20 (moving a profile): the Lichess token is encrypted for the Windows account that linked it (`CryptProtectData`). In a data folder copied to another PC or Windows account it can't be decrypted: the app sent no token, yet the profile said **Connected**, and nothing on Lichess worked, with no hint why.
- **`UserManager::lichessToken()`** (C++ only): the decrypted token; one that can't be decrypted marks the link expired (`setLichessExpired`), so Home, the Online page and the Profile ask to reconnect, as after a 401. `AppServices::updateLichessToken` uses it.
- UserManagerTest +1 (`LichessToken_CantBeDecryptedHere_AsksToReconnect`). CLAUDE.md.

### Checked
- Debug: ChessQtTests 282/282; UI tests `tst_lichess`, `tst_lichess960`, `tst_profiles`, `tst_firstRun` passed.

## 2026-11-12 — Compare-Screenshots.ps1 on PowerShell 7.7 (branch: bugs/compareScreenshotsDrawing_121126)

- **Broken:** on PowerShell 7.7 (.NET 11) `Compare-Screenshots.ps1` stopped before comparing anything: `CS1069: The type name 'Bitmap' could not be found in the namespace 'System.Drawing'. This type has been forwarded to assembly 'System.Drawing.Common'`. The script hands its C# helper the assemblies already loaded, and `Add-Type -AssemblyName System.Drawing` now loads only the facade, not `System.Drawing.Common`.
- **Fixed:** the script names `[System.Drawing.Bitmap]` before collecting them, which loads the assembly it is forwarded to.
- **Checked:** all scenes, light and dark, after PR #150 (unlocked screen): **202 compared, 200 the same**; `about` and `about-dark` differ only by the build date in the version line (noted in the `run-chess-app` skill).

## 2026-11-09 — The InfoBars no longer cover the opponent's card (branch: infoBarsRoom_091126)

### What was done
- The owner's choice on the note below: the game page makes room for the window's InfoBars (an offer, a challenge, the reconnecting bar, the opponent gone, a message). `GamePage.infoBarsHeight` (bound to the bar column's height in `ChessGame.qml`) moves the board and its cards down by the bars' height while any shows, 8 px under them. In a tall window the board uses its spare height first; in a wide window (height-bound) it shrinks while the bar shows. No animation: the bar itself appears at once. Home and the other pages are unchanged (their content is centred or scrolls).
- Screenshots, light and dark: 100 compared, only `reconnecting` and `challenge-ingame` changed (the card now under the bar): looked at and accepted as new baselines. Also checked at `-Size 1400x900`. QML only: no C++ tests run.

### Next
- BL-13 plan (wide-screen layout); it decides where the bars go in the new layout.

## 2026-10-30 — Continue fixes: autosave only vs the robot, check sound after the page shows (branch: continueFixes_301026)

### What was done
- **Only a game vs the robot is autosaved.** Closing the app (`~ChessConnector`) autosaved any unfinished game, a Lichess game too; `endGame()` also autosaved two-player games. Home then offered them as a local game, but neither can really be resumed. Now one rule, `canAutoSave()` (vs the robot, not online, not a replay, not over, at least one move), decides in both places. `canContinue()` ignores an older autosave of another kind (`autoSaveInfo().isSinglePlayer`), so it isn't offered; `continueGame()` lost its two-player branch. Two-player games can no longer be continued (owner's decision: neither they nor Lichess games can be resumed properly today).
- **Check sound on Continue.** `continueGame()` replays the saved moves (`ResumeFromLoad`), and every check in them emitted `checkNotify`: the sound (once per check of the game) and the "Check" bar came before the game page appeared. The replay is quiet now (`_resuming`), and `GamePage` signals `checkShown()` when it has settled — at least 1 s after it opened and after the board has turned to the player (`ChessBoard.turning`; Black) — if the position is in check and no move was made meanwhile (that move announces its own check); `ChessGame.qml` plays the sound and shows the bar (`announceCheck()`, shared with `onCheckNotify`).
- Screenshot scene `continue` now autosaves a game vs the robot (Level 3, as White: no engine needed); its Home card says "vs Robot · Level 3".
- Debug and Release build; ChessTests 111, ChessQtTests 85 pass; all scenes match (`continue` baselines renewed).

### Where to continue
- Manual check: Continue a robot game that stands in check (sound and bar once, after the board shows); close the app during a Lichess game and a two-player game (no Continue offered on Home next time).

## 2026-10-30 — Moved to Qt 6.12.0 (branch: qt612Upgrade_301026)

### What was done
- **Every Qt project now builds against `6.12.0_msvc2022_64`**: ChessGui, LiChessConnector, UciConnector, ChessQtTests. ChessCore was already plain C++.
- **ChessTests no longer uses Qt.** It was still a Qt VS Tools project (`QtVS_v304`, `core;network`, the QtMsBuild imports, include paths to UciConnector and ChessGui), but its tests use only ChessCore. Now it's a plain `Win32Proj`: its DLL depends only on the test framework and the C++ runtime, and its 86 tests pass with no Qt on `PATH`. CI (`release.yml`, `tests.yml`) installs 6.12.0 with only `qtmultimedia`: `qt5compat` and `qtshadertools` aren't used since BL-02 P1.
- **Modules:** the app needs Qt Multimedia (the `SoundEffect`s in `ChessGame.qml` and `SettingsPage.qml`). It's an add-on that the 6.12.0 install didn't have at first; with it added, nothing else is missing. Also not installed, and not used: Qt5Compat, Charts, Quick3D, ShaderTools, Quick Timeline.
- **Qt 6.11 change: `DialogButtonBox` sets `highlighted` on its buttons** and highlights only its new `defaultButton`. A `highlighted: true` on a footer button was overwritten to `false`, so every dialog's primary button lost its accent colour. The 8 dialogs with a primary button now set `defaultButton: primaryButton` on the box (see `AppDialog.qml`).
- **Checked:** Debug and Release build clean; 86 + 71 tests pass; all 42 screenshot scenes render. Compared with the 6.10 baselines, the rest of the differences are 6.12's semibold Fluent dialog titles, text anti-aliasing, and the Qt version and build date on About.

### Where to continue
- **CI installs aqtinstall from a pinned `master` commit** (`aqtsource:` in `release.yml` and `tests.yml`): 3.3.0, its latest release, can't find Qt 6.11+ for Windows (one repository folder per compiler since 6.11; fix miurahr/aqtinstall#1000 is merged but unreleased). Drop the `aqtsource` line once an aqtinstall release after 3.3.0 is out.
- Accept the new look as the baselines: `scripts\Compare-Screenshots.ps1 -Update` (local).
- Locally, put `D:\Tools\Qt\6.12.0\msvc2022_64\bin` on `PATH` in place of 6.10.2's (the app takes Qt's DLLs from there). You can also make 6.12.0 the default in Qt VS Tools.

## 2026-10-29 — Check glow on the king (branch: checkHighlight_291026)

- The king in check glows red-yellow and pulses gently, so a missed "Check" InfoBar doesn't leave the player wondering why most moves are refused. `ChessConnector::CheckedKing` gives the square for the position on the board (live, reviewed, or a saved game's replay); `Game::IsInCheck()` (side to move) is new in ChessCore with a test. The glow is a `QtQuick.Shapes` radial gradient in `ChessBoxes.qml` (`Theme.checkGlowCenter` / `checkGlowEdge` replace the unused `checkSquare`).
- Checked: `game` with `e2e4 f7f6 d1h5` (light; dark and flipped with Bb5+), a mate (the red flagged king on top), and a reviewed check that is blocked live. No QML warnings. ChessTests 86/86.
- Independent of P3 (#84): branched from `develop`.

## 2026-10-29 — PGN date kept in saves (branch: pgnGameDate_291026)

- Hand check by the user: Copy PGN and Export PGN work, but a continued game exported `[Date "????.??.??"]`.
- `GameSaveInfo.startDate` (`startDate` in users.json) is written with the autosave and the saved game, and read back by Continue and Open saved game. Saves made before this still give `????.??.??`. Checked: the `continue` scene's autosave holds `"startDate": "2026-10-29"`. `ChessTests` 85/85.
