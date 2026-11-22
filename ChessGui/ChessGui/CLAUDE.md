# ChessGui/ChessGui/

The app: Qt Quick. The QML is in `qml/` (its own `CLAUDE.md`); the C++ beside it. **The C++ catalogue,
class by class, is `.claude/rules/chessgui-cpp.md`**: it loads with any `.h` / `.cpp` here. For a QML task
that needs `ChessConnector`'s API, Read it or `chessconnector.h`.

- **Start:** `main.cpp` → `AppServices` (makes the services, wires them, registers the QML singletons in
  module `ChessPlusPlus`); the UI tests' `TestSetup` uses `AppServices` too.
- **The C++ by area:**
  - the game: `chessconnector`, `boardpieces`, `movelistmodel`, `movegate`, `gameclock`, `claimwin`;
  - the robot: `ChessEnginePlayer`, `EngineLevel.h`, `engineworker`;
  - kept games: `gamelibrary`, `gamesignature`, `gamepgn`, `gameanalysis`, `gameanalyzer`;
  - profiles and app services: `UserManager`, `AvatarProvider`, `AppInfo`, `AppConfig`, `appservices`;
  - infrastructure: `applog`, `frametrace`, `layoutgate`, `sceneinput`.
  - `networkplayer.h/cpp` are in no project, an early sketch (BL-33).
- **Assets:** `pics/` (pieces in `white/` and `black/`, `avatars/`, `icons/`, the window's photo, the app icon),
  `sounds/*.wav`; all bundled through `qml.qrc`.
- **A new file goes into every list that builds it:**
  - a `.cpp`: `ChessGui.vcxproj`, `ChessGui.pro`, `ChessGuiTests.vcxproj` (the link fails without it), and
    `ChessQtTests.vcxproj` when a Qt test covers it;
  - a QML file or asset: `qml.qrc`, `ChessGui.vcxproj`'s `<None>` list and `ChessGui.pro`'s `DISTFILES`.
