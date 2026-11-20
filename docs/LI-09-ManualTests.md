# LI-09 Quick Pairing — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-20 |
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

Result:

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

Result:

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

Result:

## 4. Paired while on another page

1. The app: Casual, **10+5**; **Back** to Home; open **Games** (the library).
2. B takes A's seek from the lobby list.
   - **Expected**: the app opens the game with B from the Games page, without a click.
3. B resigns.

Result:

## 5. A rated seek (not played)

1. The app: **Rated**, Standard, **10+0**.
   - **Expected**: B's lobby list shows A's seek as **Rated**.
2. A cancels it (the tile). Don't let B take it.

Result:

## 6. Chess960

1. The app: Casual, **Chess960** (the card's Variant row), **10+0**.
   - **Expected**: B's lobby list shows it as Chess960.
2. B takes it.
   - **Expected**:
     - the game page's title ends "· Chess960 #N";
     - the pieces behind the pawns are the same as in the browser.
3. B resigns.

Result:

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

Result:

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

Result:

## 9. Lichess refuses

1. In the browser signed in as A, start a real-time game on lichess.org (B challenges A, 10+0). Leave it
   running.
2. The app: Casual, **10+0**.
   - **To find out**: does Lichess refuse the seek (the Online page shows "Quick pairing: <Lichess's
     reason>"), or take it? Note the message.
   - If it waits, cancel it.
3. Finish the browser game (resign).

Result:

## 10. A real opponent (optional, any time)

With B away: A seeks **10+0** (or 15+10) **Rated** and waits, at a busy hour.
- **Note**:
  - how long it waits until someone takes it (or 3 minutes without);
  - the `gameStart` line's `source`;
  - after the game, A's rating change on the Profile page and on the 10+0 tile.
