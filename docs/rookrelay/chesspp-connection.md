# Connecting Chess++ to rookrelay

How to change Chess++ (`qt-chess`) so that it plays through **rookrelay**, our own online-chess server, and how to check it. "rookrelay" is a working name.

- **For:** a developer, or Claude Code, working in `qt-chess`. Nothing else is needed.
- **Chess++ read at:** branch `develop`, commit `69034d727ee85791d1835fbaa71cd32dfe32c304`. All files and line numbers below are at that commit.
- **Server behaviour checked:** 26.11.26. A server release from before that day is older: if the `gameFull` line of section 4 has no `myColor`, it seats a White player as Black, and it also lacks the answers to `accept` and the flairs of section 5.2.
- **The rule for every change:** without the new start options, Chess++ behaves exactly as today with lichess.org.

## 1. In short

rookrelay answers the calls of Lichess's Board API that Chess++ makes, with the same paths and JSON. Chess++ cannot use it yet, because the app cannot be pointed at another server.

| Change | What | Needed for | Size |
|---|---|---|---|
| **C1** | A server address, also for sign-in | anything at all | small |
| **C2** | Game links of that server | pasting and copying links | small |
| **C3** | No Bullet and Blitz limits on that server | fast games from the Online page | small |
| **C4** | Two extra notices (optional) | clearer play | small |

Do C1 first. The others can wait and do not depend on each other. C2 and C3 use the "own server" switch that C1 adds.

**The server side of link games and quick pairing is ready.** What is missing is in section 5.3.

## 2. Addresses

Chess++ needs two addresses:

| | API, flair images and sign-in | Web pages (game links) |
|---|---|---|
| A server running on the same PC | `http://127.0.0.1:3000` | `http://localhost:4321` |
| A server on the network | `http://<rookrelay-host>` | the same |

- `<rookrelay-host>` is a placeholder. Ask the server's owner for the real name or IP address, or to start the server on your PC (it has its own repository).
- The server writes its game links with **one fixed web address**, whatever address you called it by. The `url` in the answer of `challenge/open` (section 4) shows it. If you reach the server another way, for example by IP address while the links carry a name, give Chess++ the address from the links as the web address (C2).
- It is **plain HTTP**. The token travels unencrypted, so use it only on a network you trust.
- Is it up? `GET <API>/x/health` answers `{"ok":true,"version":"…","database":"ok"}`.

## 3. Signing in

Every call carries `Authorization: Bearer <token>`, as Chess++ does today. Two kinds of rookrelay token work:

| Kind | How to get it | Notes |
|---|---|---|
| Guest | `POST <API>/x/guest` with no body. Answer: `{"token":"…","user":{"id":"…","username":"Player","kind":"guest"}}` | Enough for all testing. Shown only once. Does not expire. Games are never rated. Every guest is called `Player`. 30 new guests an hour per address (the default). |
| Personal API token | the web page `<web>/me`, "API tokens" | Needs an account, which today only the server's owner can set up for you. Does not expire. Plays games, and cannot touch the account. An older server release may not offer it. |

**In the app** (after C1): start with a new data folder, choose "Create local profile", open the profile page, "Advanced: use an API token", paste, "Link". The first-run button "Continue with Lichess" signs in through a browser, which rookrelay does not offer.

**Keep the two worlds apart.** A rookrelay token must never go to lichess.org, and a Lichess token must never go to rookrelay. C1 says how.

A token that no longer works is answered with 401, and Chess++ then shows "sign-in expired", as it does today.

## 4. Check the server with `curl`

Run this before changing code. It needs no account and no browser: two guests, `W` and `B`, play a short game. Use Git Bash.

```bash
S=http://127.0.0.1:3000                 # or http://<rookrelay-host>
curl -s $S/x/health

curl -s -X POST $S/x/guest              # run twice, and keep both tokens
W=<token from the first answer>
B=<token from the second answer>
curl -s -H "Authorization: Bearer $B" $S/api/account

# W opens a game without a clock and takes White
curl -s -X POST -H "Authorization: Bearer $W" -d "color=white&variant=standard" $S/api/challenge/open
G=<"id" from the answer>

# B joins and watches: a gameFull line, then the command stops after 8 s (curl exit code 28)
curl -s -X POST -H "Authorization: Bearer $B" $S/api/challenge/$G/accept
curl -s -N --max-time 8 -H "Authorization: Bearer $B" $S/api/board/game/stream/$G

curl -s -X POST -H "Authorization: Bearer $W" $S/api/board/game/$G/move/e2e4
curl -s -X POST -H "Authorization: Bearer $B" $S/api/board/game/$G/move/e7e5
curl -s -X POST -H "Authorization: Bearer $W" $S/api/board/game/$G/resign
```

