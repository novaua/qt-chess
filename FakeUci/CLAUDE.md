# FakeUci/

A stand-in UCI engine for the tests (BL-05 step 3): a console exe on `ChessCore`, **no Qt**. It answers what the app
asks of Stockfish (`uci`, `isready`, `ucinewgame`, `setoption`, `position`, `go`, `quit`) at once and the same way
every time. The engine is `fakeengine.h/.cpp`; `main.cpp` only reads the environment.

- Its move: the move of the game in `FAKEUCI_MOVES` (`"e2e4 e7e5 g1f3 …"`) at that ply when it is legal there (a
  promotion without its piece: a queen); else the first legal move by its UCI text (a scripted move it can't play
  is said on stderr). A test writes the whole line; the robot plays its half.
- `FAKEUCI_DELAY_MS`: how long it "thinks" before each answer (a test can leave while it thinks).
- Its one line's score: `mate 1` when its move mates, else the material after it from the side to move's view
  (pawn 100, knight and bishop 300, rook 500, queen 900), so an analysis judges the same way every time.
- Chess960 as Stockfish plays it: `setoption name UCI_Chess960 value true`, then `position fen` of a Chess960 start
  and its moves; castling as the king onto its own rook.
- `moves` (not UCI) lists its legal moves.

Who runs it: the UI tests (`TestSetup`, through `CHESS_ENGINE_PATH`), the robot screenshot scenes
(`Take-Screenshot.ps1`), the installer's smoke test, and `ChessQtTests/GameAnalysisTest.cpp`. Its tests:
`ChessTests/FakeEngineTests.cpp`.
