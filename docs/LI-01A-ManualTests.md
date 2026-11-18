# LI-01A Chess960 (stage 1, local games) — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-18 |
| **Build** | `develop` with #175–#178 (Release) |
| **Run** | 2026-11-18 by the owner: all 8 scenarios passed |
| **Plan** | [`LI-01A-Chess960Plan.md`](LI-01A-Chess960Plan.md) (§5: "By hand") |
| **Rules** | <https://lichess.org/variant/chess960> |

The UI tests play Chess960 against FakeUci, a stand-in engine that answers at once and the same way every
time, and always at fixed positions (#3, #518). Only the real app shows that Stockfish plays Chess960 at every
level, that random positions work, and that lichess.org reads the PGN the same way. Write the result in the
last line of each scenario: **OK**, or what happened instead (and the time, so the app log can be found:
*Settings → About → Open log folder*).

**Use a throwaway data folder**, so your own profiles, games and rating stay as they are. Start the app from a
terminal with its own folder, on a profile made there (any name):

```powershell
$app  = "$env:ProgramFiles\Chess++\ChessGui.exe"   # or x64\Release\ChessGui.exe of a build
$data = "$env:TEMP\chess-li01a"
& $app --data-dir $data
```

**How to castle in Chess960** (as on lichess.org): select the king, then click its rook (or drop the king on its
rook). When the king's castling square (g1 for O-O, c1 for O-O-O) is not a square the king can go to anyway,
it has a dot too, and a click there castles as well. After O-O the king is on g1 and the rook on f1; after
O-O-O the king is on c1 and the rook on d1 (g8 / f8, c8 / d8 for Black), wherever they started.

Already checked by the tests and the screenshot scenes, no need to look again: the Variant row and its line
(`computer-960`, `players`), "Chess960 #3" in the title (`chess960`), the dots on the rook (`chess960-select`),
the swap of king f1 and rook g1 (`chess960-castled`), the Games card's "Chess960 #3" (`games`); castling by
drop and by click, Rematch keeps the position, Continue, replay, Play from here, the PGN's tags, the robot's
castling and a premoved castling against FakeUci (`tst_chess960`, 11 tests); every one of the 960 positions and
60 random games against python-chess (`Chess960Tests.cpp`).

## 1. Two players: a random position, both castlings

1. Home → **Two players** → Variant: **Chess960** → Start game.
   - **Expected**: the pieces behind the pawns are shuffled, the same for both sides (Black's mirror White's);
     the bishops are on squares of different colours and the king is between the two rooks. The title says
     "… · Chess960 #N". Write down N.
2. Play both sides until each can castle, then castle **O-O with one side and O-O-O with the other**: one by
   dropping the king on its rook, one by clicking the king, then the rook (or the castling square when it has a
   dot).
   - **Expected**: king and rook end on g/f (O-O) or c/d (O-O-O); both slide there (Settings → Appearance →
     Reduce animations off). The move list says O-O / O-O-O, not a king move or a capture; no piece shows up in
     the captured pieces.
3. Optional, if your position has one of these (New game until it does, it is random): a king that doesn't move
   when it castles (king on g1 with a rook on h1, or king on c1 with a rook on a1 or b1), or a king and a rook
   that swap squares (king f1, rook g1; king d1, rook c1).
   - **Expected**: castling still works the same way (select the king, click the rook): only the rook moves, or
     the two swap.
4. Try to castle where the rules forbid it: through a square the opponent attacks, while in check, after the
   king or that rook has moved (moving away and back counts).
   - **Expected**: no dot on the rook; a click on the rook just selects it.

Result: **OK** (2026-11-18; O-O and O-O-O both castled)

## 2. The robot at level 1 and level 5 (Stockfish)

Levels 1–2 are the app's own weak picker over Stockfish's best moves, levels 3–5 are Stockfish's own strength
setting: the two paths ask the engine differently.

1. Home → **Play vs computer** → level **1**, Variant: **Chess960** (the dialog line also says "Not rated") →
   Start. Play 15–20 moves, castle when you can.
   - **Expected**: the robot plays legal moves at once, never an illegal or strange one (a "castling" that
     isn't, a piece jumping), and castles now and then. When it castles, king and rook land on the right squares.
2. The same at level **5**, playing Black (You play: Black).
   - **Expected**: the robot starts at once; strong, normal play; it castles at some point (it nearly always
     does in a long game).
3. In one of those games, **premove a castling** while the robot thinks: select your king, then click its
   rook.
   - **Expected**: the rook's square is marked as a premove; after the robot's move you castle at once (if it is
     still legal; else the premove just goes).

Result: **OK** (2026-11-18)

## 3. Takeback, leaving and Continue

1. In a robot game (any level) right after you castled and the robot answered: **Undo** (Ctrl+Z).
   - **Expected**: both moves go back; king and rook stand on their own start squares again. Castle again: it
     works as before.
2. Play a few more moves, then **Home**.
   - **Expected**: Home offers "Continue a game"; the card (Ctrl+O, the Games page) says "Chess960 #N · you play
     …".
3. Close the app, start it again (`& $app --data-dir $data`), **Continue** that game.
   - **Expected**: the same position and title "… · Chess960 #N"; the robot (same level) goes on; castling
     rights are as they were (a side that castled or moved its king can't castle again).

Result: **OK** (2026-11-18)

## 4. Rematch and New game

1. Finish a Chess960 game (resign is fine). On the result card: **Rematch**.
   - **Expected**: the same #N, colours swapped.
2. Finish that one too, then **New game** on the result card.
   - **Expected**: the setup dialog opens with Chess960 chosen; Start gives a different #N.
3. Close the dialog, go Home, open **Two players**.
   - **Expected**: Chess960 is still chosen (the last choice), also after a restart of the app. Choose Standard
     and start: a normal game, no "Chess960" in the title.

Result: **OK** (2026-11-18)

## 5. Not rated

1. Note Home's rating (account button; a new profile: none yet). Finish a Chess960 game vs the robot (win, lose
   or draw).
   - **Expected**: the rating and the number of rated games don't change; the Games card has no "· +NN".
2. Finish a standard robot game.
   - **Expected**: that one is rated as before.

Result: **OK** (2026-11-18)

## 6. Replay, Play from here, Analyse

1. Games page → **Open** a finished Chess960 game that has a castling.
   - **Expected**: the replay starts from the game's own position (not the standard one). Step with ‹ › over the
     castling: king and rook go there and back.
2. Step to a position a few moves in → **Play from here** → **vs computer…**.
   - **Expected**: no Variant row in the dialog. The new game has the first moves; the robot answers legal
     moves; its card is "Chess960 #N · … from move …".
3. Back to the finished game: **Analyse** (result card or More → Analyse game).
   - **Expected**: the analysis runs to the end (no error, no stop). The marks and accuracy look plausible; on a
     judged move where the best move is a castling, the green arrow goes from the king to its rook.

Result: **OK** (2026-11-18)

## 7. The PGN on lichess.org

1. A finished Chess960 game with both castlings (scenario 1 or 6): More → **Export PGN…** (or a Games card's
   Export button), and open the file in Notepad.
   - **Expected**: tags `[Variant "Chess960"]`, `[SetUp "1"]` and `[FEN "<the start>"]`; castlings as O-O / O-O-O.
2. Open <https://lichess.org/paste>, paste the PGN, **Import game**.
   - **Expected**: lichess.org takes it as Chess960 with the same start position; every move is read (none cut
     off), the castlings land where they did in the app, and the final position is the same.

Result: **OK** (2026-11-18; lichess.org imports the PGN)

## 8. The engine setting "Send the game to the engine as"

1. Settings → Advanced → **Send the game to the engine as: Start position and moves** (the other one than now).
   Play a Chess960 robot game past a castling (yours and, if it comes, the robot's).
   - **Expected**: the same as in scenario 2: legal moves only. (A Chess960 game is always sent as its start and
     every move, whatever this setting says.)
2. Set it back to **Position (FEN)**.

Result: **OK** (2026-11-18)

## Clean up

```powershell
Remove-Item -Recurse "$env:TEMP\chess-li01a"
```
