# LI-16 — After a Lichess game: New opponent: plan

The ticket: [LI-16](../.tasks/open/LI-16-after-a-lichess-game-rematch-new-opponent-analyse.md). When a Lichess game
ends, lichess.org offers Rematch, New opponent and Analyse. Chess++ has Rematch, New game, Review moves and, since
LI-14, Analyse. This plan adds **New opponent**: a game from Quick pairing (LI-09) seeks again at once with the
same clock, variant and Rated / Casual. Line numbers are from `develop` at ebf4652.

## What is there today

- **The result card** (`ResultCard.qml`): Rematch (`canRematch`), New game, Review moves, Analyse (`canAnalyse`,
  LI-14: done). A `GridLayout`: three buttons in a row, four as two rows of two, one column under 400 px. Once the
  card is closed, the moves panel shows Rematch and New game again (`MovesPanel.qml:122-140`).
- **New game** for a Lichess game: `GamePage.qml:530` `page.newGameRequested()` → `ChessGame.newGameLikeThis()`
  → `leaveFlow.leave(); playOnline()`: the game is left and the Online page opens. Ctrl+N on a finished game does
  the same (`ChessGame.qml:888-891`).
- **The seek** (`LichessClient`, LI-09): `startSeek(limitSec, incrementSec, days, rated, variant)`, `seek` /
  `seeking`, `cancelSeek()`; the `SeekIndicator` in every command bar shows it, with Cancel. Its game is matched in
  the event stream (`LichessClient.cpp:691-697`, `isSeekGame`) or in Your games (`findSeekGame`, `:1038`). Both
  call `onGameStartDetected` → `streamGame` → `endSeek`. So once the game has started, nothing says it came from a
  seek.
- **The finished game's facts:** `RematchInfo _lastGame` (`LichessClient.h:45`; `parseRematchInfo` at `:1796`,
  set from `gameFull` at `:1303`) keeps the clock, days per move, variant and start FEN for `createRematch()`. It
  doesn't keep `rated`, nor where the game came from. `stopAll()` (`:1837`) leaves it as it is.
- **One game at a time:** `startSeek` refuses while a real-time challenge of ours waits (`challengeWaiting()`,
  `:1089`). A rematch challenge waiting is one. `ChessConnector` knows its id (`_rematchChallengeId`, BL-36) and
  `endGame()` withdraws it.

## 1. Where the game came from (LiChessConnector)

- `RematchInfo` gets `bool rated` (from `gameFull`) and `bool fromSeek`.
- When a `gameStart` matches the seek (both places above), `LichessClient` keeps its id (`_seekGameId`) before
  `onGameStartDetected` ends the seek. At `:1303`, `_lastGame.fromSeek = _currentGameId == _seekGameId`.
- **Only a real-time seek.** A correspondence seek is left on Lichess (`seekSent`) and its game isn't matched. A
  seek's game rejoined later (Continue, a restart) isn't known as one either. Both get New game (decided, 2).
- New in `LichessClient`:
  - `bool canSeekAgain() const`: `_lastGame.valid && _lastGame.fromSeek`. It changes with `canRematchChanged`.
  - `void seekAgain()`: `startSeek(_lastGame.clockLimitSec, _lastGame.clockIncrementSec, 0, _lastGame.rated,
    _lastGame.variant)`. A Chess960 seek gets a new position, as on lichess.org (no FEN in a seek).
- Not for an AI game: `parseRematchInfo` leaves `valid` false for one, and the Board API's seek never pairs one.

## 2. One call: `ChessConnector.newOpponent()` (root rule 1)

The card's button leaves the game's rematch and starts a seek: a flow of the game with Lichess, so one
`ChessConnector` call, beside `rematch()`.

- `Q_PROPERTY(bool CanSeekAgain …)`: the game is over, it was a Lichess game (`_lastGameOnline`), and
  `LichessClient::canSeekAgain()`. Notified with `canRematchChanged` and the game's end.
- `Q_INVOKABLE void newOpponent()`:
  1. Withdraw our rematch challenge if one waits. This is `endGame()`'s code, moved into a private
     `withdrawRematch()` that both use. `cancelChallenge` → `stopStream` → `stopAll` clears `_openChallengeReply`,
     `_waitForGameId` and the direct challenge at once, so `challengeWaiting()` is false before the seek.
  2. `_lichessClient->seekAgain()`.
  3. The finished game stays on the board. Its review and Analyse still work while the seek waits.
