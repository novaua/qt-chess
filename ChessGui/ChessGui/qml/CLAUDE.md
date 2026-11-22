# ChessGui/ChessGui/qml/

The app's QML. The C++ it calls: `ChessConnector` (the game), `UserManager`, `LichessClient`, `GameLibrary`,
`AvatarProvider`, `AppInfo`, singletons in module `ChessPlusPlus`; their API is in
`.claude/rules/chessgui-cpp.md` and the headers.

## Rules
- The game only through `ChessConnector`, one call per user action (the root's Key Conventions, rule 1).
- A file that uses the services has `import ChessPlusPlus`.
- Pages ask for the app's dialogs through signals; `ChessGame.qml` owns them and the window's InfoBars. A page's
  own confirm or note stays in it (`GamesPage`'s Delete `ConfirmDialog`; the InfoBars of `MovesPanel`,
  `OnlinePage`, `ProfilePage`).
- Every short message goes through `ChessGame.notify()`.
- An item a UI test needs has an `objectName` (`ChessGuiTests/` finds items by it).
- Colours from `Theme` (light, dark, high contrast); the app's options in `AppSettings` (the QtCore `Settings`).
- `-platform offscreen` (the UI tests, `-Offscreen` screenshots) renders with the software renderer, which has no
  shader effects: an item with `layer.effect` (`MultiEffect`) isn't drawn at all there. Enable such a layer only
  with `GraphicsInfo.api !== GraphicsInfo.Software`, so the item is drawn plain (the move list's and the
  captured strip's piece glow, BL-27).
- Debug and test helpers live only in `ScreenshotScene` (loaded only in `--screenshot` mode), never in the pages.
- A new QML file goes into `qml.qrc`, `ChessGui.vcxproj` and `ChessGui.pro`.

## The window and the pages
- `ChessGame.qml` — the window: a `StackView` with one page (`WelcomePage` on first run, `ProfilePickerPage`,
  `HomePage`, `GamePage`; `route()` picks it from `UserManager` and `ChessConnector.InGame`; before it builds the
  game page it shows "Starting the game…" (`StartNotice.qml`, over the popups) and waits until that is drawn:
  `ChessConnector.waitForStartFrame`).
  `ProfilePage` or `SettingsPage` are pushed over Home or the game, `OnlinePage` and `ContinueGamesPage` over
  Home. The InfoBars: under the command bar; on the game page's player column between its cards
  (`GamePage.barsArea`); the challenge bar over every page (Accept / Open on lichess.org, Decline, Hide; a sound
  and the taskbar flash for a new one).
- `GameSounds.qml` — every sound the app plays, one instance in the window (objectName `gameSounds`), with the
  sound effects switch and the volume. `play(name)` at once (check, low time, a challenge, SettingsPage's
  preview through `soundPreviewRequested`); `playWhenSeen(name, then)` once the board shows the position
  (`ChessConnector.boardIdle`; at once while reviewing): a move's check (`then`: the "Check" message, with sound
  off too), the mate and the win. Only a move's own sound, from its own Connections, plays as the slide starts.
  `played(name)` for the UI tests. A new sound goes here, not into a page.
- `HomePage.qml` — account menu; Continue: the only unfinished game directly, two or more:
  `ContinueGamesPage` (`ChessGame.continueGames()`); action cards, among them **Games** (Ctrl+O): `GamesPage`.
- `ContinueGamesPage.qml` — every unfinished game: the kept local games first as `GameCard`s
  (`GameLibrary.unfinishedGames`), then the Lichess ones as `OngoingGameCard`s.
- `GamesPage.qml` — the games library, objectName `library`: every kept game as a `GameCard`, Open (a replay) or
  Continue, Export PGN, Delete; the filters All · vs Robot · Two players · Unfinished. `games.js`: the words for a
  kept game, shared with Home and the Continue list.
- `OnlinePage.qml` — pushed by "Play online" (`onlineCard`): the challenges to the player with Accept and
  Decline… with Lichess's reasons; Quick pairing (`QuickPairingGrid`, lichess.org's tiles: Rapid and Classical
  seek a random opponent (`LichessClient.startSeek`), Bullet and Blitz greyed, Custom: `SeekDialog`; Rated /
  Casual, `AppSettings.quickRated`, `quickChess960`); the seek goes on when the page is left, shown in every
  command bar (`SeekIndicator`). A new challenge (a link, or to a named Lichess player, the only way to play
  Blitz; its Variant row, `AppSettings.onlineChess960`, apart from the local dialogs', and the Chess960 rating
  when chosen); join by link; last the unfinished Lichess games as `OngoingGameCard`s with Continue and
  `OngoingGamesRefresh`. `timecontrols.js`: the clock presets, shared with `TimeControlPicker`; `lichess.js`:
  Lichess's words for a game, shared with the Continue list.
