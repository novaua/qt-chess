# BL-10 Lichess Challenges as Notifications — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-09 |
| **Build** | `develop` with #127–#130 (Release) |
| **Plan** | [`BL-10-ChallengesPlan.md`](BL-10-ChallengesPlan.md) |

The automated tests play against a stand-in for Lichess (`FakeLichess`), and the screenshot scenes
feed challenges to the parser. Only a real Lichess account shows that challenges arrive, that the
sound plays, that the taskbar flashes and that Lichess accepts the answers. Write the result in the
last line of each scenario: **OK**, or what happened instead (and the time, so the app log can be
found: *Settings → About → Open log folder*; the challenges are the `event stream line` entries).

**You need two Lichess accounts**: **A**, linked to the app's profile, and **B**, signed in in a browser
(a private window works). B challenges A from A's profile page on lichess.org (`lichess.org/@/A` →
the crossed swords, "Challenge to a game"). Scenario 6 also needs a third account **C**, or a friend.

Already checked by the screenshot scenes, no need to look again: the bar's layout (`challenge`,
`challenge-unplayable`, `challenge-ingame`) and the Online page's list (`online-challenges`).

## 1. A challenge on Home

1. The app on Home, signed in as A. In the browser, B challenges A: **10+5, casual, B plays White**.
   - **Expected within a second or two**: three rising notes, and a bar at the top: "B (rating)
     challenges you · 10+5 · Casual · You play Black", with **Accept**, **Decline**, **Hide**.
     Home's Play online card says "1 challenge waiting for you".
2. Click **Accept**.
   - **Expected**: the game page opens, the board turned for Black, "Lichess · vs B". The browser shows
     the game. Play a move each way to see it's live.

Result:

## 2. In the background

1. Minimise the app (or put another window in front). B challenges A again.
   - **Expected**: the sound, and the app's taskbar button flashes until you bring the window back.
     The bar is there when you do.

Result:

## 3. Decline, with and without a reason

1. Click **Decline** on the bar.
   - **Expected**: the bar goes; in the browser B sees the challenge declined.
2. B challenges again. Home → **Play online**: "Challenges to you" lists it. **Decline…** → "Too fast
   for me".
   - **Expected**: the card goes; B sees the challenge declined because it was too fast.

Result:

## 4. The challenger changes their mind

1. B challenges A; while the bar shows, B cancels it in the browser.
   - **Expected**: the bar goes within a second or two, without a message.

Result:

## 5. A challenge this app can't play

1. B challenges A to **Bullet 1+0**.
   - **Expected**: the bar says "… · Can't be played in this app" and offers **Open on lichess.org**
     instead of Accept. Open on lichess.org opens the challenge in the browser; Decline declines it.

Result:

## 6. During a running real-time Lichess game

1. Start a Rapid game between A (app) and B (browser), play a move each.
2. C (or a friend) challenges A.
   - **Expected**: the bar says "… Finish your game first: it waits on Lichess." with **Decline** and
     **Hide** only, no Accept. The game isn't disturbed.
3. Finish the game (resign in the browser is fastest).
   - **Expected**: if C's challenge still waits, the bar now has **Accept**.

Result:

## 7. During a robot game

1. **Play vs computer**, play a few moves. B challenges A.
2. Click **Accept**.
   - **Expected**: no question; the Lichess game starts. Back on Home later, **Continue** offers the
     robot game where you left it.

Result:

## 8. During a two-player game

1. **Two players**, play a few moves. B challenges A. Click **Accept**.
   - **Expected**: "Leave this game? A two-player game isn't saved. The Lichess game starts instead."
2. **Cancel**: the two-player game goes on, the bar stays. **Accept** again → **Accept challenge**: the
   Lichess game starts.

Result:

## 9. Two at once, and Hide

1. B challenges A twice (two browser tabs; or B and C).
   - **Expected**: the bar shows the first, "(+1 more)". The Online page lists both.
2. On Home, **Hide**.
   - **Expected**: the bar shows the second one. Hide that too: no bar; both are still on the Online
     page, and Home's card still counts them.

Result:

## 10. The connection drops

1. The app on Home. Turn Wi-Fi off (or pull the cable). B challenges A from another device (a phone).
2. After about 30 s, turn Wi-Fi on.
   - **Expected**: within about 15 s the challenge shows up, once, with the sound.
3. Repeat, but B cancels the challenge while A is offline.
   - **Expected**: after the reconnect nothing shows (or it shows for at most 3 s, then goes).

Result:

## 11. A game started elsewhere

1. The app on the Online page. In another browser signed in as **A**, accept a challenge from B there
   (or start a correspondence game).
   - **Expected**: within a second or two the game appears in "Your games" without Refresh.

Result:

## 12. A rematch after leaving

1. Finish a game A (app) vs B (browser). In the app, go back to Home. In the browser, B offers a
   rematch.
   - **Expected**: the bar says "B (rating) offers a rematch · …". Accept starts it.

Result:

## 13. Another profile

1. B challenges A. Switch to a profile without Lichess.
   - **Expected**: the bar goes (it belongs to A's account).
2. Switch back to A's profile.
   - **Expected**: the challenge is back (Lichess resends it), with the sound.

Result:
