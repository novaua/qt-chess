# LI-04 Rejoin Unfinished Lichess Games — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-03 |
| **Build** | `develop` with #111–#114 (Release) |
| **Plan** | [`LI-04-RejoinPlan.md`](LI-04-RejoinPlan.md) §6 |

Screenshots can't show these: they need a real Lichess game. Write the result in the last line of
each scenario: **OK**, or what happened instead (and the time, so the app log can be found:
*Settings → About → Open log folder*).

## Setup

- **You**: Chess++ with a profile that has a linked Lichess account (Profile → Link Lichess).
- **The friend**: a second Lichess account in a browser (another browser or a private window,
  signed in to lichess.org).
- **No saved robot game**, except in scenario 5: if Home shows "Continue game", open it and finish or
  resign it first.
- A challenge from the app: Home → **Play online** → Challenge a friend → **Real time 10+5**
  (or **Days per move** in scenario 6) → Create challenge, then open the link in the friend's browser
  and accept.

## Scenarios

### 1. Leave a running game with Home

1. Start a game (Setup). Play two moves each.
2. Click **Home** (top left).
   - **Expected**: "Leave the game?" with **Leave** (highlighted), **Resign**, **Stay**.
3. Click **Stay**. **Expected**: the game goes on, nothing changed.
4. Press **Esc**. **Expected**: the same dialog. Press **Esc** again: it closes (Stay).
5. Click Home, then **Leave**.
   - **Expected**: Home, with the card **"Continue online game"** · "1 Lichess game · your turn" (or
     "· waiting for your opponent"). The friend's browser shows that you left; your clock runs there.

Result:

### 2. Continue on Home

1. After scenario 1, click **Continue online game** on Home.
   - **Expected**: the **Play online** page. Under **Your games**, a card with the board as it is,
     the friend's name (rating), "Rapid · 9 min" (time left), and **Your turn** (accent colour) or
     **Waiting**. The subtitle says "1 game · your turn in 1" or "1 game".
2. Let the friend move in the browser, then click **Refresh**.
   - **Expected**: the board shows the friend's move; the card says **Your turn**.

Result:

### 3. Continue the game

1. On the card from scenario 2, click **Continue** (or the small board).
   - **Expected**: the game page opens **at once in the current position** (no slides, all moves in
     the list), with **one** move sound, not one per move. Your side is at the bottom.
2. Play a move; let the friend answer.
   - **Expected**: both moves show as usual (slide, sound); the game goes on to the end normally.

Result:

### 4. Close the window during a game

1. Start a new game (or continue one), play a move each.
2. Close the window (the X, or Alt+F4).
   - **Expected**: **"Quit during the game?"** with **Quit**, **Resign**, **Stay**.
3. Click **Stay**. **Expected**: the app stays open, the game goes on.
4. Close again, then **Quit**. **Expected**: the app quits.
5. Start Chess++ again.
   - **Expected**: Home shows **Continue online game**; it leads to the Online page and the game;
     Continue rejoins it as in scenario 3.

Result:

### 4b. Resign from the warning (Home and window close)

1. In a running game, click **Home**, then **Resign**.
   - **Expected**: the result shows for a moment (you lost by resignation), then Home. The friend's
     browser shows the resignation. Profile → statistics vs Lichess: **one more loss**.
2. Start another game, close the window, then **Resign**.
   - **Expected**: the same, then the app quits. After a restart, the statistics show that loss too
     and Continue doesn't list the game.
3. Optional: pull the network cable (or turn Wi-Fi off) and Resign. **Expected**: after about 5 s it
   leaves (or quits) anyway; that loss isn't counted in the app (Lichess still has it once you're back
   online).

Result:

### 5. A saved robot game and an online game

1. Play vs the computer for a few moves and go Home (it is saved automatically).
2. Start a Lichess game, play a move each, and **Leave** it.
   - **Expected**: Home's card says **"Continue game"**, and its description ends with
     "· 1 Lichess game in Play online".
3. Click **Continue game**. **Expected**: the **robot** game opens.
4. Go Home, click **Play online**. **Expected**: the Lichess game is listed under Your games.
5. In the online game, use the menu → **Switch user**.
   - **Expected**: "Switch user?" says the game keeps running on lichess.org and that you can come
     back from Continue or Play online.

Result:

### 6. A correspondence game over a day

1. Create a challenge with **Days per move → 2**. The friend accepts. Play a move each, then Leave
   (no warning needed to test here: Leave is fine).
2. **The next day**, start Chess++.
   - **Expected**: Continue lists it; on the Online page the card says "Correspondence · 2x h"
     (hours left: "47 h" just after a move, about "23 h" a day later; up to two days it counts hours).
3. Click **Continue**. **Expected**: the game opens in the current position; a move is sent.

Result:

### 7. A game started on lichess.org

1. In the browser, with **your own** account (the one linked in Chess++), start a game against the
   friend from lichess.org (e.g. challenge them to **Rapid 10+5**, and once more **Blitz 5+3** from the
   lobby or as a challenge).
2. In Chess++, Home → Play online (or Refresh).
   - **Expected**: both games are listed. The Rapid one has **Continue**, which rejoins it. A game the
     Board API can't play (Bullet, lobby Blitz, Chess960) has **Open on lichess.org** and the note
     "This kind of game can't be played in Chess++ yet"; the button opens it in the browser.

Result:

### 8. The opponent ends the game while you're away

1. Start a game, play a move each, **Leave** it.
2. In the friend's browser, **resign**.
3. In Chess++, on the Online page, click **Refresh** (or go back to Home).
   - **Expected**: the game is gone from Your games; Home's Continue card is hidden (when there are no
     other unfinished games and no robot game).
4. Optional: the same, but let your own clock run out while you're away (a 10+0 game, wait 10 min).
   **Expected**: gone from the list too.

Result:

## Known limits (not bugs)

- There is **no clock** on the board (LI-02): check the time left on the Online page before Continue.
- The list doesn't update by itself: Refresh, or go back to Home (no polling, no notifications;
  BL-10).
