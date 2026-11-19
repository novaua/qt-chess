# BL-19 Games Library — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-16 |
| **Build** | `develop` with #162–#170 (Release) |
| **Plan** | [`BL-19-GamesLibraryPlan.md`](BL-19-GamesLibraryPlan.md) (§5: hand tests 1–8 passed on 2026-11-16) |
| **Run** | 2026-11-19 by Claude, `develop` at #201 (Release): the app's own window offscreen, driven with real mouse clicks by a throwaway Qt Quick Test file (the UI tests' `AppDriver`), on **Stockfish 19** (`CHESS_ENGINE_PATH`), with real time and files, on throwaway data folders. Not seen there: how it looks and feels in a real window, Windows' own contrast theme, a tooltip on hover (hover doesn't reach items offscreen). |

The UI tests play the games library against FakeUci, a stand-in engine that answers at once and the same
way every time, and the screenshot scenes show made-up games and a made-up analysis. Only the real app shows
that Stockfish's answers, its timing and real files behave. Write the result in the last line of each
scenario: **OK**, or what happened instead (and the time, so the app log can be found: *Settings → About →
Open log folder*).

**Use a throwaway data folder** for scenarios 1–7, so your own profiles and games stay as they are. "Games
kept" (scenario 1) trims every profile in the data folder. Start the app from a terminal with its own folder,
on a profile made there (any name):

```powershell
$app  = "$env:ProgramFiles\Chess++\ChessGui.exe"   # or x64\Release\ChessGui.exe of a build
$data = "$env:TEMP\chess-bl19"
& $app --data-dir $data
```

Already checked by the tests and the screenshot scenes, no need to look again: the Games page's cards and
filters (`games*`), the Play from here button and its menu (`play-from-here*`, `replay`), the analysis's
layout (`analysis`, three sizes), the result card's four buttons (`result`), Settings' new rows (`settings`).

## 1. "Games kept" with 51 finished games (plan §5, hand test 9)

1. In the throwaway folder, play one two-player game to the end (Fool's mate: f3 e5 g4 Qh4#) and one
   two-player game left unfinished (1. e4, then Home). Close the app.
2. Add 50 more finished games. Since BL-20 every game is a signed file of its own, which a script can't
   write (a game file that fails its check is set aside, and a `games.json` beside the game files too), so
   this step needs the UI tests' `TestRun.addFinishedGames(50)`: the UI tests run it (`tst_games` test 5,
   52 games), and Claude's run did this scenario with it.
3. Start the app again (`& $app --data-dir $data`). Ctrl+O: the Games page.
   - **Expected**: 52 games, 51 finished. The newest is first: the unfinished 1. e4 (played after the Fool's
     mate).
4. Settings → Data → **Games kept: 50**.
   - **Expected**: "Keep only 50 finished games?", saying 1 finished game is deleted for good. Delete it.
     Back on the Games page: 51 games; the oldest finished game is gone; the unfinished 1. e4 and the Fool's
     mate are still there.
5. Restart the app.
   - **Expected**: still 51 games, Games kept still 50.

Result: **OK** (Claude, 2026-11-19). 52 games, 51 finished, the unfinished 1. e4 first. "Keep only 50 finished
games?": "1 finished game is deleted for good: the oldest beyond 50 of each profile on this computer.
Unfinished games stay." Delete: 51 games, the oldest finished gone, the 1. e4 and the Fool's mate there. After a
restart: 51 games, Settings "50 games".

## 2. Play from here, with Stockfish

