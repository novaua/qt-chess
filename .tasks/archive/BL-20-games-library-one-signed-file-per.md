---
id: BL-20
title: "Games library: one signed file per game"
type: task
status: done
priority: medium
---

# BL-20 Games library: one signed file per game

<!-- migrated from TASKS.md:28 (In Progress) -->

- [x] **BL-20 Games library: one signed file per game** [Medium] (done 2026-11-19) — plan: [`docs/BL-20-GameFilesPlan.md`](docs/BL-20-GameFilesPlan.md) (the owner, 2026-11-18: a game per file instead of one `games.json`, so a move writes only its game; every game signed (HMAC, one app-wide key) so an edited game is set aside, not loaded: a simple anti-cheat and an integrity check; the robot rating and the statistics signed too). The owner's answers (2026-11-18): recover the rating and statistics (not recompute); an old export imports unrated. Steps:
  - [x] 1 a signed file per game (`GameSignature`), set aside (`games/rejected/`, the message), migration from `games.json`, export / import of the files (branch `features/gameFiles_181126`)
  - [x] 2 the rating and the statistics signed, their recovery (branch `features/signedRating_181126`)
  - [x] 3 an old export imported unrated; the signed rating and statistics in an export (branch `features/signedExport_191126`)
  - [x] 4 docs and hand tests: [`docs/BL-20-ManualTests.md`](docs/BL-20-ManualTests.md) (8 scenarios: a game edited, damaged or copied; the rating and the statistics edited; export / import between two data folders, an edited export; your own data from before BL-20 updated; another Windows account, a reinstall) (branch `docs/gameFilesManualTests_191126`)
  - [x] The review of #187–#190 (10 findings): `games.json` and a `users.json` below version 3 not trusted beside signed files; the catch-up only along the games that continue the rating; a failed remove tried again; a migration tried again doesn't keep a game twice (`fileIdOf`); `lichessToken()` saves after the call; a read failure enum; one signing and one atomic-write helper; each profile's games read once for a rebuild (branch `bugs/bl20ReviewFixes_191126`)
  - [x] The hand tests: 1, 2, 3, 5, 6, 7 passed (the owner, 2026-11-19); 4 (the statistics edited by hand) passed in Claude's run on a copy of the owner's folder, which also cross-checked 1, 2, 3 and 7 (2026-11-19); 8 (another Windows account, a reinstall) optional, not run

## Resolution

### 2026-11-18 — BL-20 plan: one signed file per game (branch: docs/gameFilesPlan_181126)

#### Done
- **`docs/BL-20-GameFilesPlan.md`**: the owner's request (a game per file; every game signed against editing, also an integrity check) and choices: a failing game set aside (`games/rejected/`), the rating and statistics signed too, one app-wide HMAC key. Design: `GameSignature` (HMAC-SHA256 over the record's canonical JSON, a domain per kind), `users/<id>/games/<gameId>.json`, `rating.json`, the statistics' `statsSig`, migration from `games.json` (trusted once), export / import of the signed files; 4 steps; tests; risks.
- Found while planning: the rating and the statistics can't be recomputed from the kept games (trimmed games, Lichess games, the "earlier" row); the plan recovers them instead (the newest verified rated game; the newest verifying backup of `users.json`). For the owner to confirm, with the old-export question (§3.6).
- `TASKS.md`: BL-20.

#### Next
- The owner's answers to the plan's two open questions; then step 1.

### 2026-11-18 — BL-20 step 1: one signed file per game (branch: features/gameFiles_181126)

