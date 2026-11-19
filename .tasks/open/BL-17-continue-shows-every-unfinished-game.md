---
id: BL-17
title: "Continue shows every unfinished game"
type: task
status: open
priority: medium
---

# BL-17 Continue shows every unfinished game

<!-- migrated from TASKS.md:93 (In Progress / Backlog) -->

- [ ] BL-17 Continue shows every unfinished game; "Your games" last on the Online page [Medium] — **Coded (branch `features/continueList_101126`): the list is a page of its own pushed over Home (`ContinueGamesPage`), the cards shared with the Online page (`GameCard`); left: the owner's check in a real window with Lichess games.** The owner (2026-11-10): opening "Play online", the first thing on the page is "Your games" (the unfinished Lichess games, `OnlinePage.qml`), which isn't what the player came for; and Home's Continue resumes the robot game straight away (`ChessConnector.continueGame()`), so the Lichess games are only reachable through "Play online". To do: (1) **Continue opens a list of all pending games**: the robot's autosaved game first, in the upper left corner, then the unfinished Lichess games (the `BoardPreview` cards with Continue, as on the Online page now: most urgent first, "your turn" marked); a new page pushed over Home (or a section of Home: decide in the plan), with Refresh and a way back; Home's Continue card describes what is waiting (as `_continueText` does now). The owner's answers (2026-11-10): with only one pending game in all (the robot's and the Lichess ones together), Continue opens it directly, as today; with two or more, the list. The robot game's card shows its board, the robot's level and the player's side, like the Lichess cards. (2) **The Online page**: "Your games" moves, as it is, to the end, after "Join a game", and shows only the Lichess games (the challenges to the player stay first, then the new challenge, Join a game, Your games); Home's `onlineGamesRequested` ("the Online page, at Your games") then goes to the new Continue list instead. (3) Screenshot scenes: the Continue list with a robot game and Lichess games, with Lichess games only, with the robot game only, at the default size and narrow; `continue`, `continue-online` and the `online*` scenes re-baselined after a look. Update `CLAUDE.md` (HomePage, OnlinePage) and the `run-chess-app` skill's scene list.

## Resolution

### 2026-11-10 — BL-17: Continue shows every unfinished game (branch: features/continueList_101126)

#### What was done
- **Home's Continue** counts every unfinished game, the robot's autosaved one and the Lichess ones. With only one, it opens it directly as before (the robot's game, or the one Lichess game if it can be played here; a refused rejoin says so: "Couldn't continue the game: …", and the list is refreshed). With two or more it opens the **Continue list**. The card reads "Continue a game" then, e.g. "vs Robot · Level 3 · 4 Lichess games, your turn in 2". `ChessGame.continueGames()`; `HomePage.continueRequested` replaces `onlineGamesRequested`.
- **`ContinueGamesPage.qml`** (new): a page of its own pushed over Home (decided here: Home stays a launcher; Back and Esc as for the Online page). The robot's game first, in the upper left (its board as left, from the player's side, "Level 3 · Club Player · you play White", "Your turn" / "Robot's turn"), then the Lichess games, most urgent first ("Lichess · Rapid · 9 min left"; Open on lichess.org for one that can't be played here). Cards about 220 px wide, up to four in a row; Refresh for the Lichess games, an error line when they can't be loaded, the rejoin's error over the page.
- **`ChessConnector.ContinueGame`** (new): the autosaved robot game for its card: `{ level, playerPlaysWhite, playerToMove, fen, lastMove }`, read from the autosave once (`Game::Load` into a game of its own), until `canContinueChanged`.
- **`GameCard.qml`** (new): the Online page's game card, shared by both pages; **`lichess.js`**: Lichess's words for a game (`speedName`, `gameDetails`, `gameTitle`), also shared.
- **The Online page**: "Your games" moved, as it was, to the end, after "Join a game".
- **Screenshots**: new scenes `continue-list` (+ `1400x900`, `640x560`), `continue-list-online`, `continue-list-robot`; `continue`, `continue-online` and the `online*` scenes re-baselined after a look (the card's words; "Your games" last). Light and dark: 180 compared, all match after **28 new or changed baselines**.
- **The owner's `/code-review medium` of #144: 10 findings, all fixed** (same branch):
  - A rejoin is one state now, in `LichessClient` (`rejoiningGameId`), with its own failure (`rejoinFailed`, not `networkError`): another request's error is no longer taken for it (nor it for theirs), and it ends when its stream is stopped. `ChessConnector` starts it (`continueOnlineGame(id)`, `StartingOnlineGame`, `gameStartFailed` → "Couldn't continue the game: …" over the command bar, the list refreshed) and drops it on another profile, a sign-out or a new Lichess account, or Back from its page (`cancelOnlineStart()`). Home's Continue card stayed disabled when the stream was stopped elsewhere: no more.
  - Home's Continue is one call: `ChessConnector.continueOnlyGame()`, which also waits for the Lichess list while it loads (the Continue list opens, the games come into it).
  - An unreadable autosave counts as no robot game (`CanContinue` false), read once.
  - `OngoingGameCard` (a Lichess game's card) and `OngoingGamesRefresh`, shared by the Online page and the list; `GameCard.playable` instead of the icon's name; `Chess::BoardPositionToString` for the last move; `ContinueGame.title` dropped.
  - ChessQtTests **222 pass** (3 new: a rejoin's game, a refused rejoin gives `rejoinFailed` only, a stopped rejoin ends quietly).

#### Next
- The owner's check in a real window: Continue with only a robot game, only one Lichess game, both; Back / Esc from the list; a Lichess game that ended meanwhile.
