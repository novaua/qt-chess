# LI-04 Rejoin Unfinished Lichess Games — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-03, rewritten 2026-11-19 |
| **Build** | `develop` after #194 (Release) |
| **Plan** | [`LI-04-RejoinPlan.md`](LI-04-RejoinPlan.md) §6 |

Rewritten on 2026-11-19 for the app as it is now: Home's Continue and the Continue list (BL-17), the
challenges and Your games kept up to date by Lichess's event stream (BL-10), the games library (BL-19:
every local game is kept, so "a saved robot game" is a kept unfinished game), the clocks on the board
(LI-02) and Chess960 on Lichess (LI-01A). None of these tests was run on the version of 2026-11-03.

Screenshots can't show these: they need a real Lichess game. Write the result in the last line of
each scenario: **OK**, or what happened instead (and the time, so the app log can be found:
*Settings → About → Open log folder*).

## Setup

- **You**: Chess++ with a profile that has a linked Lichess account (Profile → Link Lichess).
- **The friend**: a second Lichess account in a browser (another browser or a private window,
  signed in to lichess.org).
- **No unfinished local game**, except in scenario 5: open Home → **Games**, filter **Unfinished**,
  and delete them (or continue and finish them). Otherwise Home's Continue opens the Continue list
  instead of the Lichess game.
- **No other unfinished Lichess game** either: Home → Play online → **Your games** (at the end of the
  page) is empty; finish or resign any there on lichess.org.
- A challenge from the app: Home → **Play online** → **Challenge a friend** → **Real time** → the
  **10+5** tile (or **Days per move** in scenario 6) → **Create challenge**, then open the link in the
  friend's browser and accept. The game page opens by itself in Chess++.

## Scenarios

### 1. Leave a running game with Home

1. Start a game (Setup). Play two moves each.
   - **Expected**: both clocks on the player cards, the running one counting down.
2. Click **Home** (top left).
   - **Expected**: "Leave the game?" ("The game goes on at lichess.org and your clock is running. You
     can come back to it from Continue or Play online.") with **Leave** (highlighted), **Resign**,
     **Stay**.
3. Click **Stay**. **Expected**: the game goes on, nothing changed.
4. Press **Esc**. **Expected**: the same dialog. Press **Esc** again: it closes (Stay).
5. Click Home, then **Leave**.
   - **Expected**: Home at once, with the card **"Continue online game"** · "1 Lichess game · your
     turn" (or "· waiting for your opponent"). The friend's browser shows that you left; your clock
     runs there.

Result: OK, all as expected.

### 2. Your games on the Online page

1. After scenario 1, click **Play online** and scroll to **Your games**, the last section of the page.
   - **Expected**: one card with the board as it is (your side at the bottom), the friend's name
     (rating), "Rapid · 9 min left" (the time left), and **Your turn** (accent colour) or **Waiting**.
     The section's subtitle says "1 game · your turn in 1" or "1 game".
2. Let the friend move in the browser, then click **Refresh** under the cards.
   - **Expected**: the board shows the friend's move; the card says **Your turn**. (A move doesn't
     update the list by itself: only Refresh, or coming back to Home or to this page.)

Result: 1. as expected, can continue waiting and Your turn are present. 2. Ok, as expected.

### 3. Continue the game

1. Go **Back** to Home and click **Continue online game**: with only this one game it rejoins it
   directly, without a list.
   - **Expected**: the card shows a ring and "Connecting to Lichess…", then "Starting the game…",
     then the game page **in the current position** (no slides, all moves in the list) with **one**
     move sound, not one per move. Your side is at the bottom; the clocks show Lichess's times.
2. Play a move; let the friend answer.
   - **Expected**: both moves show as usual (slide, sound); the game goes on.
