# LI-11 Flair and a Standard Avatar for Lichess Players — Implementation Plan

| | |
| :---- | :---- |
| **Date** | 2026-11-22 |
| **Status** | Approved by the owner, 2026-11-22; done (LI-11 archived) |
| **Branch** | `features/lichessFlair_221126` |
| **Task** | [`LI-11`](../.tasks/archive/LI-11-flair-beside-a-lichess-name.md) |

## 1. Context

In a Lichess game the opponent's card shows `unicorn` from the app's own avatar library
(`chessconnector.cpp`, `onLichessGameStarted`: `setOpponentFromUser(... "unicorn")`). The owner played real
Lichess players in LI-09's hand tests and decided (2026-11-22):

- **Opponent card:** the opponent's Lichess **flair** is the avatar. With no flair (or before it loads, offline,
  or if it fails), a new **standard avatar: a pawn in front of a small globe**. It is never offered for local
  profiles.
- **Own card:** keeps the local avatar (LI-07's decision). The player's own flair shows **small after the
  Lichess name**, as lichess.org does.
- **Qt Image Formats** (`qwebp.dll`) is added now, because Lichess serves flair images only as WebP.

What was checked:
- The flair is a string on the user object (`/api/user/{name}`, `/api/users`), not in the game stream's
  `gameFull` (API spec `GameEventPlayer`).
- The image: `https://lichess1.org/assets/flair/img/<flair>.webp` answers 200 with `image/webp` (checked live
  2026-11-22). The unversioned path works, so lila's `______5` asset version isn't needed.
- The Qt Image Formats add-on is installed locally: `plugins\imageformats\qwebp.dll` and `qwebpd.dll`.

## 2. Changes

### Data: `LiChessConnector/LichessClient.h/.cpp`
- `LichessPlayer`: `_id` (from `fromJson`) and `_flairUrl`, with `id()`, `flairUrl()` and `setFlairUrl()`.
- `LichessClient`:
  - `_assetBaseUrl` (default `https://lichess1.org`), `setAssetBaseUrl()` for the fakes, and
    `flairUrl(flair)` = `<asset base>/assets/flair/img/<flair>.webp`. The flair name is checked against
    `[a-z0-9.-]+`, so a URL can't be built from anything else.
  - After a **new** game's `gameStarted` (not a resync, and not when the game was refused: the `_stops`
    check), `fetchPlayerFlairs()`:
    - `POST /api/users`, body `id1,id2` (`text/plain`), with only the players that have an id (the AI has none).
    - The reply is kept in `_flairReply` with the game id captured (the `refreshOngoingGames` pattern:
      `deleteLater`, drop it if `reply != _flairReply` or the game id ≠ `_currentGameId`), plus a 10 s transfer
      timeout.
    - It parses the array, matches `id` to `_white` and `_black`, sets each `flairUrl`, and emits a new signal
      `gamePlayersChanged()`.
    - Errors are only logged: the flair is decoration. `stopAll` aborts it.

### `ChessGui/ChessGui/chessconnector.h/.cpp`
- Connect `LichessClient::gamePlayersChanged`: when it's the online game on the board, refresh `_lichessMe` /
  `_lichessOpponent` from `gamePlayer()` (they are copies), set the opponent's avatar (below), and
  `emit gameInfoChanged()`.
- New `Q_PROPERTY(QString PlayerFlair …)`: the player's flair URL in a Lichess game, else `""`
  (`NOTIFY gameInfoChanged`).
- `onLichessGameStarted`: `_avatarProvider->setOpponentOnline(_lichessOpponent.flairUrl())` instead of
  `unicorn`. That's `""` at start: the standard avatar.

### `ChessGui/ChessGui/AvatarProvider.h/.cpp`
- `setOpponentOnline(const QString& flairUrl)`: the opponent is a Lichess player. `_opponentName =
  "online-player"`, `_opponentImageUrl = flairUrl`. It clears `_isComputerGame` and emits.
- `opponentUrl()`: `_opponentImageUrl` when set, else the qrc path as now. The other `set…` / `restore` /
  `randomize` clear `_opponentImageUrl`.
- New `Q_PROPERTY opponentFallbackUrl`: the qrc path of `_opponentName`, shown when `opponentUrl` doesn't load.
- `online-player` is **not** in `avatarNames()`, so the picker never offers it. Lichess games have no
  `GameRecord`, so nothing saves a web URL.

