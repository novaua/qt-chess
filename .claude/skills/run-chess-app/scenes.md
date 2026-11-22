# Screenshot scenes

The scenes of `scripts\Take-Screenshot.ps1 -Scene <name>` (`ScreenshotScene.qml` `setUp()`; the profiles, Lichess
link and statistics from `seedScreenshotProfiles()` in `main.cpp`). Read this file when choosing a scene or adding one.

- Scenes: `welcome` (first run), `create-user` (first run, Create profile dialog), `profiles` (Who's
  playing? with Bob, Alice and Tester, who has a Lichess account), `home`, `continue` (Home with a kept robot game and the `online` scene's four Lichess games: "Continue a game", which opens the list), `continue-online` (no robot game, four Lichess games), `continue-list` (BL-17: the Continue list over Home, the robot's game first, then the four Lichess games; also at `1400x900` and `640x560`), `continue-list-online` (the list without a robot game), `continue-list-robot` (the robot's game alone, as when the Lichess games end while the list is open), `games` (BL-19: the Games page with seven kept games of Tester on fixed days, main.cpp, one of them Chess960 #3; also at `640x560` and `1400x900`), `games-filter` (the same, "Unfinished" chosen), `games-empty` (no games yet), `chess960` (LI-01A: a two-player game vs Bob at Chess960 #3, "Chess960 #3" in the title; also at `1400x900`), `chess960-select` (#3 after 1. e4 e5 2. g3 b6, the king selected: dots on e2, g2 and its rook), `capture` (BL-21: after 1. e4 e5 2. Qh5 Nc6 the queen selected: the dot's colour in the corners of e5, f7, h7, dots elsewhere), `capture-ep` (after 1. c3 d5 2. a3 d4 3. e4 Black's d4 pawn selected: c3 and e3 en passant marked, a dot on d3), `chess960-castled` (then O-O, king and rook swapped; also at `640x560`), `computer-960` (the vs-computer setup with Chess960 chosen: the Variant row and its line), `play-from-here` (BL-19 step 4: the vs-computer setup from the reviewed position after 5. Nf3 of a two-player game: "Play from here vs computer"), `play-from-here-menu` (step 4b: the same review, the review bar's Play from here button with its menu open; also at `640x560`: the sheet handle, the menu above it), `replay` (the two-player game opened as a replay of the kept game, after 5. Nf3: "Move 9 of 20" and Play from here in the review bar; also at `640x560`), `analysis` (BL-19 step 7: a kept game with a made-up engine analysis (main.cpp) opened as a replay at 10… Bxc2??: the marks in the move list, the summary with "Blunder · Best was Bb4", the eval bar left of the board; also at `1400x900` and `640x560`), `game` (two-player
  game, Tester vs Bob), `leave-online` (the game with the leave warning of a running Lichess game: Leave / Resign / Stay), `review` (a middlegame reviewed at move 5), `result` (Fool's mate: the result
  card), `ended` (the same with the card closed: Rematch / New game in the moves panel, Result in the sheet), `moves` (the middlegame with the move sheet open; narrow window only), `select` (1. e4 e5, the g1 knight selected: tinted square and its move dots; with `-Black` on the turned board),
  `drag` (the same knight held over f3: faded on g1, dots, f3 outlined; `-Black` too), `drag-off` (the knight dragged off the board, over the bottom player card: drawn over it), `settings` (Settings page, top),
  `about` (Settings scrolled to About: version, licences, shortcuts), `restore` (Settings with the restore
  dialog: two backups, the newest the same as now), `levels`,
  `profile` (Profile page: Lichess linked with ratings, statistics, the rating vs the robot from the `games` scene's two rated robot games: 1227?), `profile-local` (no Lichess),
  `profile-expired` (Lichess sign-in expired: Reconnect), `import` (importing Bob again: the conflict
  dialog), `online` (the Online page with four seeded games, at its end since BL-17: your turn / waiting, a Blitz lobby game to open on lichess.org), `online-empty` (signed in, no games), `online-disconnected` (no Lichess account), `computer` (vs-computer setup dialog), `players`
  (two-player setup dialog), `players-many` (seven opponents: with `-Size 640x560` the dialog scrolls),
  `clock` (a two-player game with its clock running), `clock-low` (red, tenths), `clock-flag` (out of time),
  `clock-flag-draw` (out of time against a lone king: a draw), `clock-promotion` (out of time while the piece
  is chosen: the pawn goes back), `clock-continue` (a robot game continued, its clock running), `stalemate`,
  `clock-online` and `clock-correspondence` (a Lichess game fed to the stream parser, no network),
  `online-blitz` (the challenge card with a name: Blitz lit), `online-challenge` (waiting for that player),
  `online-960` (LI-01A stage 2: the challenge card with Chess960 chosen, its line and "Your Chess960 rating"),
  `online-days` (BL-23: Days per move, 14 days and a username, all inside the card; also `online-days@640x900` in `all`, the narrowest window),
  `quick-pairing` (LI-09: the Online page's Quick pairing tiles, Rapid and Classical lit, Bullet and Blitz greyed),
  `quick-pairing-waiting` (a 10+0 seek waiting 0:12: the tile, its line and the command bar's indicator; offline,
  nothing is sent), `quick-pairing-indicator` (Home while a 10+0 seek waits 0:42, the `SeekIndicator`'s menu open;
  at `640x560` the icon without the time), `seek-dialog` (Custom: a 20+0 Rapid seek), `seek-dialog-blitz` (5+3:
  Seek off, the reason shown), `seek-dialog-days` (3 days per move); the first three and `seek-dialog` also at
  `640x560` in `all`,
  `premove`, `premove-capture`, `premove-played`, `premove-dropped` (LI-05: a Lichess game fed to the stream parser, no network:
  Bf1-c4 waiting in red and the d2 pawn's premove dots; `premove-capture` (BL-21): the f3 knight instead, its premove onto e5's pawn in the premove dot's colour, in the corners; Black's Nc6 comes and Bc4 is played (our moves aren't
  posted in screenshot mode); e4-e5 waiting, gone after Nc6), `rejoin` (BL-18: a Lichess game joined with ten moves, grabbed the moment its page is up, no `--delay`: the position after 5… a6, never the start position), `reconnecting` (LI-06: the same game with its
  connection lost: the caution bar), `challenge` (BL-10: two Lichess challenges to Tester on Home, fed to
  the event stream's parser: the bar with Accept / Decline / Hide and "+1 more"), `challenge-unplayable`
  (a Bullet challenge: Open on lichess.org instead of Accept), `challenge-ingame` (a real-time Lichess
  game on the board, then a challenge: Decline only, "Finish your game first"), `online-challenges` (the
  Online page's "Challenges to you": three, one of them Bullet, SlowPlayer's a Chess960 one), `online-game-menu` (LI-10: Your games scrolled into view, the Blitz game's menu open: Open on lichess.org, Copy link), `online-challenges-decline` (the same with
  the first one's Decline… menu open: Lichess's reasons), `captures` (BL-13: Black down to a lone king,
  White lost six: a full captured strip and "+29"), `check` (the page opens in check: "Check" over the command bar), `draw-offer` (a Lichess game, the opponent offers a draw: Accept / Decline over the command bar; at 640x560 it doesn't fit: the InfoBar), `home-message` ("Profile imported" over Home's command bar), `messages` (the recent messages open: an open draw offer, an error, the connection, a decline), `opponent-left` (the Lichess opponent left: the countdown, frozen at 12 s), `opponent-left-claim` (Claim victory, on the opponent's turn; at 640x560 it doesn't fit: the InfoBar), `opponent-left-turn` (the player's turn: no Claim victory, "your move, then claim victory"; at 640x560 the text is cut short, not the InfoBar), `opponent-left-error` (a refused claim's error in the command bar's place for its 4 s, the opponent-left message waiting behind it), `messages@640x560` (the recent messages kept inside a small window), `continue-starting`, `continue-list-starting`, `challenge-accepting` (BL-16: Lichess, a local server that never answers, hasn't answered yet: Home's Continue card "Connecting to Lichess…", Friend's card "Starting…", the challenge bar's "Accepting…"; the ring is a still clock in the scenes), `starting` ("Starting the game…" as while a game page is built, over the vs-computer dialog that started it: the notice is on the Overlay layer, above popups; the scenes themselves skip it), `more-menu` (BL-11: the game's More menu open; it is made on its first opening),
  `all`.
- **Sized scenes** (BL-13): `scene@WxH[@black]` (quoted: `-Scene 'game@1400x900'`) runs a scene at its own window size, saved as
  `<scene>[-black][-dark]-<WxH>.png`. `all` ends with the list in `$sizedScenes`
  (`Take-Screenshot.ps1`): the player column at `1920x1080` and `1400x900` (`game`, `clock-online`,
  `captures`, `review`, `ended`, `result`, `challenge-ingame`, `drag@black`, `reconnecting`, `check`:
  the bars between the column's cards), and `game` at `1100x900`, `960x1040` and `1000x560` (no room
  for a bar between the cards), which must keep the cards above and below the board. `premove-robot`: 1. e4 vs the robot, Nf3 premoved
  and played after its answer (1. e4 e5 2. Nf3 Nc6). `robot`: a saved game vs the robot at level 5, continued, where the robot mates
  with Qh5# (the engine's answer is played on the GUI thread). These two and `play` run against **FakeUci** (BL-05 step 3,
  `$engineScenes` in `Take-Screenshot.ps1`: the game it follows), so they are the same every time and in `all`; `-Stockfish`
  runs them against the real engine (their pictures then vary). FakeUci not built: they are skipped with
  a warning, the other scenes still taken. The script puts the shell's own `CHESS_ENGINE_PATH` back.
  `timing` (not in `all`, BL-11; since BL-11 step 2 the board's layers are made in the first frame's polish (`LayoutGate`), so most of the build shows as `first frame`, not `page`): from Home, 1.5 s apart, vs Robot, leave, Continue, leave, two players, leave, vs Robot as Black left while the engine thinks, then Settings and the Online page opened and closed (`[Timing] Settings: N ms to open`); run with `-KeepLog` (keeps `<picture>.png.log` also on success) and read its `[Timing]` lines (and, with `$env:QT_LOGGING_RULES='chess.frames.debug=true'`, "GUI thread busy"). Its picture is not a baseline.
  `play` (not in `all`, BL-11 step 3; FakeUci unless `-Stockfish`): a game vs the robot (level 1) played with mouse events (`SceneInput`: clicks and a drag through the board's handlers), animations on: Nf3 by clicks, the robot's answer, e4 by a drag, a takeback, the review; `[Play]` lines mark the steps (run with `-KeepLog`, the frame trace, or under `qmlprofiler`). Not a baseline.
  `slides` (not in `all`): the BL-14 frame-trace scene — animations on (every other scene forces them
  off), selects the g1 knight and plays Nf3, Nf6, Bc4, Nc6 0.4–1 s apart, so a run with
  `$env:QT_LOGGING_RULES='chess.frames.debug=true'` and `-Delay 5200` gets `[Frame trace]` lines in the
  `.png.log` (frame gaps over 25 ms, the work of the dots/pieces/list). Its picture is an arbitrary
  frame, never a baseline.
  `takeback` (not in `all`, LI-03 check): a Lichess game fed to the stream parser by `main.cpp` (no
  network): four moves, then the list after an accepted takeback of two; the shot shows 1. e4 e5 and
  "2 moves taken back".
  `login`, `menu` and `board` still work as the old names of `profiles`, `home` and `game`.
  The profile scenes get their profiles, Lichess link (a placeholder token, no network) and statistics
  from `seedScreenshotProfiles()` in `main.cpp`.
  `-Moves` replaces a scene's own moves.