- When Lichess pairs the seek, `onLichessGameStarted` replaces the finished game on the same page, as a Rematch
  does. The finished game is kept already (LI-14).
- If the seek is refused or ends unpaired, the window shows its message ("Quick pairing: …", `onSeekFailed`),
  since the page isn't `online`. If the player leaves for Home meanwhile, the seek goes on, as one left behind on
  the Online page does.
- The opponent's rematch offer has its own bar. Accepting it ends the seek (`acceptChallenge` already does).

## 3. The result card and the moves panel (QML)

- `ResultCard`: `property bool canSeekAgain`, `signal newOpponentRequested()`. With it, New game's place holds a
  `BusyButton` **New opponent** (objectName `newOpponentButton`, icon `play`).
  - It is busy while `LichessClient.seeking`: "Looking for an opponent…", and it can't be clicked. Cancel is the
    command bar's `SeekIndicator`.
  - Without `canSeekAgain`: New game, as today.
- At most four buttons, as now: Rematch, New opponent or New game, Review moves, Analyse. The two-by-two layout
  fits them, so Review moves stays on the card. In a narrow window (under 400 px) they stack in one column, as now.
- Labels as lichess.org's: "Rematch", "New opponent".
- `MovesPanel` (the card closed): the same swap in its row.
- `GamePage`: `canSeekAgain: ChessConnector.CanSeekAgain`, `onNewOpponentRequested: ChessConnector.newOpponent()`.
- Ctrl+N on a finished game: New opponent when `CanSeekAgain`, else New game as today.
- Docs: `qml/CLAUDE.md` (GamePage, ResultCard), `.claude/rules/chessgui-cpp.md` (the Rematch bullet),
  `LiChessConnector/CLAUDE.md` (Quick pairing).

## 4. Tests

- **`tst_lichess`:**
  - A seek is paired (`FakeLichess.pairSeek`) and its game ends (`FakeLichess.endGame`). The card shows
    `newOpponentButton`, not `newGameButton`.
  - A click sends `POST /api/board/seek` again with the first seek's body (`FakeLichess.bodyOf`). The button is
    busy and the `SeekIndicator` shows. `pairSeek` again: the new game is on the same page.
  - A challenge's game ends: `newGameButton`, as before.
  - A rematch challenge waits (`LichessClient.challengeCreated`, as test 19). New opponent cancels it
    (`POST /api/challenge/<id>/cancel`), and the seek is sent, not refused.
- **ChessQtTests** (`LichessSeekTest.cpp`): `canSeekAgain` is true after a seek's game, and false after a
  challenge's game or a rejoin. `seekAgain()` sends the same body.
- **Screenshot scene `lichess-new-opponent`:** the `lichess-result` scene fed as a seek's game (main.cpp's feed:
  an offline `startSeek`, then a lobby `gameStart`), its card with New opponent. Baselines for it and for
  `lichess-result`.
- **Hand tests**, `docs/LI-16-ManualTests.md` (written in step 1): on lichess.org, a Rapid seek's game ended,
  New opponent, paired again; Cancel from the command bar; a friend's game shows New game.

## Steps (one PR each)

1. LichessClient (`fromSeek`, `rated`, `canSeekAgain`, `seekAgain`), `ChessConnector.newOpponent()`, the result
   card and the moves panel, with the tests and the hand-test list.
2. Only if the hand tests on lichess.org find something to change (e.g. a seek's `gameStart` with another
   `source`): a fix PR.

## Related tickets

- LI-09 (Quick pairing): the seek, its indicator and its hand tests.
- LI-14 (Analyse a Lichess game): Analyse on the card, done.
- BL-36: `ChessConnector` withdraws our waiting rematch challenge.
- LI-15 (recent Lichess games): unrelated to the card.

## Decided (the owner, 2026-11-26)

1. **New opponent waits on the finished game**, not on the Online page. The game page stays, the seek shows in
   its command bar, and the paired game replaces the board.
2. **A game not from Quick pairing keeps New game**, which opens the Online page: a challenge, a link, a
   correspondence game, a seek's game rejoined later.