### The avatar image
- `scripts/make_icons.py`: `online_player_avatar()` draws `ChessGui/ChessGui/pics/avatars/online-player.png`.
  It's 618×618 in the avatars' style (thick black outlines, flat colours, white): a blue globe with meridians
  and parallels behind, a white pawn in front.
- In `qml.qrc` (`pics/avatars/online-player.png`), `ChessGui.vcxproj` and `.filters`, as the other avatars are.

### QML
- `PlayerCard.qml`:
  - `property string avatarFallback`: the Avatar's `Image` shows it when its status is `Image.Error` (offline,
    a missing flair), and goes back to `avatarUrl` when that changes.
  - `property string flair`: a 16 px `Image` after the name (`objectName: "playerFlair"`, shown only once
    loaded, `sourceSize` 32), in `NameRow`.
- `GamePage.qml` `SideCard`:
  - `avatarFallback: isPlayer ? "" : AvatarProvider.opponentFallbackUrl`
  - `flair: isPlayer ? ChessConnector.PlayerFlair : ""`. The opponent's flair is the avatar.

### CI, installer, docs
- `.github/workflows/tests.yml` and `release.yml`: `modules: qtmultimedia qtimageformats`. There's no Qt
  cache key to bump, and the pinned `aqtsource` stays.
- Installer: `windeployqt` (`installer/Build-Installer.ps1`, `release.yml`) copies the plugins found in Qt's
  `plugins\imageformats`, so `qwebp.dll` comes along. Checked locally with `windeployqt` into a scratch
  folder. `installer/Test-Installer.ps1` requires `imageformats\qwebp.dll` (next to
  `tls\qschannelbackend.dll`).
- `CLAUDE.md` and `README.md`: "with the Qt Multimedia and Qt Image Formats add-ons". `CLAUDE.md` also covers
  LI-11 in the LichessClient and ChessConnector text.
- BL-12: a note to keep `imageformats\qwebp.dll` when trimming the deployment.

## 3. Tests

- **`ChessQtTests/LichessClientTest.cpp`**, against `FakeLichess` (`setBaseUrl`, `setAssetBaseUrl`, a token):
  - A `gameFull` with two ids: one `POST /api/users` with body `me,friend`. The answer's flairs land in
    `gamePlayer().flairUrl()` as `<asset>/assets/flair/img/<flair>.webp`, and `gamePlayersChanged` fires.
  - A resync `gameFull`: no second request.
  - The AI (no id) isn't asked for.
  - A reply that arrives after `stopStream()` or for another game is dropped.
  - A flair name with `/` or `..` gives no URL.
- **`ChessGuiTests/fakelichessserver`:**
  - `setPlayer(name, title, rating, provisional, flair = "")`.
  - `POST /api/users` answers the listed ids with their flairs.
  - `GET /assets/flair/img/<f>.webp` serves a 16×16 WebP made with `QImage::save(…, "WEBP")`. That save also
    proves the plugin is there in the test run (CI included).
  - `TestSetup` sets the client's asset base to the fake.
- **`tst_lichess`:**
  - test_03: FriendlyRival has a flair, so the opponent card's avatar `Image` becomes the fake's flair URL,
    `Ready`. TesterChess has one too: `playerFlair` shows on the own card.
  - test_09 (SlowPlayer, no flair): the opponent card shows `online-player.png`.
- **Screenshot `clock-online`:** offline, so no flair; it shows the pawn-and-globe avatar. Looked at narrow and
  wide.

## 4. Verification

1. `python scripts/make_icons.py`, then look at `online-player.png`.
2. Build the solution in Debug and Release (MSBuild, Qt `bin` on `PATH`).
3. ChessQtTests: the Lichess tests, then all of them.
4. `scripts\Run-UiTests.ps1`: every file, `tst_lichess` with the new checks.
5. `Take-Screenshot.ps1 -Scene clock-online, clock-online@1400x800 -Offscreen`, and look at them.
6. `windeployqt` into a scratch folder: `imageformats\qwebp.dll` is there.
7. LI-11: `## Resolution`, `status: done`, `git mv` to `archive/`; `PROGRESS.md`; the PR into `develop`.
