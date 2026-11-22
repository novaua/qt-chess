# LI-09 Quick Pairing — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-20; results 2026-11-22 |
| **Build** | `develop` with #210–#213 (Release) |
| **Plan** | [`LI-09-QuickPairingPlan.md`](LI-09-QuickPairingPlan.md) (§5 step 5, §6 Risks) |
| **Lichess** | <https://lichess.org/api#tag/Board/operation/apiBoardSeek> |

The automated tests seek against a stand-in for Lichess (`FakeLichess`: `LichessSeekTest`, `tst_lichess`
test_13–15), which pairs a seek when the test says so. Only the real lichess.org shows the following:

- whether a seek from the app is paired with someone using lichess.org's Quick pairing tiles, or only with
  someone who takes it from the lobby's list (Risk 1, how fast a seek pairs);
- what a seek's game carries in its `gameStart` (`source`, Risk 2);
- that Cancel takes the seek off Lichess;
- that a custom clock and a correspondence seek arrive as sent.

Write the result in the last line of each scenario: **OK**, or what happened instead. Add the time, so the app
log can be found (*Settings → About → Open log folder*). Each seek is logged as `LichessClient: seek time=…`.
The game that comes of it is logged as a `gameStart` line; copy its `source` and `speed`.

**You need two Lichess accounts**, as for BL-10:
- **A** is linked to the app's profile.
- **B** is signed in in a browser (a private window works).
- Scenario 8 also needs **A** signed in to lichess.org in another browser.

**Keep the games casual** (the Quick pairing card's **Casual**). Rated games between two accounts of one
person are against Lichess's fair play rules. Scenarios 1 and 5 need a rated seek: in 1 the game is aborted
before the first move, which Lichess doesn't rate; 5 isn't played.

**Where B finds A's seek:** lichess.org's home page, the **Lobby** tab (real time) or the
**Correspondence** tab. Each row shows the player, rating, clock and Rated / Casual; a click joins it.

Already checked by the tests and the screenshot scenes, no need to look again:
- **The card's look:** the Quick pairing card, the greyed Bullet and Blitz tiles, the waiting tile, the
  seek icon and its menu, the Seek dialog with its Blitz reason. Scenes: `quick-pairing`,
  `quick-pairing-waiting`, `quick-pairing-indicator`, `seek-dialog`, `seek-dialog-blitz` and
  `seek-dialog-days`.
- **The seek's request:** `time`, `increment`, `days`, `rated` and `variant` for the tiles and the dialog,
  including `time=0.5`.
- **Its game against the fake server:** the game opens from the Online page or from Home.
- **What ends a seek:** Cancel from the tile, the line or the icon; Back keeps the seek and says so once;
  Switch user ends it. A challenge of ours cancels the seek and blocks a new one; accepting a challenge
  ends it; so does a new token.
- **Errors:** a refusal and a lost connection are reported.

## Results so far (2026-11-22, the owner; real players on lichess.org, account A only)

The owner played Quick pairing against **real Lichess players** rather than with a second account B, so
the scenarios with B (1–9) are only partly covered; scenario 10 is the closest match.

- **Every Rapid tile (10+0, 10+5, 15+10, …)**: a click starts the search; **Cancel** shows and the tile
  waits. Paired after different waits; a game opened each time and played without trouble; won and lost a
  few (scenarios 2 and 10, partly).
- **Cancel** after 30 s and more stopped the search (scenario 3, the app's side; the lobby list not
  checked).
- **Colour:** the player got Black in most of the games. The seek sends no colour, so Lichess picks one
  at random (its seek also takes `color=white|black` if a choice is wanted); a handful of games is within
  chance. Not a bug.
- **Changed after these tests:** the tiles no longer show the player's rating ("1500?", the Lichess rating
  in that pool, provisional): it read as a setting. The tiles show the clock and the speed only, as
  lichess.org's.