The `gameFull` line carries `"myColor":"black"`, `B`'s seat. Every `POST` after the first guests answers `{"ok":true}`, except `challenge/open`, which answers the challenge with `id`, `url`, `urlWhite` and `urlBlack`. (`color` is rookrelay's own addition: Lichess's open challenge has none.)

## 5. The API

### 5.1 Works as Chess++ expects

`GET /api/account`, `GET /api/account/playing`, `GET /api/stream/event` (`gameStart`, `gameFinish`), `POST /api/challenge/open`, `…/{id}/accept`, `…/{id}/cancel`, `POST /api/board/seek`, `GET /api/board/game/stream/{id}`, `POST /api/board/game/{id}/move/{uci}` (and `…/resign`, `…/abort`, `…/draw/{yes|no}`, `…/claim-victory` on the same game path), `POST /api/users`, `GET /assets/flair/img/<flair>.webp`, `myColor` in `gameFull`, and errors as `{"error":"…"}`. Streams send an empty line every 6 seconds.

### 5.2 Where it differs from Lichess

| Topic | rookrelay | In Chess++ |
|---|---|---|
| Creator's colour | `accept?color=` by the creator of an open challenge sets their side, and starts no game. It works while the colour is open (or already that one). Without `color`, or with the other one, 403 | Works. Chess++ ignores the refusal it gets for a random colour |
| Own started game | `accept` by one of its players answers 404, as on Lichess; anybody else gets 409 | Joining it by a pasted id or link opens the game |
| Clocks | `1+0`, `2+1`, `3+0`, `3+2`, `5+0`, `5+3`, `10+0`, `10+5`, `10+10`, `15+10`, `30+0`, `30+20`, or none | All Chess++ presets are in the list. Another clock gets 400 with the list |
| Chess960, days per move, a start position | not offered, 400 | The message is shown. For Chess960 and a correspondence seek its wording is technical, such as "body must have required property 'time'" |
| Rated games | accounts only, with a clock. A guest gets 403 "Sign in to play rated." | Quick pairing is **rated by default** (`AppSettings.qml:34`): a guest chooses Casual |
| Quick pairing | the request stays open until paired, 408 after 3 minutes. The colours are drawn | Works |
| Timed games | each side has 30 seconds for its first move, or the game is aborted | Be quick when testing with a clock |
| Opponent gone | in a timed game the win can be claimed 60 seconds after the opponent left (`opponentGone`) | Works |
| Named challenge, Rematch | `POST /api/challenge/{username}` is 404, and no challenge events are sent (will come) | The call fails with a message |
| Takeback | 404 (will come) | The button does nothing |
| Players' flairs | `POST /api/users` takes the ids as `Content-Type: text/plain`, as Chess++ sends them (a form body, `curl`'s default, gets 400). It answers only `[{"id":"…","flair":"…"}]`, for registered accounts, 50 ids at most | Guests have no flair, and are not listed |
| Browser sign-in | `/oauth` and `/api/token` are 404 (may come) | Use a token (section 3) |
| Extra fields | `clockPaused`, `ratingDiff`, `expiration`, `casualReason`, `timeControl`, and the line `opponentLeft` | Ignored today; see C4 |
| Limits | 429 after 30 challenges or 120 searches an hour, or with 10 game streams open. 503 while the owner has paused new games | The message is shown, except for a search refused with 429, where Chess++ words its own "Lichess asks to wait a minute before seeking again" (`LichessClient.cpp:966`, `:1000`); here the wait can be up to an hour |

### 5.3 What works when

| State | What you can play |
|---|---|
| With C1 | Link games with either colour, quick pairing, rejoining by a pasted id, the flairs of accounts. Expected from reading both sides, not yet tried with the program |
| Later | Takeback, named challenges and Rematch, perhaps browser sign-in |
| Not planned | Chess960, correspondence, games from a position |

## 6. The changes

Each block can be pasted into a ticket. Addresses and paths are in sections 2 and 5.

### C1. Point Chess++ at another server

