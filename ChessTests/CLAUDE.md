# ChessTests/

The chess rules' tests: a DLL for the Microsoft CppUnitTest runner (Test Explorer, or
`vstest.console.exe x64\Debug\ChessTests.dll`). It links only `ChessCore` and compiles `FakeUci/fakeengine.cpp`;
**no Qt**: a test that needs Qt (UCI engine, Lichess, users, models) goes into `ChessQtTests/`.

- `BasicTest.cpp`, `CheckTests.cpp`, `FenGenTests.cpp`, `SanTest.cpp`, `UndoTests.cpp`, `BoardDiffTests.cpp`,
  `MatingMaterialTests.cpp`, `PremoveTests.cpp` — `Game`, `Board`, `MoveGeneration`, SAN and FEN directly.
- `FakeEngineTests.cpp` — FakeUci's engine (`FakeUci/fakeengine.h`).
- `Chess960Tests.cpp` (LI-01A) — the start positions, castling from any set-up, and 60 random games checked ply
  by ply (legal move counts, final FEN) against python-chess: `Chess960Reference.h`, made by
  `python scripts\make_chess960_reference.py` (a fixed seed). Regenerate it with the script; never edit it by hand.
