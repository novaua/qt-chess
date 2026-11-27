# LI-18 — Online servers as plugins (Lichess, rookrelay): plan

The ticket: [LI-18](../.tasks/open/LI-18-online-servers-as-plugins.md). Chess++ plays online only on lichess.org.
**rookrelay**, our own server, answers the same Board API calls with the same paths and JSON
([`docs/rookrelay/chesspp-connection.md`](rookrelay/chesspp-connection.md), "the connection doc" below). This plan
makes "an online server" a plugin: the Board API's behaviour is written once and shared, what differs between
servers lives in that server's plugin, and either plugin can be removed without breaking the app or the other.
Line numbers are from `develop` at 4ce226f.

## Goals and rules

1. **Shared once.** Challenges (link, named, rematch), quick pairing, the game stream, game actions, ongoing
   games, the event stream and the players live in one shared library. No server copies them.
2. **Different in one place.** Whatever a server does its own way (addresses, sign-in, links, allowed clocks and
   variants, its pages, wording) is answered by that server's plugin. QML and `ChessGui`'s C++ ask the plugin
   what it offers (its *traits*), never "is this Lichess?".
3. **Removable.** Without the rookrelay plugin, Chess++ is today's Lichess app. Without the Lichess plugin, it
   plays on rookrelay. With none, the online parts are hidden and local play works. Removing a plugin loses
   no data: its accounts and kept games stay in the profile.
4. **A token only goes to the server that issued it.** It is stored with that server's id and address, and
   only that server's client gets it. This replaces the connection doc's `--server` option and its
   one-server-per-data-folder rule (its C1 step 5).
5. **Lichess users see no change**, apart from the profile page's account section (step 4). Each refactor step
   is compared with `develop`'s screenshots.

## What is there today

- **`LiChessConnector`** (DLL, 3 000 lines): `LichessClient` (2 089 lines in the `.cpp`) is almost all the Board
  API as such. The Lichess-only parts are few:
  - the addresses: `setBaseUrl` / `setAssetBaseUrl` (`LichessClient.h:141`, `:143`), the defaults at `:421-422`,
    and the fixed lichess.org addresses in `LichessAuth.cpp:18-20`;
  - what may be played: `playableHere` (`LichessClient.cpp:351-359`, used at `:399`, `:842`), `playableVariant`;
  - the game links: `gameIdFromUrl`, `colorFromUrl` (`:1910-1933`; `colorFromUrl` is `static`);
  - the wording: "Lichess didn't answer" (`noAnswerMessage`), "Lichess asks to wait a minute…" (`:979`, `:1013`);
  - the sign-in: `LichessAuth`, OAuth with PKCE in the browser, or a pasted token.
- **`ChessGui`'s C++:** `ChessConnector` holds one `LichessClient*` (122 mentions of Lichess in the `.cpp`,
  `onLichess*` slots, `LichessGameUrl`). `UserManager` keeps one token per profile (`lichessToken`,
  `lichessUsername`, `lichessExpired`, `lichessRatings` in `users.json`, `UserManager.cpp:705-730`, `:953`).
  `AppServices` makes one client and feeds it that token (`appservices.cpp:73-88`). `GameLibrary` keeps a
  finished game as kind `"lichess"` with a `LichessGameInfo` (`gamelibrary.h:57-135`); `GamePgn` writes its
  lichess.org Site. `main.cpp`'s screenshot scenes drive `LichessClient` directly.
- **QML:** 13 files call `LichessClient.` (most: `OnlinePage`, `OngoingGamesRefresh`, `QuickPairingGrid`), 35
  files say "Lichess" in their text.
- **rookrelay's differences** (the connection doc, section 5.2): its own address, plain HTTP, guest tokens and
  personal tokens but no browser sign-in, links `<web>/<id>[/white|/black]`, every clock of a fixed list (Bullet
  and Blitz included), standard chess only, no correspondence, no analysis or profile page, a rematch as an
  ordinary challenge, two extra stream lines (`clockPaused`, `opponentLeft`), other rate limits.

## 1. The shape