1. Play a two-player game for about 10 moves and resign it (an unfinished game's card offers only Continue,
   and a game going on isn't played from), then Home. Ctrl+O → Open it (a replay; it opens at the start).
   Go to move 5 with ›.
   - **Expected**: the review bar shows **Play from here**.
2. Play from here → **vs computer…** → level 3, you take the side to move → Start.
   - **Expected**: the board has the game's first moves; the robot answers when it is its turn. Takeback
     (Ctrl+Z) stops at the position you started from ("No more moves").
3. Play two moves, Home. Games page.
   - **Expected**: a new card "… from move 5" (or the move you chose); the first game is unchanged.

Result: **OK** (Claude, 2026-11-19; step 1 as written now: the unfinished game's card said Continue). At ply 8
of 20: "Move 8 of 20" and Play from here. The dialog: "Play from here vs computer", "From the position after
4… Nf6: White to move."; level 3, White. Stockfish answered in about 1 s; Ctrl+Z took that back, then "No more
moves" at ply 8. The new card: "you play White · from move 5 · … · 6 moves"; the first game still 20 plies.

## 3. The rating vs the robot

1. Play the robot at level 1 or 2 and win (or lose; a draw is fine too).
   - **Expected**: Home's account button shows "Rating 1xxx?" (the "?" stays until 10 rated games). The
     Games card says "You won · +NN" (or "· −NN"). Profile page → Statistics: "Rating vs the robot".
2. Play another robot game to the end, then Ctrl+Z after the result (the game goes on).
   - **Expected**: the rating goes back to what it was before that game; finishing the game now doesn't
     change it (a game with a takeback isn't rated).
3. Delete a rated game (Games page → Delete).
   - **Expected**: the rating doesn't change.
4. Start a robot game as Black and resign right after the robot's first move (before you have moved).
   - **Expected**: not rated, as an aborted game on lichess.org: the rating and the number of rated games stay
     the same; its card has no "· ±NN".

Result: **OK** (Claude, 2026-11-19; lost to level 1 by mate, a player of random moves). 1161 → 1123: Home "Rating
1123?", the card "You lost · −38", Profile → Statistics "Rating vs the robot" 1123?. 2: a mate (1123 → 1085),
Ctrl+Z: back to 1123, the same rated games; mated again: still 1123, the record not rated. 3: deleting a rated
game: 1123 stays. 4: Black, resigned after the robot's first move: 1123 and the rated games unchanged, the card
"You lost" without a change.

## 4. Analysing a real game

1. Play the robot (level 2 or 3) to a result, 20–40 moves. On the result card: **Analyse**.
   - **Expected**: the card goes; the moves panel says "Analysing… N of M" and counts up. Write down
     roughly how long it takes (Settings → Computer → Analysis depth is Normal (14)).
2. When it's done:
   - **Expected**: the summary shows both sides' accuracy and counts of ?! ? ??; the moves in the list have
     marks; the eval bar left of the board. Click a move with "??": "Blunder · Best was …" under the
     summary. Does the best move make sense? Does the eval bar lean to the side that is better?
3. Flip the board (F).
   - **Expected**: the bar turns with it (White's part at White's side).
4. Home, then Ctrl+O → Open the same game.
   - **Expected**: the analysis is there at once, without analysing again.

Result: **OK** (Claude, 2026-11-19), how long it took: **2.6 s** for 49 positions (24 moves, Normal (14), this
PC). "Analysing… 3 of 49" while it ran; then White 78 % (1 ?!, 1 ?, 1 ??), Black 97 %; marks in the list, the eval
bar. 7. Ne4??: "Blunder · Best was e3", −7.8, both arrows. Checked with Stockfish apart (python-chess, depth 18):
e3 is its first choice there too, and at the ? move; the evals agree (−7.8 / −7.5, −1.3 / −1.2, −0.6 / −0.5), and the
bar leans to the side the eval favours. F turns the bar with the board; reopened from the Games page the analysis is
there at once.

## 5. Analysis depth, Cancel, leaving

1. Settings → Computer → Analysis depth: **Deep (18)**. Open the game of scenario 4 → More → **Analyse
   again**.
   - **Expected**: slower than Normal; the marks may change a little when it is done.
2. Analyse again, and **Cancel** half way.
   - **Expected**: the progress goes; the earlier analysis stays as it was.
3. Analyse again, and go Home while it runs. Wait a while, then open the game again.
   - **Expected**: the analysis went on in the background: the game has it (or is still being analysed: the
     progress shows).
4. Analyse again, and close the app while it runs.
   - **Expected**: the app closes within a second or two; Task Manager shows no stockfish.exe left over.

Result: **OK** (Claude, 2026-11-19). Deep (18): 16 s against Normal's 2.6 s, the same marks. Cancel at 25 of 49:
the progress went, the earlier analysis exactly as it was. Home while it ran: it went on (15 of 49 after 5 s),
and the game reopened showed the progress, then the analysis. Closed while analysing: the process was gone
0.19 s after the window, no stockfish.exe left.

## 6. Without the engine

1. Close the app; start it with an engine that isn't there:

   ```powershell
   $env:CHESS_ENGINE_PATH = 'C:\no-such-engine.exe'; & $app --data-dir $data; Remove-Item Env:CHESS_ENGINE_PATH
   ```
2. Open a finished game from the Games page.
   - **Expected**: Settings → Computer says the engine isn't found. **Analyse game** in the moves panel and
     More → Analyse are disabled; hovering the panel's button says the analysis needs the chess engine. A
     game analysed before still shows its analysis.

Result: **Found a bug** (Claude, 2026-11-19; BL-25). After Settings has been opened: "Chess engine not found"
(and "C:\no-such-engine.exe didn't answer."), Analyse game in the panel and More → Analyse disabled, the analysed
game shows its analysis: OK. **But before Settings has been opened in a session, Analyse game is enabled**: the
engine is probed only when the Settings page is made, and the button counts "checking" as found. Pressing it:
"The chess engine couldn't analyse the game." Not checked: the tooltip on hover (no hover offscreen).

## 7. Not during a game

1. Start a robot game, play a few moves.
   - **Expected**: More → Analyse game is disabled; the moves panel has no Analyse button (no hints during a
     game).

Result: **OK** (Claude, 2026-11-19). vs the robot (Stockfish, level 1), three moves: More → "Analyse game"
disabled, no Analyse in the moves panel.

## 8. Export and import keep the games, the rating and the analysis

This one runs in your normal data folder (no `--data-dir`), or in the throwaway one if you prefer.

1. Profile with rated games and an analysed game: Settings → Data → Profiles → **Export…**.
2. Import the file on another Windows user (or another data folder: `--data-dir` a new folder) as a new
   profile.
   - **Expected**: the same games in the Games page, the same rating on Home, and the analysed game opens
     with its analysis.

Result: **OK** (Claude, 2026-11-19; the export and the import called as Settings' buttons call them, without
the file dialogs). Into another data folder, as a new profile: the same 57 games, rating 1113 (3 rated games),
nothing set aside; the analysed game opens with its analysis.

## 9. Dark mode and high contrast

1. Settings → Appearance → Dark. Open the analysed game.
   - **Expected**: the marks (?! blue, ? orange, ?? red) and the eval bar are easy to read on the dark panel.
2. Windows high contrast (Settings → Accessibility → Contrast themes), the app open.
   - **Expected**: the marks are readable (they take the text colour); the eval bar is still visible.

Result: 1 **OK** (Claude, 2026-11-19, from a picture): ?! blue, ? orange, ?? red and the eval bar read well on the
dark panel. 2 **open, for the owner**: it needs Windows' own contrast theme (forcing the app's high contrast on a
normal Windows theme mixes its colours, so a picture of that says nothing).

## Clean up

```powershell
Remove-Item -Recurse "$env:TEMP\chess-bl19"
```
