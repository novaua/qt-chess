# ChessQtTests/

The tests of the Qt parts: a DLL for the Microsoft CppUnitTest runner (Test Explorer, or
`vstest.console.exe x64\Debug\ChessQtTests.dll`). It links ChessCore, UciConnector, OnlineCore and LiChessConnector, and
**compiles the ChessGui sources it tests** (`AppConfig`, `UserManager`, `gamelibrary`, `gameanalyzer`,
`movegate`, …): a test of another ChessGui file adds that `.cpp` to `ChessQtTests.vcxproj`.

- **Running:** Qt's `bin` on `PATH`. `UciConnectorTest.cpp` and `ChessEnginePlayerTest.cpp` need Stockfish (one
  test of `GameAnalysisTest.cpp` too; the rest of it runs FakeUci, built with the tests). `LichessNetworkTests`
  (in `LichessClientTest.cpp`) calls the real lichess.org with a fake token. GitHub doesn't run this DLL
  (`tests.yml` runs ChessTests and the UI tests only): run it locally before a PR that touches what it tests.
- **One `QCoreApplication`** for the whole DLL, made by `TEST_MODULE_INITIALIZE` in `UciConnectorTest.cpp`; a
  test spins its own `QEventLoop` to wait for signals.
- `TestSupport.h`: `ToString(QString)` for CppUnitTest's messages; include it, don't define another.
- One test file per class under test (`<Class>Test.cpp`): `AppLogTest.cpp`, `BoardPiecesTest.cpp`,
  `MoveListModelTest.cpp`, `MoveGateTest.cpp`, `GameClockTest.cpp`, `ClaimWinTest.cpp`, `UserManagerTest.cpp`,
  `GameLibraryTest.cpp` (the games library, BL-19; its signed files, BL-20), `GameSignatureTest.cpp` (BL-20: the
  signature, key 1 pinned, a game file read back), `GamePgnTest.cpp` (a kept game's PGN), `GameAnalysisTest.cpp`
  (the engine's analysis of a kept game), `UciConnectorTest.cpp` (the UCI protocol), `ChessEnginePlayerTest.cpp`
  (also `EngineWorker`), and the Lichess ones below.
- **Lichess without Lichess:** `LichessClientTest.cpp`'s `LichessReconnectTests`, `LichessStreamTest.cpp`,
  `LichessChallengeTest.cpp` and `LichessSeekTest.cpp` (LI-09's quick pairing seek, `FakeSeekLichess`) play
  against `FakeLichess` (`FakeLichess.h`), a `QTcpServer` on 127.0.0.1 that answers each request with
  `onRequest(socket, method, path)` and keeps a stream open until the handler closes it. The client is pointed at
  it with `LichessClient::setBaseUrl` and made quick with `setStreamTiming` (and `setRequestTimeout`: LI-12's
  `LichessTimeoutTests`, a route that never answers). A new Lichess behaviour gets its
  fake answer here, never a call to lichess.org.
