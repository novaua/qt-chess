# LI-02 Game Clocks and Lichess Speeds — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-04 |
| **Build** | `develop` with #119–#122 (Release) |
| **Plan** | [`LI-02-ClockPlan.md`](LI-02-ClockPlan.md) |

Screenshots can't show these: time stands still in screenshot mode, nothing makes a sound, and a
Lichess game needs the real Lichess. Write the result in the last line of each scenario: **OK**, or
what happened instead (and the time, so the app log can be found: *Settings → About → Open log
folder*).

Already checked by scenes, no need to play them: the draw on a flag against a lone king
(`clock-flag-draw`), a flag while the promotion piece is chosen (`clock-promotion`), a stalemate
(`stalemate`).

**Two questions only these tests can answer:** does Lichess end a flagged game by itself (scenario
13), and how far does the shown time drift from Lichess's (scenario 9)?

## Part 1: on this computer

No Lichess needed. Part 1 takes about 15 minutes.

### 1. Two players: the clock runs and switches

1. Home → **Two players** → clock **3+2** → Start game.
   - **Expected**: both cards show `03:00`, nothing lit, nothing counting.
2. Play 1. e4, then 1... e5.
   - **Expected**: nothing counts after White's first move. After Black's, White's clock lights up and
     counts.
3. Play 2. Nf3.
   - **Expected**: White's clock stops with 2 s added, Black's lights up and counts.

Result:

### 2. Two players: low time and the flag

1. **Two players** → clock **1+0** → Start game. Play one move each, then wait.
   - **Expected at 10 s**: the running clock turns red and **a low-time sound plays once**: two soft,
     falling notes. Tell me if it sounds wrong, too loud, or too quiet.
   - **Expected under 10 s**: tenths (`00:09.4`).
2. Let it run out.
   - **Expected**: the result card says "<name> ran out of time", and that king has the flag.
3. Click **Rematch**. **Expected**: the same 1+0.

Result:

### 3. Robot: a whole Bullet game

1. **Play vs computer** → level **3** → clock **1+0** → Start game. Play it out as fast as you can.
   - **Expected**: the robot answers quickly, faster as its time goes down, and its clock doesn't run
     out early. The clocks switch after every move.
2. Note how it ended, and how much time the robot had left.

Result:

### 4. Robot: a flag ends the game for good

1. **Play vs computer** → clock **1+0**. Play one move, then let your time run out.
   - **Expected**: "<your name> ran out of time". The robot plays nothing more. **Undo is greyed
     out**, and Ctrl+Z does nothing.
2. Go Home and start a game vs the robot with **No clock**. Make a move, then press Ctrl+Z.
   - **Expected**: Undo works. (The flag of the last game is forgotten.)

Result:

### 5. Robot: Undo doesn't add time

1. **Play vs computer** → clock **30+20**. Play three moves. Note your time.
2. Make a move (20 s are added), let the robot answer, press Ctrl+Z. Repeat twice.
   - **Expected**: your time is **not** higher than it was, and it has gone down by what you spent.

Result:

### 6. Robot: review doesn't stop the clock

1. **Play vs computer** → level **5** → clock **5+0**. Play three moves.
2. Make a move and press **Left** at once, while the robot thinks.
   - **Expected**: the robot's answer brings you back to the live position, and your clock runs.
3. Press **Left** again and stay there.
   - **Expected**: your clock keeps counting while you review.

Result:

### 7. Robot: Continue

1. **Play vs computer** → clock **5+0**. Play three moves. Note both times. Click **Home**.
2. Click **Continue game**.
   - **Expected**: both times as you left them. After about a second your clock starts counting.
3. Do it again, and this time press **Left** right after Continue.
   - **Expected**: your clock still starts after about a second, while you review.

Result:

## Part 2: on Lichess

### Setup

- **You**: Chess++ with a profile that has a linked Lichess account (Profile → Link Lichess).
- **The friend**: a second Lichess account in a browser (another browser or a private window,
  signed in to lichess.org). Keep it next to the app, so you can compare the clocks.