**Why.** `LichessClient::setBaseUrl` and `setAssetBaseUrl` exist (`LiChessConnector/LichessClient.h:139`, `:141`; defaults `:400-401`), but the app calls the first only for a screenshot scene (`ChessGui/ChessGui/main.cpp:464`). Sign-in, a pasted token too, uses fixed lichess.org addresses (`LiChessConnector/LichessAuth.cpp:17-19`, used at `:77`, `:143`, `:174`). And a profile keeps one token (`ChessGui/ChessGui/UserManager.cpp:331-343`) that goes to whatever address the client has (`ChessGui/ChessGui/appservices.cpp:73-88`).

**Do.**
1. `ChessGui/ChessGui/main.cpp`: a start option `--server <address>` beside `--data-dir` (`:514`, `:530`). Accept it **only when `--data-dir` is given too** (test the option itself: a screenshot run makes a temporary folder, `:546-554`), and only as `http` or `https` with a host and optional port, no path; drop a trailing `/`. Otherwise stop with an error, like a bad command line (`:533-536`).
2. `AppServices`: take the address before `start()` (`ChessGui/ChessGui/main.cpp:615`), set the client's base **and** asset base with it, and hand it to `UserManager`.
3. `LichessAuth`: make the three fixed addresses paths on a settable base that defaults to `https://lichess.org`. `UserManager::startLichessAuth` (`UserManager.cpp:397-401`) sets it.
4. An "own server" property that QML can read (C2 and C3 use it too). When set, hide the browser sign-in: "Continue with Lichess" (`qml/WelcomePage.qml:51-61`), "Reconnect" (`qml/ProfilePage.qml:128-132`) and "Link Lichess account" (`:214-221`). The token field stays.
5. **Keep each data folder to one server**, before the services are made (`ChessGui/ChessGui/main.cpp:567`). With `--server`: refuse the app's own default folder (what `AppConfig::appDataDir()` gives before `setAppDataDir`, `ChessGui/ChessGui/AppConfig.cpp:19-26`); in a folder with no profiles yet (no `users.json`), record the address in a small file there; refuse a folder that has profiles and no record, or a record of another address. Without `--server`: refuse a folder that has the record. Say why when refusing.

Step 5 matters: `--data-dir` alone does not protect anything. Given your normal folder, `--server` would send your Lichess token, unencrypted, to that address, and a rookrelay folder opened without `--server` would send its token to lichess.org.

**Done when.**
- Without `--server`, nothing differs for a folder with no record, and the tests pass.
- A test shows each refusal of step 5.
- A test signs in with a token against the fake server (`ChessGuiTests/testsetup.cpp:58-63` already points the client there).
- By hand, from the build's folder:

```powershell
.\ChessGui.exe --data-dir D:\chess-rookrelay --server http://127.0.0.1:3000
```

1. Run section 4 up to the first guests. Sign in with token `B` (section 3). Linking succeeds: lichess.org would refuse this token, so it went to rookrelay.
2. Create a game as `W` with the `challenge/open` command of section 4 (White).
3. In Chess++: "Play online", "Join a game", paste the id. The game starts with you as **Black**.
4. Move as `W` with `curl`, answer in Chess++, then resign as `W`. Chess++ shows that you won.

### C2. Game links of that server

**Why.** A pasted link is recognised only for lichess.org, and every link Chess++ makes points there. The web address can differ from the API address (section 2). rookrelay's links are `<web>/<id>`, `<web>/<id>/white` and `<web>/<id>/black`; an id is 8 letters or digits.

**Do** (only when "own server"):
1. A start option `--web <address>`, accepted only with `--server`; the default is the server address. Give the client one function that builds a game's page address. The kept-game record (`gamelibrary.cpp`) and the PGN writer (`gamepgn.cpp`) have no client, so hand them the address.
2. Reading: `gameIdFromUrl` and `colorFromUrl` (`LichessClient.cpp:1871-1894`) also take the web address's host and port, escaped for the pattern. Two things are in the way: `colorFromUrl` is `static` (`LiChessConnector/LichessClient.h:259`), so hand it the address (a parameter that defaults to lichess.org keeps the tests as they are), and both build their pattern in a function-local `static`, which would keep the first address for the whole run.
3. Building: use that function instead of the fixed address at `chessconnector.cpp:781-785`, `gamelibrary.cpp:51-54`, `gamepgn.cpp:67-70` and `qml/OngoingGameCard.qml:14`.
4. Open the web address instead of lichess.org at `qml/QuickPairingGrid.qml:50` and `qml/OnlinePage.qml:562`.
5. Hide what rookrelay has no page for: analysis (`qml/GamePage.qml:338-345`, `qml/GamesPage.qml:157-162`) and the public profile (`qml/ProfilePage.qml:198-203`).
6. Reword "Open on lichess.org" (`qml/GamePage.qml:334`, `qml/GamesPage.qml:153`, `qml/OngoingGameCard.qml:24`, `:61`, `qml/OnlinePage.qml:249`, `qml/ChessGame.qml:664`).