```
ChessGui (QML, ChessConnector, UserManager, GameLibrary)   knows only OnlineCore
        │
        ▼
OnlineCore.dll      OnlineClient (the Board API), OnlineStream, OnlinePlayer, RematchInfo, OnlineGameInfo,
                    OnlineHub (the plugins, the clients),
                    interfaces: OnlineServer, OnlineAuth, OnlineProviderPlugin
        ▲  implemented by, loaded at start from <app>\online\*.dll
        ├── LichessOnline.dll     LichessServer, LichessAuth (OAuth with PKCE, token)
        └── RookrelayOnline.dll   RookrelayServer, RookrelayAuth (guest, token)
```

- `ChessGui` never includes a plugin's header and doesn't link a plugin. A plugin includes only `OnlineCore`.
  Plugins don't know each other.
- A third server that speaks the Board API is one more plugin, without rebuilding the app.
- A server that does **not** speak the Board API (e.g. a server of our own with another protocol, TSK-13) is out
  of scope. If it ever comes, `OnlineClient` becomes an interface with the Board API as one implementation; the
  QML side of this plan stays as it is.

## 2. OnlineCore: the shared behaviour

A new DLL project `OnlineCore` (Qt Core + Network, `ONLINECORE_EXPORT`), mostly moved from `LiChessConnector`:

| Today | In OnlineCore |
|---|---|
| `LichessClient` | `OnlineClient`: the same API, its Lichess-only spots replaced by calls on its `OnlineServer` |
| `LichessStream` | `OnlineStream` |
| `LichessPlayer`, `RematchInfo` | `OnlinePlayer`, `RematchInfo` |
| `LichessGameInfo` (ChessGui) | `OnlineGameInfo`: with the server's id and the game's page address, so a kept game needs no plugin |
| token encryption (`encryptToken`) | `OnlineToken` (DPAPI, as now) |

- **One `OnlineClient` per linked account**: each has its own token, event stream ("one per token" stays true per
  server), ongoing games, challenges and seek. It has a `server` property (the `OnlineServer`, for QML).
- **The connection doc's C4** (`clockPaused`, `opponentLeft`) goes into `OnlineClient`: Lichess never sends
  those lines, so sharing them costs nothing.
- The messages take the server's name: "%1 didn't answer", "Takeback failed: …" as now.

### `OnlineServer`: what differs

An abstract `QObject` (so QML reads it); one per loaded plugin and address.