- The challenges are casual: they don't change anyone's rating.

### 8. The challenge card

1. Home → **Play online**. Look at **Challenge a friend**.
   - **Expected**: Real time is selected. Blitz tiles are dim, with a line saying why, and 15+10 is
     selected. "Your Rapid rating: …" shows your rating (compare with your Lichess profile).
2. Click a Classical tile, then **Days per move**, then **Unlimited**.
   - **Expected**: the rating follows: Classical, Correspondence, then nothing.
3. Back to **Real time**. Type a name in **Lichess username**.
   - **Expected**: the Blitz tiles light up, and the button says "Challenge <name>".
4. Pick a Blitz tile, then clear the name.
   - **Expected**: the tile stays selected but dim, and the button is off.
5. Type `open`. **Expected**: a red line says that isn't a username, and the button is off.

Result:

### 9. Rapid by link: the clock in a real game

1. Real time **10+5**, no username → Create challenge. Accept with the friend's link.
   - **Expected**: both cards show `10:00`, nothing counts until both have moved (as in the browser).
2. Play a few moves.
   - **Expected**: your move switches the clock at once. The increment shows up.
3. Compare the two clocks in the app with the browser, just after a move and again 30 s later.
   - **Write down the differences.** This is the drift. A second or so is expected.

Result:

### 10. A takeback

1. In the game of scenario 9, click **Propose takeback**. Accept it in the browser.
   - **Expected**: the board goes back, the clocks match the browser, and the right one runs.

Result:

### 11. Blitz by name

1. Real time **5+3**, username = the friend's → Challenge.
   - **Expected**: the dialog says "Challenge sent to <name>", "Waiting for <name> to accept…", no link.
2. Accept in the browser (the friend's notifications).
   - **Expected**: the game starts here, as Blitz, with the clock `05:00`, playing as the side you picked.

Result:

### 12. No, and no such player

1. Challenge the friend by name, and decline in the browser.
   - **Expected**: the dialog closes, and a message says "<name> declined the challenge".
2. Challenge the friend, **Cancel challenge**, then at once press the button again.
   - **Expected**: the new dialog stays open.
3. Cancel that one. Challenge a name that doesn't exist.
   - **Expected**: an error on the Online page, and the card is usable again.

Result:

### 13. A flag online

Needs Blitz by name (3+0), so it's short.

1. Start a game. Play two moves each. Then **let your own time run out**, with the friend's browser open.
   - **Expected**: your clock stops at `00:00.0`, then the game ends: "<you> ran out of time".
2. Rematch or start again. This time let the **friend's** time run out, in the browser.
3. Once more, let your time run out with the **friend's browser closed**.
   - **The open question**: does Lichess end the game by itself? Write down how long it took, or that
     it never ended. There's no flag call in the Board API, so the app can only wait.

Result:

### 14. Low time on a rejoin

1. In a 5+3 game, let your time go below the red line (37 s), on your turn.
2. Click Home → Leave, then Continue the game on the Online page.
   - **Expected**: the low-time sound plays about a second after the page opens, not while it opens.

Result:

### 15. Correspondence by name

1. **Days per move** → 3, username = the friend's → Challenge.
   - **Expected**: no waiting dialog. A message says it was sent and that the game will be under Your games.
2. **Close the app.** Accept in the browser. Open the app, Home → Play online → Refresh.
   - **Expected**: the game is under Your games. The challenge survived the app closing.
3. Continue it.
   - **Expected**: `3 days` on the card of the side not to move, `2 days 23 hours` or so on the other.
     No red, no tenths. Make a move: your card goes back to `3 days`.

Result:

## Not tested here

- **A move Lichess refuses**: there's no reliable way to cause one. If you ever see "Move failed", note
  what the clock did.
- **A move lost on the way** (a timeout): a known gap, see `PROGRESS.md`.