- **Not run yet:** 1 (a pool tile on lichess.org meets the seek), 3's lobby side, 4 (paired on another
  page), 5 (rated, B's view), 6 (Chess960), 7 (Custom clock), 8 (correspondence), 9 (Lichess refuses).
  LI-09 stays open until they are.
- **The end of a game** (a later Rapid game, 2026-11-22): the result card offers **Rematch**, **New game**
  (back to the Online page) and **Review moves**, but no **Analyse**, and the game isn't kept here (Lichess
  games have no record, BL-19), so it can't be found or analysed afterwards. lichess.org offers Rematch,
  New opponent and Analysis board. Filed: **LI-14** (analyse a Lichess game), **LI-15** (the last Lichess
  games from the server, 10 at a time with More), **LI-16** (the end of a game: Rematch, New opponent,
  Analyse).
- **Ideas from the tests:** Bullet and Blitz can't be sought through Lichess's Board API (Lichess's own
  site and app play them); a service of our own for fast games is noted in TSK-13. The opponent's avatar in
  a Lichess game: a standard avatar and the Lichess flair (LI-11).

## 1. A pool tile on lichess.org meets the app's seek (Risk 1)

lichess.org's Quick pairing tiles are rated pools, so the app's seek is rated here.

1. The app: **Play online** → Quick pairing, **Rated**, **Standard**. Click **10+0**.
   - **Expected**:
     - the tile shows a ring and the time waited;
     - "Looking for an opponent… 0:0N" with **Cancel** is under the grid;
     - the command bar's clock icon shows the time.
2. In the browser, B clicks lichess.org's **10+0** Quick pairing tile.
   - **To find out**: does Lichess pair B with A within a minute? If it does, the app opens the game with B.
     Note the `gameStart` line's `source` (`pool` or `lobby`).
   - If not within a minute: B cancels on lichess.org, and A clicks the waiting tile to cancel. Then a seek
     from the app waits for someone in the lobby list, which can be slow outside busy hours. Write that down
     for the plan's Risk 1.
3. If a game started: before anyone moves, B clicks **Abort** in the browser.
   - **Expected**: the game ends as aborted in the app too; no rating change on A's Profile page.
4. Set the card back to **Casual**.

