# BL-19 Games Library — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-16 |
| **Build** | `develop` with #162–#170 (Release) |
| **Plan** | [`BL-19-GamesLibraryPlan.md`](BL-19-GamesLibraryPlan.md) (§5: hand tests 1–8 passed on 2026-11-16) |

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
2. Add 50 more finished games to the profile's file with this script (PowerShell, the app closed). The
   profile's folder is the one under `users\` (one profile: one folder).

   ```powershell
   $file = (Get-ChildItem "$env:TEMP\chess-bl19\users\*\games.json" | Select-Object -First 1).FullName
   Copy-Item $file "$file.bak"
   $records = for ($i = 1; $i -le 50; $i++) {
       $start = (Get-Date '2026-10-01T18:00:00').AddHours($i)
       [ordered]@{
           id = [guid]::NewGuid().ToString(); kind = 'local'; finished = $true
           startedAt = $start.ToString('s'); finishedAt = $start.AddMinutes(5).ToString('s')
           playerPlaysWhite = $true; opponentName = "Guest $i"; playerAvatar = ''; opponentAvatar = ''
           moves = @('f2f3', 'e7e5', 'g2g4', 'd8h4')
           fen = 'rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3'
           result = '0-1'; resultText = "Guest $i wins by checkmate" } | ConvertTo-Json -Compress
   }
   # Into the file's list as text: the app's own records stay exactly as they are.
   $text = [IO.File]::ReadAllText($file)
   $text = $text.Replace('"games":[', '"games":[' + ($records -join ',') + ',')
   [IO.File]::WriteAllText($file, $text)   # UTF-8 without a BOM
   ```
3. Start the app again (`& $app --data-dir $data`). Ctrl+O: the Games page.
   - **Expected**: 52 games, 51 finished. The newest is first: your real Fool's mate game.
4. Settings → Data → **Games kept: 50**.
   - **Expected**: "Keep only 50 finished games?", saying 1 finished game is deleted for good. Delete it.
     Back on the Games page: 51 games; "Guest 1" (the oldest, 1 Oct) is gone; the unfinished 1. e4 is still
     there.
5. Restart the app.
   - **Expected**: still 51 games, Games kept still 50.

Result:

## 2. Play from here, with Stockfish

1. Play a two-player game for about 10 moves, then Home. Ctrl+O → Open it (a replay). Step back to move 5
   with ‹.
   - **Expected**: the review bar shows **Play from here**.
2. Play from here → **vs computer…** → level 3, you take the side to move → Start.
   - **Expected**: the board has the game's first moves; the robot answers when it is its turn. Takeback
     (Ctrl+Z) stops at the position you started from ("No more moves").
3. Play two moves, Home. Games page.
   - **Expected**: a new card "… from move 5" (or the move you chose); the first game is unchanged.

Result:

## 3. The rating vs the robot

1. Play the robot at level 1 or 2 and win (or lose; a draw is fine too).
   - **Expected**: Home's account button shows "Rating 1xxx?" (the "?" stays until 10 rated games). The
     Games card says "You won · +NN" (or "· −NN"). Profile page → Statistics: "Rating vs the robot".
2. Play another robot game to the end, then Ctrl+Z after the result (the game goes on).
   - **Expected**: the rating goes back to what it was before that game; finishing the game now doesn't
     change it (a game with a takeback isn't rated).
3. Delete a rated game (Games page → Delete).
   - **Expected**: the rating doesn't change.

Result:

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

Result: (how long it took:)

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

Result:

## 6. Without the engine

1. Close the app; start it with an engine that isn't there:

   ```powershell
   $env:CHESS_ENGINE_PATH = 'C:\no-such-engine.exe'; & $app --data-dir $data; Remove-Item Env:CHESS_ENGINE_PATH
   ```
2. Open a finished game from the Games page.
   - **Expected**: Settings → Computer says the engine isn't found. **Analyse game** in the moves panel and
     More → Analyse are disabled; hovering the panel's button says the analysis needs the chess engine. A
     game analysed before still shows its analysis.

Result:

## 7. Not during a game

1. Start a robot game, play a few moves.
   - **Expected**: More → Analyse game is disabled; the moves panel has no Analyse button (no hints during a
     game).

Result:

## 8. Export and import keep the games, the rating and the analysis

This one runs in your normal data folder (no `--data-dir`), or in the throwaway one if you prefer.

1. Profile with rated games and an analysed game: Settings → Data → Profiles → **Export…**.
2. Import the file on another Windows user (or another data folder: `--data-dir` a new folder) as a new
   profile.
   - **Expected**: the same games in the Games page, the same rating on Home, and the analysed game opens
     with its analysis.

Result:

## 9. Dark mode and high contrast

1. Settings → Appearance → Dark. Open the analysed game.
   - **Expected**: the marks (?! blue, ? orange, ?? red) and the eval bar are easy to read on the dark panel.
2. Windows high contrast (Settings → Accessibility → Contrast themes), the app open.
   - **Expected**: the marks are readable (they take the text colour); the eval bar is still visible.

Result:

## Clean up

```powershell
Remove-Item -Recurse "$env:TEMP\chess-bl19"
```