3. Leave again (Home → Leave), then Play online → Your games → **Continue** on the card.
   - **Expected**: the card's button shows "Starting…", then the game as in step 1. Play it to the
     end normally (or resign in the friend's browser).

Result: 1. yes, direct rejoin, if exit app and continue list is shown. 2. Ok. 3. Ok as expected. 

### 4. Close the window during a game

1. Start a new game (or continue one), play a move each.
2. Close the window (the X, or Alt+F4).
   - **Expected**: **"Quit during the game?"** with **Quit**, **Resign**, **Stay**.
3. Click **Stay**. **Expected**: the app stays open, the game goes on.
4. Close again, then **Quit**. **Expected**: the app quits.
5. Start Chess++ again.
   - **Expected**: Home shows **Continue online game**; clicking it rejoins the game as in
     scenario 3.

Result: 2. as expected. 5. as expected. All ok.

### 4b. Resign from the warning (Home and window close)

1. In a running game, click **Home**, then **Resign**.
   - **Expected**: the result shows for a moment (you lost by resignation), then Home. The friend's
     browser shows the resignation. Profile → **Statistics**, the **Lichess** row: **one more loss**.
2. Start another game, close the window, then **Resign**.
   - **Expected**: the same, then the app quits. After a restart, the statistics show that loss too
     and neither Home's Continue nor Your games lists the game.
3. Optional: pull the network cable (or turn Wi-Fi off) and Resign. **Expected**: after about 5 s it
   leaves (or quits) anyway; that loss isn't counted in the app (Lichess still has it once you're back
   online).

Result: 1. OK, one more loss. 2. No Lichess game at continue or the games history.

### 5. A kept robot game and an online game

1. Play vs the computer for a few moves and go Home (the game is kept by itself).
2. Start a Lichess game, play a move each, and **Leave** it.
   - **Expected**: Home's card says **"Continue a game"**; its description names the robot game,
     then "· 1 Lichess game" (", your turn" when it is).
3. Click **Continue a game**.
   - **Expected**: the **Continue a game** page: the robot game's card first, then the Lichess game's
     card ("Lichess · Rapid · … left"), and "2 unfinished games" above them.
4. Click **Continue** on the robot game. **Expected**: the robot game opens as it was left.
5. Go Home, click **Play online**. **Expected**: the Lichess game is listed under Your games (only
   it: the robot game isn't there).
6. Continue the Lichess game, then use the More menu (⋯) → **Switch user**.
   - **Expected**: "Switch user?" says "You leave the Lichess game; it keeps running on lichess.org.
     You can come back from Continue or Play online."

Result: 2. OK as expected two games. 3. Yes robot card first, OK, 4. OK, 5. OK. 6. as expected.

### 6. A correspondence game over a day

1. Create a challenge with **Days per move** → **2**. The friend accepts. Play a move each, then
   Leave (Leave is fine here).
2. **The next day**, start Chess++.
   - **Expected**: Home's Continue offers it; on the Online page the card says "Correspondence ·
     2x h left" (hours left: "47 h" just after a move, about "23 h" a day later; up to two days it
     counts hours).
3. Click **Continue**. **Expected**: the game opens in the current position, with the time left for
   the move instead of a running clock; a move is sent.

Result: 2. 47 h and 23 h as expected. 3. time left is OK.
  Noted UI bug, card 'Challenge a friend' [10] ans [14] buttons, going out of this card, alo 'Lichess username (optional) input extends out of the parent frame/card.

### 7. A game started on lichess.org

1. In the browser, with **your own** account (the one linked in Chess++), start games against the
   friend from lichess.org: challenge them to **Rapid 10+5**; once more **Blitz 5+3** from the lobby
   (Quick pairing, so it isn't a direct challenge); and once a **Crazyhouse** game (any time control).
2. In Chess++, Home → Play online → Your games. (The list updates by itself when a game starts on
   lichess.org; if a game is missing, click **Refresh**.)
   - **Expected**: all the games are listed. The Rapid one has **Continue**, which rejoins it. A game
     the Board API or Chess++ can't play (lobby Blitz, Bullet, Crazyhouse) has **Open on lichess.org**
     with the tip "This kind of game can't be played in Chess++ yet"; the button opens it in the
     browser. (A Chess960 Rapid game **can** be continued since LI-01A.)

Result: 2. Yes, and unsupported contains a link, would be nice to be able on right button to copy the link not just open.

### 8. The opponent ends the game while you're away

1. Start a game, play a move each, **Leave** it.
2. In the friend's browser, **resign**.
3. In Chess++, look at Home (or the Online page).
   - **Expected**: within a few seconds, without Refresh, the game is gone from Your games, and
     Home's Continue card is hidden (when there are no other unfinished games). Lichess's event stream
     says the game ended.
4. Optional: the same, but let your own clock run out while you're away (a 10+0 game, wait 10 min).
   **Expected**: gone from the list too.

Result: 3. OK, as expected.

## Known limits (not bugs)

- A move the opponent makes while you're away doesn't update Your games by itself: Refresh, or go
  back to Home (only games that start or end come by Lichess's event stream).
- Lichess games aren't kept in the games library (the owner's choice): they are only on lichess.org.