#### Done
- The owner's answers to the plan's questions: recover the rating and statistics from what is signed; an export from before BL-20 imports unrated (step 3). Written into the plan (§2.6, §2.7).
- **`GameSignature`** (`gamesignature.h/cpp`): HMAC-SHA256 (`QMessageAuthenticationCode`), one app-wide key (key 1, two masked halves), a domain per kind (game, rating, statistics), signatures `"<key>:<hex>"`; a key number this copy doesn't know fails.
- **`GameLibrary`**: a file per game, `users/<id>/games/<gameId>.json` (`{"record", "sig"}`, the record's canonical compact JSON signed), only the changed game written (`_changed` / `_removed`); the rating in `rating.json` (signed in step 2); a game that fails its check (bad signature, not JSON, no signature, unknown key, another game's id) moved to `games/rejected/`, never loaded: `setAsideCount`, `gamesSetAside` → "N kept games couldn't be verified and were set aside." (`ChessGame.qml`, also for the first profile, loaded before the window); records signed with an older key re-signed when written; ids that can't name a file get a new UUID. `games.json` (BL-19) migrated once, trusted and signed, then `games.json.migrated`; one that can't be read now: migrated at a later write (`_legacyPending`, the rating waits, games ended meanwhile rated after), a damaged one kept aside as before.
- **Export / import** (`UserManager`): export format 2 carries `games/<id>.json` and `rating.json`; import writes only those names under `games/` (no paths out); a format 1 export's `games.json` migrates as above.
- Tests: `GameSignatureTest` (5, key 1 pinned by a Python-computed HMAC), `GameLibraryTest` (the storage tests rewritten for the files; new: migration, an edited game set aside, other failing files, only the changed game written), UserManagerTest +2 (export / import of the signed files, only game file names written), `tst_restart` (run 1 edits a kept game's file, run 2 sets it aside and says so). `TestRun.addEditedGame()`.
- CLAUDE.md, the plan (step 3's export / import of the files moved into step 1, so no release loses games in exports).

#### Checked
- Debug: ChessTests 165/165, ChessQtTests 292/292, UI tests 12 files, 0 failed; scene `games` as before.

#### Next
- Step 2 (`features/signedRating_…`): `rating.json` signed, `ratedGames` on rated records, the statistics' `statsSig`, recovery.

### 2026-11-18 — BL-20 step 2: the rating and the statistics signed (branch: features/signedRating_181126)

#### Done
- **The rating** (`GameLibrary`): `rating.json` signed (`ratingFileContent`, `readRatingFile`, `GameSignature::Kind::Rating`); every rated record has `ratedGames` (the rated games after it), so the newest rated game gives the whole rating (`ratingFromGames`). At load (`recoverRating`): a `rating.json` that fails its check (edited, unsigned, not JSON) is moved to `games/rejected/` and the rating restored from the newest rated game, else 1200; a missing one while rated games are kept is restored the same way; one behind the newest rated game (a crash between the two writes) catches up silently. `ratingRestored`, `ratingRestoredOnLoad`; `ChessGame.qml`: "Your rating couldn't be verified and was restored from your kept games." / "… was started again." The migration of `games.json` works out `ratedGames` along the rated records.
- **The statistics** (`UserManager`): `users.json` version 3 (`kUsersVersion`), each profile's statistics signed (`statsSig`, also in an export); an older file is trusted once and signed at the next save. Statistics that fail their check: the newest backup whose statistics for that profile verify, else rebuilt from the kept games (`GameLibrary::keptRecordsOf`: the profile's, and its two-player games another profile kept), Lichess and older rows lost; written signed at once (that file, not the failing one, becomes the backup). `takeRestoredStatistics`, `statisticsRestored`; `ChessGame.qml` says it per profile. Also for a backup restored from Settings or after a damaged `users.json`.
- Step 1 fix: the import's game file name pattern lacked a backslash (`\.` in a C++ string is a plain `.`, warning C4129), so any character before `json` matched.
- Tests: GameLibraryTest +5 (signed and `ratedGames`; an edited rating restored; unsigned / not JSON / missing; behind the newest game; `ratedGames` worked out for older records), UserManagerTest +3 (restored from a backup; rebuilt from the kept games of both profiles; version 2 trusted once), the export test's rating verifies on the other PC, the import's `.json` check; `tst_restart` (run 1 also edits `rating.json`; run 2: the rating message, the set-aside one in Recent messages: `app.recentMessages()`, the flyout's objectName `recentMessages`). `TestRun.editRatingFile()`.
- CLAUDE.md, the plan (§3.4 as built), TASKS.md.

#### Checked
- Debug: ChessQtTests 301/301, no warnings; UI tests (Debug) 12 files, 0 failed.

#### Next
- Step 3 (`features/signedExport_…`): an export of format 1 imported unrated; an import checks the signed rating and statistics.

### 2026-11-19 — BL-20 step 3: an import checked, an old export unrated (branch: features/signedExport_191126)

#### Done
- **`GameLibrary::checkImport`**: an imported profile's files checked before `UserManager` writes them: a game file that fails its check → `games/rejected/`; a `rating.json` that fails → `games/rejected/`, the rating restored from the newest verified rated game imported (none: 1200); an export's `games.json` (before BL-20) → signed game files without their rating fields, and no rating (the owner's choice: unrated). `readGameContent` and `parseLegacy` read content, not only files.
- **`UserManager::importProfile`**: the profile's statistics checked (`statsSig`); without one that fits (edited, or an old export) they are rebuilt from the imported games (`statisticsFromGames`). `lastImport()`: `{ setAside, unrated, ratingRestored, statsRebuilt }`; `ChessGame.qml` puts them in the import's message ("Profile imported. It comes from an older Chess++: its games are unrated, …").
- Tests: UserManagerTest +2 (an old export unrated, its statistics rebuilt, the profile loads at 1200 without a message; an edited export: the edited game set aside, the rating restored from the first game, the statistics rebuilt, nothing said again at load); the export round trip finds nothing; the import's file names test: a game name with bad content is set aside.
- CLAUDE.md, the plan (§3.6 as built), TASKS.md.

#### Checked
- Debug: ChessQtTests 303/303, no warnings; UI tests (Debug) 12 files, 0 failed.

#### Next
- Step 4 (`docs/gameFilesManualTests_…`): `BL-20-ManualTests.md` (edit a game, the rating, the statistics by hand; damage a file; export / import between two data folders; an old profile updated).

### 2026-11-19 — BL-20 step 4: the hand tests (branch: docs/gameFilesManualTests_191126)

#### Done
- [`docs/BL-20-ManualTests.md`](docs/BL-20-ManualTests.md): 8 scenarios on throwaway data folders (`--data-dir`): a game edited by hand set aside and said once; a damaged game and one copied under another name; the rating edited, then deleted (restored from the kept games); the statistics edited (from a backup, then rebuilt with no backup); export / import between two data folders (nothing said); an export's statistics edited (rebuilt, "Name (2)"); the owner's own data from before BL-20 updated on a copy (no message, `games.json.migrated`, `users.json` version 3); optional: another Windows account, a reinstall. Each lists the files, the exact messages and what the automated tests already cover.
- TASKS.md (step 4 done; open: the owner's hand tests), the plan's status.

#### Next
- The owner runs `docs/BL-20-ManualTests.md`; BL-20 is done when they pass.

### 2026-11-19 — BL-20: the review of #187–#190 fixed (branch: bugs/bl20ReviewFixes_191126)

#### Done
- **Ways around the check**: `games.json` (BL-19) is migrated only in a profile without files of BL-20 (`GameLibrary::hasSignedFiles`: `rating.json`, a game file, `games.json.migrated`, `games/rejected/`); beside them it is set aside (counted, said). A `users.json` below version 3 isn't trusted when a profile has signed files, and an old one is written as version 3 as soon as it loads. The rating catches up only along the rated games that continue it exactly (`continued`), so a game copied from another profile doesn't move it (the owner chose this over a signature per profile).
- **Rare bugs**: a failing `rating.json` beside a `games.json` that can't be read is restored and said at once (it waited, then went back to 1200 silently); a game file that can't be removed is tried again at the next write; a `games.json` record whose id can't name a file gets a UUIDv5 of it (`fileIdOf`), so a migration tried again doesn't keep it twice; `lichessToken()` marks the link expired at once but saves and emits after the call (no re-entrancy).
- **Cleanup**: `GameLibrary::ReadFailure` instead of comparing the reason's text; `GameSignature::sign/verify/canonical` for JSON objects (one copy); `UserManager` checks game file names with `GameLibrary::isFileId`; `AppConfig::writeFileAtomically` for every file the app keeps; a statistics rebuild reads each profile's games once.
- Tests: GameLibraryTest +5 (a copied rated game, `games.json` beside signed files, a failing rating beside a locked `games.json`, a remove retried, a migration retried), the catch-up across two games, the read failures; UserManagerTest +1 (version 2 beside signed files), the old file signed at load, the token's change after the call.
- CLAUDE.md, the plan (review fixes), the hand tests' "already checked", TASKS.md.

#### Checked
- Debug: ChessQtTests 309/309, no warnings; UI tests (Debug) 12 files, 0 failed.

#### Next
- The owner runs `docs/BL-20-ManualTests.md`; BL-20 is done when they pass.

### 2026-11-19 — BL-20: the hand tests passed, BL-20 done (branch: docs/bl20ManualResults_191126)

#### Done
- [`docs/BL-20-ManualTests.md`](docs/BL-20-ManualTests.md): the owner passed scenarios 1 (a game edited), 2 (damaged, copied under another name), 3 (the rating edited), 5 (export / import), 6 (an export edited) and 7 (the owner's own data from before BL-20).
- Claude ran 1, 2, 3, 4 and 7 on copies of the owner's folders, starting the Debug build as a player would. Each run was checked through a picture of the window, the log and the files. All passed. Scenario 4 (the statistics edited by hand: restored from a backup, then rebuilt, then no message) counts as passed on that run. Scenario 7 ran without its step-2 game.
- Scenario 8 (another Windows account, a reinstall) is optional and wasn't run. TASKS.md: BL-20 done.

#### Notes
- Files set aside are never deleted by the app. The ones missing from the owner's `games\rejected\` were moved or deleted by hand between scenarios (the owner, 2026-11-19). A set-aside game file put back unedited verifies again, as it should.
