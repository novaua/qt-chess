# LI-16 New opponent — Manual Tests

| | |
| :---- | :---- |
| **Date** | 2026-11-26 |
| **Build** | `develop` with LI-16 step 1 (Release) |
| **Plan** | [`LI-16-NewOpponentPlan.md`](LI-16-NewOpponentPlan.md) |
| **Lichess** | <https://lichess.org/api#tag/Board/operation/apiBoardSeek> |

The automated tests play against a stand-in for Lichess (`FakeLichess`: `LichessSeekTest`, `tst_lichess` test_19
and test_20). Only the real lichess.org shows the following:

- that a real seek's game is known as one (its `gameStart`'s `source`), so its card shows **New opponent**;
- that the second seek is paired and its game replaces the finished one on the same page;
- that Cancel from the command bar's seek icon takes the second seek off Lichess.

Write the result in the last line of each scenario: **OK**, or what happened instead. Add the time, so the app
log can be found (*Settings → About → Open log folder*). Each seek is logged as `LichessClient: seek time=…`.

**Real players are fine**, as in LI-09's results: a Rapid tile is paired with someone in a minute or two. With a
second account B, keep the games **Casual** (rated games between two accounts of one person are against Lichess's
fair play rules) and let B take the seek from lichess.org's **Lobby** tab.

Already checked by the tests and the screenshot scenes, no need to look again:
- **The card's look:** New opponent in New game's place, two rows of two, one column in a narrow window. Scene
  `lichess-new-opponent` (also `@640x560`).
- **The request:** New opponent sends the first seek's body again (`time`, `increment`, `rated`, `variant`).
- **A challenge's game:** New game, as before.
- **A rematch challenge of ours still waiting:** withdrawn first, and the seek isn't refused.

## 1. New opponent after a Quick pairing game

1. Home → **Play online** → a Rapid tile (e.g. **10+0**), Casual. Wait for the game.
2. Play it to its end (or resign).
3. The result card shows **Rematch**, **New opponent**, **Review moves**, **Analyse**; no **New game**.
4. Click **New opponent**. It says "Looking for an opponent…" and can't be clicked again; the game page stays,
   with the seek icon in the command bar. Its menu says what is sought: the same clock and Casual.
5. While waiting: **Review moves** and **Analyse** still work on the finished game.
6. When paired: the new game opens on the same page, with the new opponent's card.

Result:

## 2. Cancel the second seek

1. As 1, steps 1–4.
2. The command bar's seek icon → **Cancel seeking**. The button is **New opponent** again.
3. With B: the seek is gone from lichess.org's Lobby tab.

Result:

## 3. Home while it waits

1. As 1, steps 1–4.
2. **Home**: the seek goes on beyond the game page, as beyond the Online page; its icon is in Home's command bar.
3. When paired: the game opens from Home.

Result:

## 4. A challenge's game keeps New game

1. With B: B challenges A (or A accepts a challenge from the bar); play it to its end.
2. The result card shows **New game**, not New opponent. New game leaves for the Online page.

Result:

## 5. Ctrl+N

1. As 1, steps 1–2.
2. **Ctrl+N** does what New opponent does (the seek starts, the page stays).

Result:
