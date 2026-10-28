# Lichess Rematch — Implementation Plan

| | |
| :---- | :---- |
| **Date** | 2026-10-28 |
| **Status** | Planned |
| **Branch** | new branch from `develop` |

## 1. Problem

After a game against a Lichess opponent there is no way to play again. Two things block it:

1. The app hides the button. `MoveHistoryPanel.qml` shows **Rematch** only when
   `!panel.wasOnlineGame`, and `ChessConnector::rematch()` only restarts local games
   (vs Robot or two players on one device).
2. Lichess has no rematch endpoint in the Board API, the API the app plays through. Its game endpoints
   cover move, resign, draw, takeback and abort only. The native **Rematch** button on lichess.org works
   over the website's own websocket, and the public docs don't show whether a Board API client ever
   receives that offer.

## 2. Approach

Keep the existing **Rematch** button and make it work for online games by **creating a new
challenge** with the same settings:

| Last game | Rematch does |
|---|---|
| vs Robot | Unchanged: new game at the same level |
| Two players, one device | Unchanged: new local game |
| Lichess, **registered** opponent | Direct challenge to that user (`POST /api/challenge/{username}`). They get a Lichess notification and accept it; the app starts the game automatically. No link needed. |
| Lichess, **anonymous** opponent | New open challenge (`POST /api/challenge/open`); the existing link/QR dialog opens so the user can send the new link. |

Settings carried over from the finished game:
- **Time control**: same `clock.limit` / `clock.increment`, or correspondence if the game had no clock.
- **Variant**: same (`standard` / `chess960`).
- **Colors**: swapped. Our new color is the opposite of the last game's, as usual for a rematch.
- **Rated**: `false`, as today.

## 3. Design

### 3.1 LichessClient (`LiChessConnector/`)

Remember the last game's settings from the `gameFull` stream line, which works whether we created
the challenge or joined someone else's:

```cpp
struct RematchInfo {
    QString opponentId;     // Lichess user id; empty = anonymous
    QString opponentName;
    bool    playedAsWhite = true;
    int     clockLimitSec = 0;   // 0 = correspondence / no clock
    int     clockIncrementSec = 0;
    QString variant = "standard";
    bool    isValid() const;
};

// Pure function, unit-testable without network.
static RematchInfo parseRematchInfo(const QJsonObject& gameFull, const QString& username);
```

- `gameFull` fields: `white` / `black` objects (`id`, `name`; an anonymous side has no `id`),
  `clock.initial` / `clock.increment` (milliseconds), `variant.key`.
- Store it in `_lastGame` in `handleStreamData()` on `gameFull`. Do **not** clear it in
  `stopStream()`, because the game has ended by the time Rematch is clicked.

New API:

```cpp
Q_INVOKABLE bool canRematch() const;      // _lastGame.isValid()
Q_INVOKABLE void createRematch();         // direct or open challenge, see below
QString lastOpponentName() const;
```

`createRematch()`:
- **Registered opponent**: `POST /api/challenge/{opponentId}` with `clock.limit`,
  `clock.increment` (or `days` for correspondence), `variant`, `color` = opposite of
  `playedAsWhite`, `rated=false`. Response contains the challenge `id` (it becomes the game id).
  Emit **`directChallengeSent(gameId, opponentName)`** and start waiting with the event stream
  (see below).
- **Anonymous opponent**: reuse `createOpenChallenge(minutes, increment, color, variant)`. It
  already emits `challengeCreated(gameId, joinUrl)` and calls `waitForGameStart()`.

Waiting for a direct challenge:
- A direct challenge **expires after 20 s** unless it is created with `keepAliveStream=true`.
  With it, the POST response stays open as an ndjson stream: the first line is the challenge, and
  the challenge lives until the connection closes. When it ends, the server sends
  `{"done":"accepted"|"declined"|"canceled"}`. The client keeps that reply open
  (`_directChallengeReply`); closing it, or `cancelChallenge()`, cancels the challenge.
  - `declined`: emit **`challengeDeclined(opponentName)`**, stop waiting.
  - `canceled` (server side, e.g. expired): emit the existing `challengeCanceled()`.
  - `accepted`: probe the game stream right away.
- `waitForGameStart()` currently POSTs `/accept` on the challenge so the creator is registered for
  the Board API. That is needed for open challenges, but for a direct challenge we are the challenger and can't
  accept our own. It now takes a flag, `waitForGameStart(gameId, bool registerAsPlayer = true)`, and
  direct challenges pass `false`. The existing event stream + probe then detect the game start and
  call `streamGame()`, which emits `gameStarted`.

### 3.2 ChessConnector (`ChessGui/ChessGui/chessconnector.*`)

- Track `_wasOnlineGame` in C++ (set in `startOnlineGame`, cleared by local starts) instead of
  relying only on the QML flag.
- `rematch()`: if the last game was online, call `_lichessClient->createRematch()`. Otherwise keep the current
  behavior.
- Expose `Q_PROPERTY(bool CanRematch …)` so QML can hide the button when an online
  rematch is impossible (e.g. no settings captured, not logged in to Lichess).

### 3.3 QML

- **`MoveHistoryPanel.qml`**: rematch bar `visible: chessConnector.GameResult !== "" &&
  chessConnector.CanRematch` (drop the `!wasOnlineGame` condition).
- **`ChallengeDialog.qml`**: add a direct-challenge mode:
  - `property string opponentName: ""`. When set, hide the link field, Copy button and QR code
    and show *"Rematch challenge sent to **name**. Waiting for them to accept…"*.
  - Cancel button unchanged (`lichessClient.cancelChallenge(id)`).
- **`ChessGame.qml`** (`Connections { target: lichessClient }`):
  - `onDirectChallengeSent(gameId, name)`: set `_pendingChallengeId`, the opponent name and
    `_showChallengeDialog = true`; save it with `userManager.savePendingChallenge` like open
    challenges.
  - `onChallengeDeclined(reason)`: `clearPendingChallenge()`, show
    `networkErrorBanner.show(name + " declined the rematch")`.
  - `onChallengeCreated` / `onGameStarted` / `onChallengeCanceled`: unchanged. They already
    handle the anonymous path and game start.
  - The challenge dialog opens over the finished board. On cancel or decline the user stays on the
    result screen.

## 4. Implementation steps

1. `LichessClient`: `RematchInfo`, `parseRematchInfo()`, store `_lastGame` on `gameFull`.
   Unit tests (3 below).
2. `LichessClient`: `createRematch()` for both paths, `waitForGameStart(…, registerAsPlayer)`,
   `challengeDeclined` / `challengeCanceled` event handling, new signals.
3. `ChessConnector`: `_wasOnlineGame`, `CanRematch`, `rematch()` routing.
4. QML: rematch bar visibility, `ChallengeDialog` direct mode, `ChessGame.qml` handlers.
5. Manual test against Lichess (5 below). Build both configurations.

## 5. Testing

**Unit tests** (`ChessQtTests/LichessClientTest.cpp`, no network):
- `parseRematchInfo` with a registered opponent, playing white: id/name, swapped color, clock in seconds.
- `parseRematchInfo` with an anonymous opponent (side without `id`): `opponentId` empty.
- `parseRematchInfo` with a correspondence game (no `clock`): `clockLimitSec == 0`.

**Manual tests** (two Lichess accounts, plus a browser without login):

| # | Scenario | Expected |
|---|---|---|
| 1 | Registered opponent accepts | Game starts in the app with colors swapped and the same time control |
| 2 | Registered opponent declines | Banner "… declined the rematch", dialog closes, result screen stays |
| 3 | Cancel while waiting | Challenge canceled on Lichess, dialog closes |
| 4 | Anonymous opponent | Link/QR dialog with a new link; game starts once they open it |
| 5 | Rematch after a vs-Robot game | Unchanged local behavior |
| 6 | App restarted while a rematch challenge is pending | Pending challenge restored like an open challenge today |

## 6. Risks and open questions

- **Opponent clicks Rematch in their browser.** We probably never see that offer through the
  Board API. Their click won't start a game, and the app has to be the side that sends the
  challenge. Check this in manual testing and, if confirmed, mention it in the UI tooltip.
- **Direct-challenge restrictions.** The opponent's Lichess settings may block challenges (e.g.
  "only friends", rating range). The API then returns an error, which we show via `networkError`.
- **Challenge expiry.** Without `keepAliveStream` a direct challenge expires after 20 s; with it the
  challenge waits as long as the app keeps the request open.
- **Board API time-control limits.** The Board API rejects very fast clocks for some accounts.
  This is not new, but a rematch can hit it too; the error is shown as today.
