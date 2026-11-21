---
id: LI-11
title: "Flair beside a Lichess name"
type: task
status: open
priority: low
---

# LI-11 Flair beside a Lichess name

Follow-up of LI-07, which shows a Lichess game's players with their title, name and rating, but not their **flair**: the small picture a player picks on lichess.org, shown there beside the name. Not done in LI-07 because Chess++ can't show it today (the owner's choice, 2026-11-21: no new dependency for it then).

What was found (2026-11-21):
- **Format:** Lichess has the flair images only as **WebP** (lila `public/flair/img/<flair>.webp`, about 3,600 of them; the names in `public/flair/list.txt`, e.g. `activity.1st-place-medal`, `objects.light-bulb`). No PNG or SVG was found. Qt reads WebP only with the **Qt Image Formats** add-on (`plugins/imageformats/qwebp.dll`); the Qt 6.12 install here has only gif, ico, jpeg and svg.
- **URL:** lila builds it as `/assets/<flairVersion>/flair/img/<flair>.webp` (`AssetHelper.scala`: `flairVersion = "______5"`, raised when an image changes), so a URL built in the app can break when the version changes. Check the live URL and its host (lichess1.org is Lichess's asset host) before relying on it.
- **Data:** the game stream's `gameFull` players have no `flair` (API spec `GameEventPlayer`); the user object has it (`/api/account`, `/api/user/{name}`). One `POST /api/users` with both ids gets both players' flairs per game.

To do it:
1. Add Qt Image Formats: the local Qt, CI (`tests.yml`, `release.yml`) and the installer (`qwebp.dll`; mind BL-12's deployment size).
2. `LichessPlayer`: `id`, `flair`; `LichessClient` asks `POST /api/users` after a new game's `gameFull` (not on a resync), fills the players' flairs, and signals it; a reply for a game no longer on the board is dropped.
3. `ChessConnector`: `PlayerFlair`, `OpponentFlair` (image URLs, "" when none); `PlayerCard`: a 16-px image after the name, shown only once loaded (offline: nothing).
4. Tests: `FakeLichess` serves `/api/users` and a flair image (never lichess.org); `LichessClientTest` parses the flairs; `tst_lichess` checks the card's flair.
