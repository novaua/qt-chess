# BL-20 Games library: one signed file per game

| | |
| :---- | :---- |
| **Date** | 2026-11-18 |
| **Status** | Steps 1 (#188) and 2 (#189) done; step 3 (the import checked, old exports unrated) on `features/signedExport_191126` |
| **Branch** | `docs/gameFilesPlan_181126` (plan); each step on its own branch from `develop` |
| **Task** | BL-20 in `TASKS.md` (the owner, 2026-11-18: a game per file instead of one `games.json`; every game signed, so an edited game isn't taken in: a simple anti-cheat and an integrity check) |

## 1. What there is (reviewed 2026-11-18)

- **One file per profile**: `users/<id>/games.json` holds every kept game (`GameRecord`), finished or not,
  and the robot rating (`LocalRating`: value and rated games, stored as it goes, BL-19 step 5).
- **Rewritten whole on every change.** `GameLibrary::put` (every move, takeback, result, leave) and `remove`
  write the whole file once the current event is done (`QSaveFile`). With the default 100 finished games and
  their analyses it is a few hundred KB per move.
- **Damage**: a `games.json` that can't be read is kept aside (`.damaged`) and never written over. A single bad
  record has no such handling: LI-01A's review fixes (#185) keep an unreadable Chess960 start as read.
- **Nothing is checked.** Anyone can edit `games.json`: change a result, the rating, a game's moves. The rating
  vs the robot can be set to any number.
- **Statistics** (per opponent: games, won, drawn, lost) live in `users.json` (`UserManager::recordResult`),
  three backups (`backups/users-1..3.json`). They count more than the kept games: games trimmed by "Games kept",
  Lichess games (no record, BL-19's owner choice), and the "earlier" row from before results were kept.
- **Export / import** (`.chessprofile`): `UserManager` puts `games.json` in with the profile (`kProfileFiles`),
  never the Lichess token; an import migrates the old binary saves.
- QML and `ChessConnector` see only `GameLibrary`'s API (`games`, `put`, `remove`, `find`, `rating`, …).

## 2. The owner's decisions (2026-11-18)

1. **One file per game** (`users/<id>/games/<gameId>.json`): a move writes only its game's small file.
2. **Every game is signed**: HMAC-SHA256 (`QMessageAuthenticationCode`) of its content with a key built into
   the app. A plain hash would catch only damage: anyone could recompute it after an edit.
3. **A game that fails its check is set aside**: moved to `users/<id>/games/rejected/`, not loaded, never
   counted; a message says how many. (Damage and editing look the same; the file is kept for a look.)
4. **The rating and the statistics are signed too**, so they can't be edited directly. On a mismatch they are
   recovered, not trusted (§3.4).
5. **One app-wide key**: the same in every copy of Chess++, so an exported profile's games verify on another PC.
   A determined user could find the key in the program: this stops editing a file by hand, not a programmer.

6. **Recover, not recompute** (2026-11-18): the rating and the statistics can't be fully *recomputed* from the
   kept games (§1: trimmed games, Lichess games, the "earlier" row). §3.4 recovers them from what is signed:
   the rating from the newest verified rated game, the statistics from the newest backup that verifies, and
   only as a last resort rebuilt from the kept games (Lichess and older counts lost then).
7. **An export from before BL-20 imports unrated** (2026-11-18, §3.6): its games come in, but not its rating.

## 3. Design

### 3.1 The files

```
users/<id>/games/<gameId>.json      {"record": {…GameRecord…}, "sig": "1:<64 hex>"}
users/<id>/games/rejected/…         set aside: failed the check, or unreadable
users/<id>/rating.json              {"rating": {"value": 1227, "games": 2}, "sig": "…"}
```

- `<gameId>` is the record's id (already unique); the file name must match the id inside, so a game can't be
  copied under another name.
- Written with `QSaveFile`, one file per `put`; `remove` and "Games kept" delete files.
- The list is the folder: read once when the profile becomes active, newest first by `startedAt`, as now.
- `games.json` goes: migrated once (§3.5).

### 3.2 The signature: `GameSignature`

A small class in ChessGui (`gamesignature.h/cpp`, C++ style as CLAUDE.md asks):
- `static QString sign(const QByteArray& payload)`; `static bool verify(const QByteArray& payload, const QString& sig)`
  (constant-time compare).
- The payload is the record's **canonical JSON**: `QJsonDocument(record).toJson(Compact)`. `QJsonObject` keeps its
  keys sorted and Qt writes numbers in the shortest form that reads back the same, so reading a file and writing
  the record again gives the same bytes; a test guards it (the analysis's doubles included).
- A domain prefix per kind (`"game\n"`, `"rating\n"`, `"stats\n"`) so one kind's signature never fits another's.
- The key: 32 bytes in the source, not as one plain string (put together at run time): it keeps a `strings`
  search from finding it, nothing more (§2.5).
- **The key never changes, and each signature names its key**: `"<key number>:<hex>"`, key 1 to begin with. A
  new key, if one is ever needed, is added beside the old ones: a file signed with any known key verifies, and
  is signed again with the newest key when it is next written. A file signed with a key number this copy
  doesn't know (made by a newer Chess++) is set aside like a failing one, and the message says to update.
  Changing key 1 itself would set aside every kept game after an update: never done.

### 3.3 Loading and writing games

- `GameLibrary` reads each file: unreadable JSON, a bad signature, or an id that differs from the file name →
  the file moves to `rejected/`, the count of set-aside games grows. After loading, if any were set aside:
  `GameLibrary.rejectedCount` and a signal; `ChessGame.qml` shows "2 games couldn't be verified and were set
  aside." once (InfoBar, caution).
- `put` signs and writes that one record; nothing else is written. The once-per-event write stays (the record
  as it is when the event is done).
- A Chess960 record whose start can't be read (#185) is signed as it is: it stays unplayable, not rejected.
- API unchanged for QML and `ChessConnector`.

### 3.4 Rating and statistics

- **Rating** (`rating.json`): signed. Each rated record also gets `ratedGames` (the count after it) next to
  `ratingBefore` / `ratingChange`, so the newest verified rated record gives the whole rating:
  `ratingBefore + ratingChange` and `ratedGames`. On a missing or failing `rating.json`: that, or the start
  rating (1200, 0 games) when no rated game is kept; a message says the rating was restored.
- **Statistics** (`users.json`): each user's `statistics` array gets a `statsSig`. On a mismatch: the newest of
  `backups/users-1..3.json` whose statistics for that user verify; if none does, the rows rebuilt from the
  verified kept games (vs Robot by level, two-player opponents), Lichess and "earlier" rows dropped, and a message.
  `UserManager::recordResult` signs as it writes.
- A file from before BL-20 (no signature) is trusted once and signed (§3.5); after that, unsigned = failing.
- **As built (step 2):** `rating.json` has no "before BL-20" form (it came with step 1, never released), so one
  without a signature fails like an edited one; games.json's rating is trusted once, as its games, by the
  migration, which also works out `ratedGames` along the rated records. A failing `rating.json` is moved to
  `games/rejected/`; a missing one while rated games are kept is restored the same way; one that verifies but
  is behind the newest rated game (a crash between the two writes) catches up without a message. A record rated
  before step 2 has no `ratedGames`: the rated games kept up to it are counted.
- **As built (step 2):** "before BL-20" for `users.json` is its `version` below 3 (version 3 signs). A player who
  edits the version back and drops the signatures gets the statistics trusted once: a deliberate edit of the
  format, beyond editing a number (§2.5: a deterrent). The rebuild counts the kept games the statistics had
  (`resultRecorded`), and a profile's two-player games kept by the other profile (the one signed in keeps
  them). The window says it: "Alice's statistics couldn't be verified and were restored from a backup." /
  "… rebuilt from the kept games (Lichess and older games aren't in them)." An import checks its own (§3.6).

### 3.5 Migration (once per profile)

- `games.json` (unsigned) → one signed file per record, the rating → `rating.json`, `ratedGames` worked out
  along the rated records, oldest first. `games.json` is then renamed `games.json.migrated` (kept, never read
  again). A `games.json` that can't be read stays as today (`.damaged`), nothing migrated from it.
- `users.json` statistics without `statsSig`: signed as they are.
- Trusting what is there once is the only way: the old files were never signed. (A player could edit
  `games.json` before updating; nothing to do about that.)

### 3.6 Export and import

- `.chessprofile` carries the `games/` files and `rating.json` as they are (signed), not `games.json`; a new
  format version. Import verifies each: failing ones go to `rejected/` in the imported profile, with the message.
- An export from before BL-20 (with `games.json`): **imported unrated** (§2.7): its games come in and are
  signed, but their rating fields are dropped and the profile's rating starts at 1200. Trusting it would be a
  way around the check (edit an old-style export, import it).
- **As built (step 3):** the export format stays 2 (step 2's exports already carry the signed rating and
  `statsSig`). `GameLibrary::checkImport` checks the files before `UserManager` writes them, so nothing that
  failed is ever loaded and nothing is said twice: a failing game file → `games/rejected/`; a failing
  `rating.json` → `games/rejected/`, and the rating restored from the newest verified rated game among the
  imported ones (none: 1200); `games.json` → signed game files without `rated`, `ratingBefore`,
  `ratingChange`, `ratedGames`, and no `rating.json` (also when it came with one). Statistics without a
  signature that fits (edited, or an export from before BL-20) are rebuilt from the imported games, so an old
  export's Lichess and "earlier" rows are lost: the same reasoning as its rating (a way around the check
  otherwise). One message after the import names what was found: "Profile imported. It comes from an older
  Chess++: its games are unrated, its rating starts at 1200 and its statistics count only its kept games." /
  "… 1 game couldn't be verified and was set aside." / "Its rating couldn't be verified and was restored from
  its games." / "Its statistics couldn't be verified and were rebuilt from its kept games."

### 3.7 Moving a profile: reinstall, another PC, another Windows account

The signatures don't tie a game to a PC, a Windows account or an installation (§2.5: one app-wide key), so:
- **A reinstall** keeps every game valid. Uninstalling asks whether to delete the user data (the default is
  Yes; a silent uninstall keeps it unless `/DELETEDATA=1`); data that is kept is read as before.
- **Export / import** (`.chessprofile`, §3.6) is the supported way to another PC: games, rating and statistics
  verify there. The Lichess sign-in is never exported: the player links Lichess again.
- **A copied data folder** (`%LOCALAPPDATA%\ChessPlusPlus_Inc` to the same place on the new PC, or to another
  Windows account) verifies too. Its Lichess sign-in is encrypted for the old Windows account
  (`CryptProtectData`) and can't be read there: the profile shows "reconnect" (a bug fix outside BL-20,
  `bugs/lichessTokenUndecryptable_181126`: before it, the profile said Connected and nothing worked).

## 4. Steps

Each step is one branch and one PR from the latest `origin/develop`.

| Step | Branch | What |
|---|---|---|
| 1 | `features/gameFiles_…` | §3.1–3.3, §3.5 (games): `GameSignature`, a file per game, set aside, migration from `games.json`; export / import of the game files and `rating.json` (export format 2; moved here from step 3 so no export loses games in between); `GameLibraryTest`, `GameSignatureTest`, UserManagerTest +2, `tst_restart` (an edited game set aside at the next start) |
| 2 | `features/signedRating_…` | §3.4: `rating.json`, `ratedGames` on rated records, the statistics' `statsSig`, recovery from the newest rated game / the backups; UserManagerTest, GameLibraryTest |
| 3 | `features/signedExport_…` | §3.6: an export of format 1 imported unrated; the signed rating and statistics in an export; UserManagerTest |
| 4 | `docs/gameFilesManualTests_…` | CLAUDE.md, `BL-20-ManualTests.md` (edit a game, the rating, the statistics by hand; damage a file; export / import between two data folders; an old profile updated), `TASKS.md`, `PROGRESS.md` |

## 5. Tests

**ChessQtTests:**
- `GameSignatureTest` (new): sign / verify; a changed byte fails; another domain's signature fails; the
  canonical JSON round trip (a record with an analysis, Chess960, a fork) gives the same bytes; a signature with
  an older key number verifies (a test-only second key) and is re-signed with the newest on write; an unknown
  key number fails.
- `GameLibraryTest`: a file per game, only the changed one written; an edited result, moves or rating field →
  set aside, the others load; a file renamed to another id → set aside; unreadable JSON → set aside; the
  migration from a `games.json` (rating, `ratedGames`, `.migrated` kept); "Games kept" deletes files; profile
  switch reads the other folder.
- Rating: `rating.json` edited → restored from the newest rated game; none kept → 1200.
- `UserManagerTest`: statistics edited → the newest verifying backup; none → rebuilt from the games; an export /
  import round trip verifies; an edited game in an export is set aside on import; an old-format export migrates.

**UI** (`tst_games`): the "set aside" message after a test edits a file on disk between two runs
(`// ui-test-runs: 2`).

**By hand** (step 4): the edits a player might try, in a text editor; an old profile updated in place; the
data folder copied to another Windows account (games verify, Lichess asks to reconnect); a reinstall that
keeps the data.

## 6. Risks

- **The canonical JSON** must give the same bytes on every read and write; if Qt's number output changed between
  versions, every game would fail its check after an upgrade. Guarded by the round-trip test, and a failure is
  never a loss: the file is set aside, not deleted. (Alternative if it ever bites: sign the file's exact bytes,
  the record kept as a string.)
- **Many small files**: 100–200 files per profile, read once per profile switch; fast on any disk. Antivirus
  scanning of many writes: one small file per move instead of one large one is less, not more.
- **A crash between writes**: each file is written whole (`QSaveFile`); the rating file and its game are two
  writes. The rating's recovery (§3.4) covers a rating file older than its newest rated game.
- **The key in the program**: by design (§2.5), a deterrent, not security.
- **Losing the key**: a change to key 1 would set aside every game; the key number (§3.2) lets a new key come
  in without that, and a test pins key 1's signature of a fixed record so an accidental change fails at once.
