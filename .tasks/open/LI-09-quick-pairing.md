---
id: LI-09
title: "Quick pairing"
type: task
status: in_progress
priority: medium
---

# LI-09 Quick pairing

Plan: [`docs/LI-09-QuickPairingPlan.md`](../../docs/LI-09-QuickPairingPlan.md) (the owner's decisions of 2026-11-20: greyed Bullet/Blitz tiles, Custom as on lichess.org, rated by default, the seek goes on across pages with an icon in the command bar to cancel it). Five steps, one PR each.

<!-- migrated from TASKS.md:306 (Lichess Improvement Backlog) -->

- [ ] LI-09 Quick pairing [Medium], **not started; needs a plan** (the owner, 2026-11-19). Play a random opponent of about the player's rating, as lichess.org's **Quick pairing** tiles do, without a link to share or a name to type. Today a Lichess game starts only from a challenge: to a friend by link, to a named player, or one received (BL-10).
  - **Board API** (to confirm in the plan): `POST /api/board/seek` makes a public seek, with `time` + `increment` (minutes + seconds), or `days` for correspondence. Its parameters:
    - `rated`;
    - `variant` (`standard`, `chess960`: LI-01A);
    - `ratingRange` (e.g. `1300-1700`).
    - A real-time seek lasts as long as its HTTP connection stays open: closing it cancels the seek.
    - When someone takes it, the game comes as a `gameStart` on the event stream, which the app already handles (challenges, BL-10) and which opens the game as today.
    - A correspondence seek returns at once with the seek's id.
  - **The Board API's limit:** like challenges (LI-02), seeks may only be **Rapid, Classical or Correspondence**; Bullet and Blitz are for the browser and bots. So of lichess.org's tiles only these work here: 10+0, 10+5, 15+10, 30+0, 30+20, plus Correspondence (days per move). In the plan:
    - Bullet and Blitz tiles either aren't shown, or are shown greyed with "Open on lichess.org" (recommended: the familiar grid, honest about what this app can do);
    - a **Custom** tile: the challenge card's `TimeControlPicker` limited to what is allowed.
  - **The UI:**
    - a **Quick pairing** section at the top of the Online page (after the challenges to the player): the tiles as on lichess.org, "10+0 Rapid", with the player's rating in that pool;
    - a Rated / Casual switch;
    - the Variant row (`AppSettings.onlineChess960`);
    - optionally a rating range around the player's rating;
    - perhaps also a Home card or the "Play online" card's menu.
  - **While it waits:** clicking a tile starts the seek. The tile shows a ring and "Looking for an opponent… 0:12" (`BusyButton`, BL-16); a Cancel closes the seek. Only one seek at a time.
  - **What ends the wait:**
    - leaving the page keeps the wait, shown on Home's bar, with Cancel; closing the app or signing out cancels it;
    - an incoming challenge accepted meanwhile cancels the seek first;
    - not during a running real-time Lichess game (as `CanAcceptChallenge`).
  - **Errors:** Lichess's refusal (a 400 for a speed the Board API doesn't allow, a rate limit 429, a provisional account's rating range) as a message, not a silent stop.
  - **C++:** `LichessClient::seek(…)` / `cancelSeek()`, `Seeking`, `seekFailed`; the seek's connection held open and closed on cancel (a `QNetworkReply` kept, aborted to cancel); `ChessConnector` only through the existing `gameStart` → game start.
  - **Tests:**
    - `FakeLichess` takes a seek, holds the connection, and starts a game after a while (a `gameStart` on its event stream), or is cancelled;
    - `LichessClientTest`: cancel closes the connection, a 400 / 429 is reported;
    - `tst_lichess`: a tile starts a game, Cancel stops the wait;
    - scenes `quick-pairing`, `quick-pairing-waiting`, narrow;
    - a hand test on lichess.org (a real opponent; a second account seeking the same tile makes it quick).

## Resolution

### 2026-11-20 — step 1: LichessClient's seek (branch: features/li09QuickPairing_201126)
- `LichessClient::startSeek(limitSec, incrementSec, days, rated, variant)` / `cancelSeek()`, the `seek` property (`seekState()`, `seekChanged`), `seekFailed`, `seekSent` (correspondence), `speedOf`. The method is `startSeek`, not the plan's `seek`: QML can't have a method and a property of one name.
- The real-time seek's POST held open (`_seekReply`, aborted to cancel); its game: the first `gameStart` of source `lobby` / `pool`, of the seek's speed, not a game known when it began; the fallback when Lichess closes the request: Your games polled for a new real-time game of that speed, else after 5 s "No opponent was found: Lichess ended the seek." Ended by `cancelSeek`, any game starting (`streamGame`), `acceptChallenge` / `acceptRematch`, a new token, the destructor; not by `stopStream`.
- Tests: `ChessQtTests/LichessSeekTest.cpp` (17) against `FakeSeekLichess` (`FakeLichess.h`; `FakeLichess::dropped`: the connections the client closed). All 327 ChessQtTests pass (Debug); ChessGui builds.
- Left: steps 2–5 (the Online page, the wait beyond the page, Custom, docs and hand tests).
