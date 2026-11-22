---
id: LI-15
title: "My recent Lichess games, from the server"
type: task
status: open
priority: medium
---

# LI-15 My recent Lichess games, from the server

The owner (2026-11-22): Lichess games aren't saved here, "however games are saved on the Lichess server, so this may be fine as soon as we can query the server — the last 10 at most, and a More button."

**Today:** a finished Lichess game is gone once its page is left. The Games library (`GamesPage`) keeps only local games, and the Online page lists only *unfinished* Lichess games (`ongoingGames`, LI-04).

**Wanted:** the player's **last 10 Lichess games** (finished), newest first, with **More** for the next 10. Each is a card like `GameCard`: opponent, result, clock and speed, date, a `BoardPreview` of the final position. On a card:
- **Open**: a replay on the board, as a kept game's (`mode "replay"`, from its moves and start FEN);
- **Analyse**: LI-14;
- **Open on lichess.org** and **Copy link**, as `OngoingGameCard`'s menu (LI-10).

**The API (to check in the plan):** `GET /api/games/user/{username}?max=10&moves=true&pgnInJson=false&finished=true` with `Accept: application/x-ndjson`, one game per line (id, players with ratings, status, winner, speed, clock, variant, initialFen, moves, createdAt / lastMoveAt). **More** asks again with `until=<the last game's createdAt − 1>`. Lichess rate-limits this export, so ask only when the list is shown, cache the pages for the session, and refresh on demand.

**To decide in the plan:**
- **Where:** a **Lichess** filter on the Games page (beside All · vs Robot · Two players · Unfinished), or a section on the Online page under "Your games".
- Whether opening a game keeps it locally afterwards. Connected to LI-14's decision (b).
- Offline, or not linked to Lichess: the filter or section says so instead of an empty list.

**Tests:** `FakeLichess` serves `/api/games/user/<name>` (ndjson, two pages); `LichessClientTest` parses them, including `until`; the UI test checks the list, More, and a replay opening.

Related: LI-14, LI-16, LI-04 (unfinished games), LI-10 (a card's link menu).
