# LiChessConnector/

lichess.org as an online server (LI-18, plan `docs/LI-18-OnlineServersPlan.md`): a Qt plugin,
`x64\<config>\online\LichessOnline.dll`, Qt Core + Network, on `OnlineCore` (`OnlineCore/CLAUDE.md`: the Board API client, `OnlineClient`, which does all
the playing). Here only what Lichess does its own way. Its tests: `ChessQtTests/` (`Lichess*Test.cpp`, against
`FakeLichess`).

## Files
- `LichessServer` — Lichess's `OnlineServer`: id `lichess`, name "Lichess"; `https://lichess.org` (API, game
  pages), `https://lichess1.org` (the flairs); its links (`gameUrl`, `gameIdFromUrl`: `lichess.org/<id>`,
  `…/<id>/white`, `…/game/<id>?color=black`, a bare 8-character id; `colorFromUrl`); `playsVariant` (standard,
  chess960); `canPlay`: the Board API's restrictions (Rapid, Classical, Correspondence; Blitz only from a friend,
  the API or the AI; never Bullet); the seek's 429 wording; `createAuth` → `LichessAuth`.
- `LichessAuth` — Lichess's `OnlineAuth`: OAuth with PKCE in the browser (a local `QTcpServer` takes the
  callback, `board:play challenge:write`), or a personal API token (`useToken`); both end with the account's
  username from `/api/account`.
- `LichessPlugin` (`lichess.json`: id `lichess`, order 10) — the plugin `OnlineHub` loads: `createServer`.
- `LichessClient` — an `OnlineClient` with a `LichessServer` of its own: the Lichess tests'.

## Invariants
- Nothing here plays: a Lichess behaviour of the Board API (a stream, a challenge, a seek) goes into
  `OnlineClient`; only what another server would do otherwise goes here.
- Nothing includes this folder's headers or links the plugin: `ChessGui` and `ChessGuiTests` only build it
  (`LinkLibraryDependencies` false), so it can be left out; `ChessQtTests` compiles the classes it tests
  (`LichessServer`, `LichessAuth`, `LichessClient`). `scripts\Check-OnlineIsolation.ps1` checks it.
