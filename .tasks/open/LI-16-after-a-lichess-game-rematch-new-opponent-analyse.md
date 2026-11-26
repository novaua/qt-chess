---
id: LI-16
title: "After a Lichess game: Rematch, New opponent, Analyse"
type: task
status: in_progress
priority: medium
---

# LI-16 After a Lichess game: Rematch, New opponent, Analyse

The owner (2026-11-22, after Rapid games on Lichess): "When the online game ends Lichess shows Rematch, find a new partner and Analyse buttons."

**Today:** the result card (`ResultCard.qml`) of a Lichess game shows **Rematch**, **New game** and **Review moves**.
- **New game** leaves the game and opens the Online page (`ChessGame.newGameLikeThis`: `leaveGame(); playOnline()`). The player then picks a tile again.
- There's no **Analyse** (LI-14).

**Wanted, as lichess.org's end of a game:**
- **Rematch**: as now.
- **New opponent**: a game from Quick pairing (LI-09) seeks again at once with the same clock, variant and Rated / Casual. The seek's waiting shows as it does on the Online page (the command bar's seek icon, Cancel). A game from a challenge or a link has no seek to repeat: there the button opens the Online page as New game does now, or is hidden (to decide).
- **Analyse**: LI-14's analysis of this game.
- **Review moves**: as now (maybe in the More menu if the card gets crowded in a narrow window).

**To decide in the plan:** how the card knows a game came from a seek (`LichessClient`'s `seek` matched its `gameStart`: keep the seek's settings with the game), and the buttons' order and labels in a narrow window (the card is over the board).

**Tests:** `tst_lichess`: a game paired from a seek ends; **New opponent** sends the same `POST /api/board/seek` body again; a challenge's game shows the other choice; **Analyse** is there once LI-14 is done.

**Plan:** [docs/LI-16-NewOpponentPlan.md](../../docs/LI-16-NewOpponentPlan.md). Decided by the owner (2026-11-26): New
opponent waits on the finished game (the seek in its command bar, the paired game replaces the board); a game not
from Quick pairing keeps New game. Analyse on the card is done (LI-14).

Related: LI-09 (Quick pairing), LI-14 (Analyse), LI-15.

## Resolution

### 2026-11-26 — step 1: New opponent (branch: features/li16NewOpponent_261126)
- `LichessClient`: the seek's game is remembered when it matches (`_seekGameId`); its `gameFull` sets
  `RematchInfo::fromSeek` and `rated`. `canSeekAgain()`, `seekAgain()` (the same clock, rating and variant).
- `ChessConnector`: `CanSeekAgain`, `newOpponent()` (withdraws our waiting rematch challenge, `withdrawRematch()`
  shared with `endGame()`, then `seekAgain()`; nothing while a seek waits). The game stays on the board until the
  seek's game replaces it.
- QML: New opponent (`BusyButton`, "Looking for an opponent…" while seeking) in New game's place on the result
  card (`newOpponentButton`) and in the moves panel; Ctrl+N on a finished game does the same. Other Lichess games
  keep New game.
- Tests: `LichessSeekTest` (+2: the same seek again; no seek to repeat after another game); `tst_lichess` test 20
  (a Quick pairing game → New opponent → the same body → paired on the same page; a waiting rematch withdrawn
  first) and test 19 (a challenge's game: New game). `FakeLichess` (UI) gives a seek's game the seek's clock,
  rating and variant. Scene `lichess-new-opponent` (also `@640x560`).
- Left: the hand tests on lichess.org, `docs/LI-16-ManualTests.md`. LI-16 closes once they pass.