Result: **Not run** (needs B on lichess.org's tile). Step 1 seen with real players: the tile waits and **Cancel** shows (2026-11-22).

## 2. B joins A's seek from the lobby list

1. The app: Casual, Standard, **15+10**.
2. In the browser, B opens the **Lobby** tab.
   - **Expected**: A's seek is listed: A's name and Rapid rating, **15+10**, Casual.
3. B clicks it.
   - **Expected**:
     - the app opens the game with B at once: no "Accept", no dialog;
     - the command bar's icon is gone;
     - clocks are 15:00, + 10 s.
   - Log: `gameStart` with `source` `lobby`.
4. Play two moves each; then B resigns.
   - **Expected**: the moves go both ways; the result card.

Result: **Partly OK** with real players instead of B (2026-11-22): the game opened at once each time, moves both ways, games won and lost to the end. Not checked: the lobby row, the `source` in the log.

## 3. Cancel takes the seek off Lichess

1. The app: Casual, **30+0**. B's **Lobby** tab lists it (Classical).
2. Click the waiting **30+0** tile (or **Cancel** under the grid).
   - **Expected**: the app stops waiting; within a few seconds the seek is gone from B's lobby list (reload the
     page if the list doesn't move).
3. Again 30+0; **Back** to Home.
   - **Expected**:
     - "Still looking for an opponent. Cancel it from the icon at the top.";
     - the icon with the time in Home's command bar.
4. Click the icon → **Cancel seeking**.
   - **Expected**: the icon goes; the seek is gone from B's lobby list.
5. Again 30+0, then close the app (the window's ×).
   - **Expected**: the seek is gone from B's lobby list.

Result: **Partly OK** (2026-11-22): Cancel after 30 s and more stopped the search in the app. Not checked: the seek gone from the lobby list, Back with the icon, closing the app.

## 4. Paired while on another page

1. The app: Casual, **10+5**; **Back** to Home; open **Games** (the library).
2. B takes A's seek from the lobby list.
   - **Expected**: the app opens the game with B from the Games page, without a click.
3. B resigns.

Result: Not run yet.

## 5. A rated seek (not played)

1. The app: **Rated**, Standard, **10+0**.
   - **Expected**: B's lobby list shows A's seek as **Rated**.
2. A cancels it (the tile). Don't let B take it.

Result: Not run yet.

## 6. Chess960

1. The app: Casual, **Chess960** (the card's Variant row), **10+0**.
   - **Expected**: B's lobby list shows it as Chess960.
2. B takes it.
   - **Expected**:
     - the game page's title ends "· Chess960 #N";
     - the pieces behind the pawns are the same as in the browser.
3. B resigns.

Result: Not run yet.

## 7. A custom clock

1. The app: Casual, Standard; the **Custom** tile.
   - **Expected**: the Seek dialog, "Casual · Standard, as chosen on the Quick pairing card.", **20+0 · Rapid**.
2. Move the sliders to **5+3**.
   - **Expected**: "5+3 · Blitz" in red; **Seek** is off; the reason is under the sliders.
3. Move them to **20+0**; **Seek**.
   - **Expected**:
     - the dialog closes;
     - the Custom tile shows "20+0", the ring and the time;
     - B's lobby list shows **20+0**.
4. B takes it.
   - **Expected**: the game, clocks 20:00. B resigns.
5. Optional: **½+12** (½ minute, 12 s: Rapid by its increment).
   - **Expected**: B's lobby shows "½+12", or note what Lichess shows or refuses.
   - Cancel from the Custom tile.

Result: Not run yet.

## 8. A correspondence seek

1. The app: Casual, Standard; **Custom** → **Correspondence**, **3** days; **Seek**.
   - **Expected**:
     - "Your seek is on Lichess. When someone takes it, the game shows in Your games. Only lichess.org can
       cancel it." with **Open on lichess.org**;
     - nothing waits in the grid and no icon in the command bar.
2. B's **Correspondence** tab lists it (3 days). B joins it.
   - **Expected**: within seconds the app's **Your games** on the Online page lists the game. If the app was
     on another page, Home's Continue offers it. Continue plays it: B's moves arrive, A's go out.
3. B resigns.
4. Again a correspondence seek (5 days). In the other browser, signed in as A, cancel it from lichess.org's
   Correspondence tab.
   - **Expected**: it is gone from B's list; the app shows nothing of it (it never followed it).

Result: Not run yet.

## 9. Lichess refuses

1. In the browser signed in as A, start a real-time game on lichess.org (B challenges A, 10+0). Leave it
   running.
2. The app: Casual, **10+0**.
   - **To find out**: does Lichess refuse the seek (the Online page shows "Quick pairing: <Lichess's
     reason>"), or take it? Note the message.
   - If it waits, cancel it.
3. Finish the browser game (resign).

Result: Not run yet.

## 10. A real opponent (optional, any time)

With B away: A seeks **10+0** (or 15+10) **Rated** and waits, at a busy hour.
- **Note**:
  - how long it waits until someone takes it (or 3 minutes without);
  - the `gameStart` line's `source`;
  - after the game, A's rating change on the Profile page (the tiles no longer show a rating).

Result: **OK** (2026-11-22, the owner): every Rapid tile (10+x and the others) paired with real players after
different waits (some seeks cancelled after 30 s and more); the games played well, won and lost. Mostly
Black: Lichess's random colour (the seek sends none). Not noted: the `source`, the exact waits, a rating
change. A later Rapid game also played to the end; after it, no Analyse and no way to find
the game again: LI-14, LI-15, LI-16.