- `GamePage.qml` — command bar, the board between two `PlayerCard`s with a `CapturedStrip` and "+N" (in a wide
  window where that gives a bigger board, the cards go `stacked` in a column left of the board, `area.side`); the
  moves (`MovesPanel` + `MoveList`) beside the board or in a bottom sheet; `ResultCard` over the board; `EvalBar`
  and `AnalysisSummary` (the moves panel; "Best was …" in green) for an analysed game; Analyse on the result card
  and in More. The review bar's **Play from here** (`MovesPanel`, the sheet handle in a narrow window) → its menu
  (vs computer… / Two players…) → the setup dialogs' `openFrom(position)`.
- `WelcomePage`, `ProfilePickerPage`, `ProfilePage`, `SettingsPage` — first run, "Who's playing?", the profile,
  the settings.

## The board
- `ChessBoard.qml` — one geometry for all layers (`squareX/Y`, `squareAt`, a1 bottom left; turned for Black by
  `rotation`); the layers made behind a `LayoutGate`; the click `TapHandler`; the drag state (`dragFrom`,
  `dragPoint`, `dropSquare`) and the dragged piece under the pointer; coordinates (`MarkerRow`, `MarkerColumn`:
  only the shown set, the labels' size fitted once); the promotion dialog (`PawnPromotionOptions`).
- `ChessBoxes.qml` — the squares and their highlights (last move, premove, check, flagged king, selected piece,
  drop square: one item each, moved to its square).
- `NavigationLayer.qml` — the legal-move dots (premove targets in red). A target that takes a piece
  (`ChessConnector.CaptureSquares`: the other side's piece there, or en passant) shows its four corners in the
  dot's colour instead, as lichess.org, from a second instance under the pieces (`kind: "captures"`, objectName
  `captureLayer`; the corners a clipped Rectangle ring, made once per square; `ChessBoard` reads the targets
  once for both layers).
- `ChessFigures.qml` — the pieces, one item per row of `ChessConnector.Pieces` on its square; it slides when the
  square changes, a captured one fades out; a `DragHandler` per piece of the side to move, or of the player during
  the opponent's turn (a premove); drop → `ChessConnector.movePiece`.
- `MoveArrows.qml` — on a judged move, over the pieces: the move played in its mark's colour and the engine's best
  in green.

## Shared components
- `Theme.qml` (also `levelColor(level, count)`: the computer levels' colours), `AppSettings.qml` (singletons), `SettingRow`, `AppDialog`, `ConfirmDialog`, `InfoBar`,
  `IconButton`, `HomeCard`, `BoardPreview`, `ChoiceRow`, `SectionCard`, `AvatarPicker`, `FocusRing` (keyboard
  focus outline for custom-drawn items).
- `CommandBar` (`messageArea`: the item over which short messages show; at its right end `SeekIndicator`: a seek
  waiting, its menu with Cancel seeking).
- `CommandBarMessage` — a message over the command bar, taking no room: "Check", "Game saved", an offer with
  Accept / Decline, the lost connection, the opponent left (Claim victory: `ChessConnector.CanClaimVictory`;
  closable). A short message takes the state's place for its time (an error until closed), but an offer or Claim
  victory is hidden at most 4 s ("Check" not at all) and takes the place back at once when it comes; text without
  buttons is cut short to fit (all of it on hover); only what doesn't fit falls back to the InfoBar.
- `MessagesFlyout` — the recent messages: the last 20 of the session, an open offer still answerable; "Recent
  messages" in the game's More menu and Home's account menu, Ctrl+M, a dot for new ones (`NewMessagesDot`,
  `RecentMessagesMenuItem`); cleared when the profile changes.
- `GameCard` — a game: a `BoardPreview`, who, whose turn or the result, Continue / Open, an optional Delete.
  `OngoingGameCard`: one of `LichessClient.ongoingGames`; a right click opens its menu: Continue (a game playable
  here), Open on lichess.org, Copy link (`AppInfo.copyToClipboard`, "Link copied").
- `TimeControlPicker` (the clock tiles of the setup dialogs and the challenge card), `VariantPicker` (the setup
  dialogs' Variant row, Standard / Chess960, `AppSettings.chess960`; none in Play from here; `online`: the Online
  page's, `AppSettings.onlineChess960`).
- `BusyButton` (a button whose action waits for Lichess: `busy`, a ring in the icon's place and `busyText`; set
  `idleText` / `idleIcon`), `BusyRing` (spinning, or a still clock with Reduce animations).

## Dialogs
- Setup: `ComputerGameDialog`, `TwoPlayerDialog` (both with the Variant row), `ChallengeDialog` (a Lichess
  challenge waiting for the friend or a named player), `SeekDialog` (Quick pairing's Custom tile: the player's own
  clock or days per move; Seek off below Rapid), `LeaveOnlineDialog` (Leave / Resign / Stay when leaving or
  closing during a running Lichess game).
- Profiles: `CreateUserDialog`, `ImportConflictDialog`, `RestoreBackupDialog`.
- Others: `LevelsHelpDialog` (the five computer levels: F1, the robot's avatar), `ApplyMovesDialog` (moves typed
  in UCI: `ChessConnector.applyMoves`).

## Tooling
- `ScreenshotScene` — the `--screenshot` scenes (the `run-chess-app` skill); `SceneInput` plays them with the
  mouse.