| Member | Lichess | rookrelay |
|---|---|---|
| `id()` | `lichess` | `rookrelay` |
| `name()` | "Lichess" | "rookrelay" (from the plugin's metadata, so the working name can change) |
| `apiUrl()`, `webUrl()`, `assetUrl()` | lichess.org, lichess.org, lichess1.org | the account's address; the web one defaults to it |
| `gameUrl(id, color)` | `https://lichess.org/<id>[/black]` | `<web>/<id>[/white\|/black]` |
| `gameIdFromUrl(text)`, `colorFromUrl(text)` | lichess.org links, a bare id | links of its web address, a bare id (8 letters or digits) |
| `canPlay(speed, source, variant)` | the Board API's limits (no Bullet, Blitz only from a friend, the API or the AI); standard, chess960 | every clock of its list; standard |
| `rateLimitMessage(kind)` | "Lichess asks to wait a minute before seeking again" | "rookrelay allows 120 searches an hour…" |
| `createAuth(parent)` | `LichessAuth` | `RookrelayAuth` |
| `traits()` | below | below |

**Traits** (an `OnlineTraits` `Q_GADGET`, `client.server.traits.x` in QML). QML and `ChessConnector` decide by
these, never by `id()`:

| Trait | Lichess | rookrelay | Used by |
|---|---|---|---|
| `browserSignIn` | true | false | Welcome, Profile (connection doc C1 step 4) |
| `guestSignIn` | false | true | Profile, Welcome |
| `tokenSignIn`, `tokenPageUrl` | true, lichess.org/account/oauth/token | true, `<web>/me` | Profile |
| `serverAddress` | false (fixed) | true (asked for, plain HTTP warned) | Profile |
| `fastSpeeds` | false | true | QuickPairingGrid, SeekDialog, OnlinePage, TimeControlPicker (C3) |
| `clocks` | `[]` (any) | the 12 clocks | SeekDialog snaps its sliders to them |
| `variants` | standard, chess960 | standard | VariantPicker |
| `correspondence` | true | false | the "Days per move" choices |
| `guestsRated` | (no guests) | false | Quick pairing's Rated switch for a guest |
| `gamePage`, `analysisPage`, `profilePage` | true | true, false, false | "Open on …", Analyse, the public profile (C2 step 5) |
| `rematchOffers` | true | false | the rematch bar expects no offer |

### `OnlineAuth`: signing in

An interface with `startBrowserLogin()`, `useToken(token)`, `startGuest()`, `cancel()`, and
`loginSucceeded(token, username, accountKind)` / `loginFailed(message)`. Each plugin's version takes its addresses
from its server, so a token is checked only where it will be used (the connection doc's C1 step 3 by design).
`LichessAuth` is today's, made an implementation. `RookrelayAuth`: `GET /x/health` first (a clear "no rookrelay
server at this address"), then `POST /x/guest`, or `GET /api/account` with a pasted token.

### The plugin interface and `OnlineHub`

```cpp
// OnlineProviderPlugin.h (OnlineCore)
class ONLINECORE_EXPORT OnlineProviderPlugin
{
public:
	virtual ~OnlineProviderPlugin() = default;
	// A server for `address` ("" = the plugin's default); a child of `parent`.
	virtual OnlineServer* createServer(const QString& address, QObject* parent) const = 0;
};
#define OnlineProviderPlugin_iid "org.chessplusplus.OnlineProvider/1"
Q_DECLARE_INTERFACE(OnlineProviderPlugin, OnlineProviderPlugin_iid)
```

Each plugin's class: `Q_PLUGIN_METADATA(IID OnlineProviderPlugin_iid FILE "lichess.json")`, with
`{ "id": "lichess", "name": "Lichess", "order": 10, "coreVersion": 1, "defaultAddress": "https://lichess.org" }`.

`OnlineHub` (OnlineCore, a `QObject`):
- `loadPlugins(dir)`: `QPluginLoader` over `<app>\online\*.dll`. It reads the metadata first and skips, with a
  line in the log, a plugin of another `coreVersion`, a second one with the same id, or one that fails to load.
  `addPlugin(plugin)` adds one from code (the tests, without files).
- `providers`: the loaded plugins, sorted by `order` (id, name, traits).
- One `OnlineClient` per account of the active profile whose plugin is loaded; made and dropped as profiles
  switch and accounts are linked. An account whose plugin isn't loaded stays in `users.json` untouched.
- For QML, the singleton **`Online`** (instead of `LichessClient`): `providers`, `clients`, `current` (the
  server the Online page shows; the last one is remembered), `client(id)`, and `incomingChallenges` /
  `ongoingGames` of every client together, each item with its `server` id, so Home's challenge notices and the
  Continue list show both servers.
- **One seek at a time, across servers:** a seek, an accept or a real-time challenge on one client ends a seek on
  another (the invariant of `LiChessConnector/CLAUDE.md`, widened).

## 3. ChessGui

### Accounts in a profile (`UserManager`)

- `users.json`: `"accounts": [{ "server": "lichess", "address": "https://lichess.org", "token": "<encrypted>",
  "username": "…", "kind": "user", "expired": false, "ratings": {…} }]`, at most one per server id.
- **Migration:** a profile with the old `lichessToken` fields gets a `lichess` account. For one release the old
  fields are written too, so an older Chess++ still finds the Lichess sign-in. Covered by a test.
- **The token's address:** the hub gives a client the token only when the account's `address` is the server's.
  Changing the rookrelay address means signing in again (a guest token is lost then, as a guest is).
- API: `accounts` (QML, without tokens), `linkAccount(serverId, method, tokenOrAddress)`, `cancelLink()`,
  `unlinkAccount(serverId)`, `setAccountState(serverId, username, ratings | expired)`; `accountToken(serverId)`
  is C++ only. The `lichess…` members go (QML moves in the same step).
- Exports carry no token of any server (the invariant, widened).
- Statistics: the kind `"lichess"` stays for Lichess games; a rookrelay game counts as `"rookrelay"`, after it in
  the kinds' order (`UserManager.cpp:174`).

### `ChessConnector`

- `setLichessClient` → `setOnlineHub`. The game's client is the one whose `gameStarted` came (`_gameClient`);
  game signals of another client are ignored while it plays. One online game at a time, as now.
- `onLichess*` → `onOnline*`; `LichessGameUrl` → `OnlineGameUrl`; new `OnlineServer` (the server of the game on
  the board or of the kept game replayed, else null), from which QML takes the name and traits.
- `acceptChallenge(serverId, challengeId)`, `continueOnlineGame(serverId, gameId)`: the item's server.

### `GameLibrary`, `GamePgn`

- A finished online game is kept as kind `"online"` with an `OnlineGameInfo` (`server`, `serverName`, `gameUrl`
  stored when kept). An old `"lichess"` record is read as `server: "lichess"`. The record id stays
  `<server>-<gameId>`, so Lichess's are the same files (`lichess-<id>`).
- A kept game whose plugin is gone is listed, replays, and still has its link (from the record).
- PGN: the Site from `gameUrl`; the Event's wording from `serverName`.

### `AppServices`, `main.cpp`

- `AppServices` makes the hub, loads the plugins from `<app>\online\`, and wires each client's
  `accountFetched` / `authExpired` to `UserManager` by server id. `offline` (screenshots, UI tests) goes to every
  client.
- The screenshot scenes use the Lichess plugin's client; a scene whose plugin isn't loaded is skipped with a line.
- No `--server` option: the address is part of the account (rule 4).

### QML

- `LichessClient.x` → `client.x`, where `client` is `Online.current` (the Online page and its parts) or the
  game's (`ChessConnector.OnlineServer`'s client). Text takes `server.name`: "Open on %1", "%1 didn't answer",
  the leave dialogs.
- The connection doc's C2 step 5 and C3 become traits (the table above), so they apply to any server.
- **Online page:** a server switch at the top when 2+ servers are loaded; with one, no switch (as today); with
  none, Home has no Play online and the Online page can't be opened.
- **Profile page:** "Online accounts", one row per loaded server: its state, ratings, Link (browser, token,
  "Play as guest", as its traits say), the address field where `serverAddress`, Unlink. A Lichess-only profile
  sees one row instead of today's Lichess block.
- **Welcome page:** "Continue with Lichess" from `browserSignIn`, "Play on rookrelay as a guest" from
  `guestSignIn`, one button per loaded server that has either.
- Renames: `LichessChallenges.qml` → `OnlineChallenges.qml` (and `qml.qrc`).

## 4. The rookrelay plugin

Only the differences; everything else is OnlineCore's.

- **Address:** asked for on the Profile page; the plugin's default is `http://127.0.0.1:3000` (the connection doc,
  section 2). The web address for links is the API one unless the server says otherwise (`url` in
  `challenge/open`'s answer, kept per account). A plain `http` address shows "The token travels unencrypted: use
  it only on a network you trust."
- **Sign-in:** guest (`POST /x/guest`) or a personal token (`<web>/me`). A guest plays casual only: the Rated
  switch is off for a guest account (`guestsRated`).
- **Links, clocks, variants, pages, rematch:** the traits and `OnlineServer` members of section 2.
- **Rate limits:** its own wording (a search refused with 429 can wait up to an hour).

## 5. Proving removability

- **Build:** `ChessGui.vcxproj`, `ChessGuiTests.vcxproj` and `ChessQtTests.vcxproj` reference `OnlineCore` only.
  `LichessOnline.vcxproj` and `RookrelayOnline.vcxproj` reference `OnlineCore` and write to `x64\<cfg>\online\`;
  the solution builds them before the tests. `ChessGui.pro` (Qt Creator) gets the same `SUBDIRS`.
- **Tests with and without:** the UI tests run a file with no plugin (Online hidden, a profile's Lichess account
  kept, a kept Lichess game replays with its link), the Lichess files as today, and a rookrelay file.
- **A check:** `scripts\Check-OnlineIsolation.ps1` (run by `Run-UiTests.ps1`): no `LichessOnline` or
  `RookrelayOnline` header included outside its folder, and no `"lichess"` / `"rookrelay"` string in
  `ChessGui\ChessGui\*.cpp|h|qml` except the migration code (an allow list in the script).
- **Installer:** `release.yml` copies `x64\Release\online\*.dll` (`:106`, `:120`). Optional installer components
  per server can come later.

## 6. Tests

- **ChessQtTests:** the `Lichess*Test.cpp` move to `OnlineClient` with the Lichess server (they link
  `LichessOnline`'s import library, which a test may). `FakeLichess` becomes `FakeBoardApi`, with a rookrelay mode
  (`/x/health`, `/x/guest`, the clocks list, `opponentLeft`, `clockPaused`). New: `OnlineHubTest` (load, skip a
  wrong `coreVersion` or a second id, an account without its plugin), `RookrelayServerTest` (links, clocks),
  `UserManagerAccountsTest` (migration, the old fields still written, a token never sent to another address:
  the fake counts the `Authorization` headers it gets), `GameLibraryTest` (an old `"lichess"` record).
- **UI tests:** every Lichess test passes unchanged in what it checks; new: the no-plugin start, a rookrelay link
  game against `FakeBoardApi`, the Online page's server switch, a guest's Rated switch.
- **Screenshots:** steps 2 and 3 equal `develop`'s (stash, `-Update` from `develop`, compare). New scenes:
  `profile-accounts`, `online-servers`, `online-rookrelay`.
- **By hand:** `docs/LI-18-ManualTests.md`, on lichess.org (nothing changed) and on a rookrelay server (the
  connection doc's section 4 with `curl` as the opponent, then section 5.3's list).

## Steps (one PR each)

| # | Step | Visible change | Size |
|---|---|---|---|
| 1 | This plan and the ticket | none | docs |
| 2 | **OnlineCore extracted:** the new project, `LichessClient` → `OnlineClient` with the `OnlineServer` and `OnlineAuth` interfaces; `LichessServer` and `LichessAuth` in `LiChessConnector`, still linked directly; ChessGui's C++ on the `Online…` types. QML untouched (the singleton keeps its name). | none | large, mechanical |
| 3 | **Plugins:** `OnlineHub` with `QPluginLoader`; `LiChessConnector` becomes the `LichessOnline` plugin in `online\`; ChessGui stops linking it; the installer; the no-plugin UI test; the isolation check. QML's `LichessClient` → `Online` (one client), `ChessConnector`'s `Lichess…` members renamed. | none | medium |
| 4 | **Accounts per server:** `users.json` `accounts` and its migration, `UserManager`'s API, the Profile page's "Online accounts", `OnlineGameInfo` in the library and PGN. | Profile page | medium |
| 5 | **rookrelay:** the plugin, its sign-in and address, traits applied in QML (C2, C3), C4 in `OnlineClient`, the Online page's server switch, `FakeBoardApi`'s rookrelay mode. | rookrelay | large |
| 6 | Hand tests on both servers; `CLAUDE.md` for `OnlineCore` and both plugins, the root's tables; close. | none | small |

Steps 2 and 3 change no behaviour, so each is checked by the existing tests and screenshots alone.

## Related tickets

- TSK-13 (the owner, 2026-11-22: fast games on a service of our own): rookrelay is that service; TSK-13's
  "Future direction" is answered by this ticket.
- LI-15 (recent games from the server): written against `OnlineClient`, with a `gameExport` trait (rookrelay:
  false until it has the export).
- LI-01B (Crazyhouse): a variant is a trait per server.
- The connection doc's C1–C4: C1 is replaced by rule 4 and section 3's accounts; C2 and C3 become traits; C4 goes
  into `OnlineClient`.

## To decide (the owner)

1. **Plugins loaded at run time** (`QPluginLoader`, DLLs in `online\`), *recommended*: a server is removed by
   leaving its DLL out (installer, tests), a new one needs no app rebuild. Or **compiled in** (each server a
   static library that registers itself; removing one = dropping its project and one line): simpler, no folder or
   version check, but the app is rebuilt for any change.
2. **A profile linked to both servers at once**, *recommended*: challenges from both, one game at a time. Or one
   server per profile.
3. **The `LI-` prefix** for online play on any server (*recommended*; the root `CLAUDE.md`'s wording changes in
   step 6), or a new prefix for rookrelay.
4. **The connection doc's `--server` and per-folder rule are dropped** for the per-account address (rule 4).
5. **Keep writing the old Lichess fields** in `users.json` for one release (*recommended*), or not.
