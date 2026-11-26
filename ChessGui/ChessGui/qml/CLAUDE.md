# ChessGui/ChessGui/qml/

The app's QML. The C++ it calls: `ChessConnector` (the game), `UserManager`, `LichessClient`, `GameLibrary`,
`AvatarProvider`, `AppInfo`, singletons in module `ChessPlusPlus`; their API is in
`.claude/rules/chessgui-cpp.md` and the headers.

## Rules
- The game only through `ChessConnector`, one call per user action (the root's Key Conventions, rule 1).
- A file that uses the services has `import ChessPlusPlus`.
- Pages ask for the app's dialogs through signals; `ChessGame.qml` and its parts (`ProfileFiles`, …) own them and
  the window's InfoBars. A page's own confirm or note stays in it (`GamesPage`'s Delete and `SettingsPage`'s
  "Games kept" `ConfirmDialog`; the InfoBars of `MovesPanel`, `OnlinePage`, `ProfilePage`). A page with a dialog
  of its own has `readonly property bool dialogOpen`: the window's `_dialogOpen` reads it, so its shortcuts
  (Esc among them) wait while it is open.
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
  off too; told once, and only if the board still shows a check then), the mate and the win. Only a move's own sound, from its own Connections, plays as the slide starts.
  `played(name)` for the UI tests. A new sound goes here, not into a page.
- `MessageCenter.qml` — the messages' state, one instance in the window (`messageCenter`, `window.messages` for
  `ScreenshotScene`): the game's state (`statusKind`: an offer, the opponent left, the lost connection; Claim
  victory), the short message (`notify()`, which `ChessGame.notify()` forwards; 4 s, an error until closed) and
  the recent messages (`model`, `unseen`, `addRecent()`, `resetForUser()`). The window draws them
  (`CommandBarMessage`s, the InfoBars, `MessagesFlyout`) and tells it what they show (`shortShowing`,
  `statusFits`, `flyoutOpen`).
- `LichessChallenges.qml` — the Lichess challenges' state, one instance in the window (`lichessChallenges`,
  `window.challenges` for `ScreenshotScene`): the player's challenge waiting for the opponent (`pendingId`,
  `opponentName`, `rematch`; `resumePending()`, `clearPending()`; a rematch is withdrawn by `ChessConnector.endGame()`; it asks the window to
  open or close `ChallengeDialog`) and the ones to the player (`challengesShown`, `challenge`, `text()`,
  `accept()`, `hide()`). The challenge bar and the dialog stay in the window; a new one's sound is GameSounds',
  the taskbar flash the window's.
- `ProfileFiles.qml` — a profile's export, import (`ImportConflictDialog` when it is here already) and delete
  (`deleteProfileConfirm`), with their dialogs; one instance in the window (`profileFiles`), which forwards the
  pages' requests and `inspectImport()` for the `import` scene. `DataCheckNotices.qml` — BL-20's notices (games set
  aside, the rating or the statistics restored): `tellAll()` once the window is up, then on each load.
- `LeaveFlow.qml` — leaving the game on the board: Home and Esc (`leave()`), Switch user (`switchUser()`),
  Resign (`resign()`) and closing the window (`windowClosing(event)`, from its `onClosing`), with their dialogs
  (`resignConfirm`, `switchUserConfirm`, `leaveOnlineDialog`; `onlineDialog` for `ScreenshotScene`); one instance
  in the window (`leaveFlow`). Each answer is one `ChessConnector` call. A running Lichess game asks first; its
  Resign leaves once Lichess has ended the game (`resignAndLeave()`), and after a Quit the window closes then:
  the window's `gameInfoChanged` calls `leaveFlow.gameInfoChanged()` after `route()`, and `quitRequested` closes it.
- `HomePage.qml` — account menu; Continue: the only unfinished game directly, two or more:
  `ContinueGamesPage` (`ChessGame.continueGames()`); action cards, among them **Games** (Ctrl+O): `GamesPage`.
- `ContinueGamesPage.qml` — every unfinished game: the kept local games first as `GameCard`s
  (`GameLibrary.unfinishedGames`), then the Lichess ones as `OngoingGameCard`s.
- `GamesPage.qml` — the games library, objectName `library`: every kept game as a `GameCard`, Open (a replay) or
  Continue, Export PGN, Delete; the filters All · vs Robot · Two players · Lichess · Unfinished. A kept Lichess game
  (LI-14) is only opened: its card's menu (a right click, the Menu key) has Open on lichess.org, Open the analysis
  on lichess.org and Copy link, as `OngoingGameCard`'s. `games.js`: the words for a kept game (a Lichess one:
  "GM Magnus (2850)", "Rapid 10+5 · Rated · you play White · …"), shared with Home and the Continue list.
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
  and in More. A Lichess game from Quick pairing (`ChessConnector.CanSeekAgain`, LI-16) has **New opponent** in
  New game's place, on the result card and in the moves panel (`ChessConnector.newOpponent()`; busy "Looking for
  an opponent…" while `LichessClient.seeking`; the page stays until the seek's game replaces it; Ctrl+N too). The review bar's **Play from here** (`MovesPanel`, the sheet handle in a narrow window) → its menu
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
  clock or days per move; Seek off below Rapid), `LeaveOnlineDialog` (in `LeaveFlow`: Leave / Resign / Stay when
  leaving or closing during a running Lichess game).
- Profiles: `CreateUserDialog`, `ImportConflictDialog` (in `ProfileFiles`), `RestoreBackupDialog`.
- Others: `LevelsHelpDialog` (the five computer levels: F1, the robot's avatar), `ApplyMovesDialog` (moves typed
  in UCI: `ChessConnector.applyMoves`).

## Tooling
- `ScreenshotScene` — the `--screenshot` scenes (the `run-chess-app` skill); `SceneInput` plays them with the
  mouse.