**Done when.** The link tests (`ChessQtTests/LichessClientTest.cpp:32-53`) still pass, with new ones for `http://localhost:4321/<id>/black`. By hand, add `--web http://localhost:4321`: a `urlBlack` from `challenge/open` pasted into "Join a game" joins as Black, and "Copy link" (the right-click menu of an unfinished game's card) gives an address that opens in a browser (on your own PC the server's web part must be running).

### C3. No Bullet and Blitz limits on that server

**Why.** Lichess forbids Board API apps the fast speeds, and Chess++ enforces that in its interface. rookrelay has no such rule: every clock of section 5.2 may be played, sought and challenged. (The Seek dialog's sliders can still make a clock that is not in that list, which gets the 400.)

**Do** (only when "own server"):
1. `playableHere` (`LichessClient.cpp:350-358`, used at `:398` and `:829`): every speed is playable. The variant check stays.
2. `qml/QuickPairingGrid.qml:24-25`: no tile is `lichessOnly` (the greyed look, tooltip and name follow from it).
3. `qml/SeekDialog.qml:37`: never `tooFast`.
4. `qml/OnlinePage.qml:377`: `blitzAllowed` always; the sentence at `:346` goes.
5. `qml/TimeControlPicker.qml:20`: an online challenge offers Bullet too.
6. Optional: hide the online Chess960 and "Days per move" choices, which the server refuses.

**Done when.** On rookrelay the Bullet and Blitz tiles start a search, and an open challenge offers `1+0` and `3+2`; without `--server` they are greyed as today. By hand: click `1+0` with Casual chosen, then pair it as a second guest with

```bash
curl -s -X POST -H "Authorization: Bearer $W" -d "time=1&increment=0" $S/api/board/seek
```

### C4. Two extra notices (optional)

**Why.** rookrelay says two things Lichess does not. Chess++ ignores them safely today.

1. **`clockPaused: true`** in a `gameState` (also in `gameFull.state`), after a server restart: the clocks wait until both players are back. **Do:** report the clock as not running while it is set (`updateClock`, `LichessClient.cpp:1399-1406`).
2. **`{"type":"opponentLeft","forfeitAt":1795718256489}`** on the game stream: the opponent can no longer play (their account was banned or deleted), and the game will end for you at that time (milliseconds since 1970) if nobody ends it sooner. `"forfeitAt":null` takes the notice back. **Do:** read it beside `opponentGone` (`LichessClient.cpp:1346-1350`), pass it to `ChessConnector` (compare `chessconnector.cpp:1546`) and show a message (`qml/MessageCenter.qml`).

**Done when.** Tests feed both lines through the fake server: the clock stands still while paused and runs at the next state without the flag; the notice appears and goes away.

## 7. Example lines

Real lines, for the fake server's tests. They are not one game: the first starts a `3+0` game, the second ends a game **without** a clock (so its times are 0; such a game has `"clock":null` in its `gameFull`), the third is the opponent-gone line.

```json
{"type":"gameFull","id":"azu55lzv","myColor":"white","rated":false,"variant":{"key":"standard","name":"Standard","short":"Std"},"clock":{"initial":180000,"increment":0},"speed":"blitz","perf":{"name":"Blitz"},"createdAt":1795689363261,"white":{"id":"7zjzw9ny","name":"Player","title":null,"rating":1500,"provisional":true},"black":{"id":"8iwx94y6","name":"Player","title":null,"rating":1500,"provisional":true},"initialFen":"startpos","state":{"type":"gameState","moves":"","wtime":180000,"btime":180000,"winc":0,"binc":0,"status":"started","expiration":{"idleMillis":250,"millisToMove":30000}}}
{"type":"gameState","moves":"e2e4 e7e5","wtime":0,"btime":0,"winc":0,"binc":0,"status":"resign","winner":"white"}
{"type":"opponentGone","gone":true,"claimWinInSeconds":60}
```
