# LI-01A Chess960 on Lichess (stage 2) — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-18 |
| **Build** | `develop` with #182–#183 (Release) |
| **Plan** | [`LI-01A-Chess960LichessPlan.md`](LI-01A-Chess960LichessPlan.md) (§5: "By hand") |
| **Rules** | <https://lichess.org/variant/chess960> |

The automated tests play Chess960 against a stand-in for Lichess (`FakeLichess`, `tst_lichess960`) at
one fixed position (#3). Only the real lichess.org shows the following: that it sends a Chess960 game's
start the way the app reads it, that its castling moves arrive and go out right, that a rematch keeps the
position, and that a game of another variant is refused cleanly. Write the result in the last line of each
scenario: **OK**, or what happened instead. Add the time, so the app log can be found: *Settings → About →
Open log folder*. The `gameFull` line of each game shows its `variant` and `initialFen`.

**You need two Lichess accounts**, as for BL-10:
- **A** is linked to the app's profile.
- **B** is signed in in a browser (a private window works). B challenges A from A's profile page on
  lichess.org (`lichess.org/@/A` → the crossed swords, "Challenge to a game"); under **Variant** choose
  **Chess960**.

Keep the games **casual**. The app's own challenges always are; choose Casual in B's challenges too, so
neither account's rating changes.

**How to castle in Chess960** (as on lichess.org): select the king, then click its rook (or drop the king on its
rook). After O-O the king is on g1 and the rook on f1. After O-O-O the king is on c1 and the rook on d1 (g8 / f8,
c8 / d8 for Black), wherever they started.

Already checked by the tests and the screenshot scenes, no need to look again:
- the Online page's Variant row and its line (`online-960`), the Chess960 challenge row (`online-challenges`)
  and the Profile's Chess960 tile (`profile`);
- against FakeLichess (`tst_lichess960`): accepting a 960 challenge at #3, a premoved castling sent as the king
  onto its rook, the opponent's castling, Continue of a 960 game, a Crazyhouse challenge without Accept, the
  challenge's `variant=chess960`;
- in ChessQtTests: the rematch's `fen`.

## 1. A Chess960 challenge from lichess.org, both castlings

1. The app on Home, signed in as A. In the browser, B challenges A: **Chess960, 10+5, Casual, B plays White**.
   - **Expected**: the bar says "B (rating) challenges you · 10+5 · Chess960 · Casual · You play Black", with
     **Accept**. (Before stage 2 it said "Can't be played in this app".)
2. **Accept**.
   - **Expected**: the game page; the title ends "· Chess960 #N". The pieces behind the pawns are the same as in
     the browser, square by square (the board is turned for Black). If lichess.org shows the position's number
     (the game's info box), it is the same N. Write down N.
3. Play until both can castle. B castles in the browser (O-O or O-O-O); A castles the other way in the app.
   - **Expected**: B's castling lands in the app with king and rook on the right squares, and A's in the
     browser. The app's move list says O-O / O-O-O, not a king move or a capture, and nothing shows up in the
     captured pieces. Both boards show the same position after each move.

Result:

## 2. A premoved castling

In the game of scenario 1 if A hasn't castled yet, else in a new one (B challenges again):

1. While B is thinking, as A: select the king, then click its rook.
   - **Expected**: the rook's square is marked as a premove.
2. B moves in the browser.
   - **Expected**: A's castling is played at once, and the browser shows it. If B's move made it illegal (a
     check, an attacked square), the premove just goes and nothing is sent.

Result:

## 3. Leave, Continue, restart

1. In a running 960 game, **Home** → **Leave** (the game goes on at Lichess).
   - **Expected**: Home offers Continue. Home → **Play online** → *Your games*: the card says "Rapid · Chess960 ·
     …", with Continue (not "Open on lichess.org").
2. **Continue** it.
   - **Expected**: the same position, "· Chess960 #N" in the title; castling rights as they were (a side that
     castled or moved its king can't castle again).
3. Leave again, close the app, start it, **Continue** from Home.
   - **Expected**: the same as 2. Play a move each way to see the game is live.

Result:

## 4. Rematch: the same position

1. Finish a 960 game (resigning is fine). On the result card: **Rematch**.
   - **Expected**: B gets a challenge in the browser: Chess960, casual. Once B accepts, the new game in the app
     and in the browser starts from **the same position as the last game** (the same #N), with colours swapped.
2. Finish that one too. This time **B** offers the rematch in the browser.
   - **Expected**: the app's rematch bar; **Accept** starts the game at whatever position Lichess picked. Both
     boards show the same set-up. (Write down whether Lichess kept the position.)

Result:

## 5. A Chess960 challenge from the app

1. Home → **Play online** → Challenge a friend: Real time **10+5**, Variant **Chess960**, Lichess username: B →
   **Challenge B**.
   - **Expected**: the line under the Variant row says "…picked by Lichess…". If A has a Chess960 rating,
     "Your Chess960 rating: …" shows instead of the Rapid one. B gets a Chess960 challenge in the browser; once
     accepted, both play from the same position, "· Chess960 #N" in the app's title.
2. Resign it. Again with the username field **empty** → **Create challenge**, and open the link in B's browser.
   - **Expected**: lichess.org shows a Chess960 challenge; once B joins, a Chess960 game in the app.
3. Go back to the Online page.
   - **Expected**: Chess960 is still chosen, also after a restart. Home → **Two players** (or **Play vs
     computer**): the local dialogs' Variant row shows what was last chosen there, independent of this one.

Result:

## 6. Correspondence

1. Online page → **Days per move** (any), Chess960, to B. B accepts. Play a move each, castle once if you can,
   then leave the game.
   - **Expected**: *Your games* lists it ("Correspondence · Chess960 · … left"), and **Continue** brings it back
     at its position. A move B makes meanwhile is on the board when you continue.

Result:

## 7. The PGN of a Lichess 960 game

1. In a Lichess 960 game (running or just finished): More → **Export PGN…**, open the file in Notepad.
   - **Expected**: `[Variant "Chess960"]`, `[SetUp "1"]`, `[FEN "<the start>"]`; the castlings as O-O / O-O-O.
2. Paste it at <https://lichess.org/paste> → **Import game**.
   - **Expected**: the same start, every move read, the final position the same as the game's.

Result:

## 8. The Chess960 rating

1. Only if A has played a **rated** Chess960 game on Lichess (on the website, any time): **Profile**.
   - **Expected**: a Chess960 tile with the same rating as `lichess.org/@/A` shows for Chess960. Without such a
     game there is no tile (Lichess has no rating to give).

Result:

## 9. Another variant: refused

1. B challenges A to **Crazyhouse** (casual).
   - **Expected**: the bar says "… · Crazyhouse · … · Can't be played in this app", with **Open on lichess.org**,
     no Accept. **Decline** it.
2. In the browser, B creates a link challenge (*Play with a friend* → Crazyhouse → "Invite" link). In the app:
   Home → **Play online** → *Join a game*: paste the link → **Join**.
   - **Expected**: no game page. A message: "Couldn't continue the game: it is Crazyhouse, and Chess++ plays
     standard chess and Chess960. Open it on lichess.org." The game was started on Lichess by the join: abort it
     in the browser (B can, before the first moves).
3. Optional, if A has a Crazyhouse game going on (A accepted one on the website): the Online page's *Your games*
   card says "… · Crazyhouse · …" with **Open on lichess.org**, not Continue.

Result:
