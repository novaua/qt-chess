---
id: LI-11
title: "Flair beside a Lichess name"
type: task
status: done
priority: medium
---

# LI-11 Flair beside a Lichess name

Follow-up of LI-07, which shows a Lichess game's players with their title, name and rating, but not their **flair**: the small picture a player picks on lichess.org, shown there beside the name. Not done in LI-07 because Chess++ can't show it today (the owner's choice, 2026-11-21: no new dependency for it then).

**Plan:** [`docs/LI-11-FlairPlan.md`](../../docs/LI-11-FlairPlan.md) (approved 2026-11-22).

**Decisions (the owner, 2026-11-22, after playing real Lichess players in LI-09's hand tests):**
- **Do it now, with the dependency:** add Qt's **Image Formats** add-on (`qwebp.dll`) to the build, CI and the installer, so the flair (WebP) can be shown.
- **A standard avatar for Lichess players:** today a Lichess opponent's card shows `unicorn` from the app's own avatar library. Instead: the player's **flair** when they have one; else a **standard Lichess-player avatar**, a **pawn in front of a small globe** ("a player online"). It is never offered for local profiles (not in the avatar picker) and uses no Lichess logo (their trademark).
- The player's own card in a Lichess game keeps the local avatar (LI-07's decision).

What was found (2026-11-21):
- **Format:** Lichess has the flair images only as **WebP** (lila `public/flair/img/<flair>.webp`, about 3,600 of them; the names in `public/flair/list.txt`, e.g. `activity.1st-place-medal`, `objects.light-bulb`). No PNG or SVG was found. Qt reads WebP only with the **Qt Image Formats** add-on (`plugins/imageformats/qwebp.dll`); the Qt 6.12 install here has only gif, ico, jpeg and svg.
- **URL:** lila builds it as `/assets/<flairVersion>/flair/img/<flair>.webp` (`AssetHelper.scala`: `flairVersion = "______5"`, raised when an image changes), so a URL built in the app can break when the version changes. Check the live URL and its host (lichess1.org is Lichess's asset host) before relying on it.
- **Data:** the game stream's `gameFull` players have no `flair` (API spec `GameEventPlayer`); the user object has it (`/api/account`, `/api/user/{name}`). One `POST /api/users` with both ids gets both players' flairs per game.

To do it:
1. Add Qt Image Formats: the local Qt, CI (`tests.yml`, `release.yml`) and the installer (`qwebp.dll`; mind BL-12's deployment size).
2. `LichessPlayer`: `id`, `flair`; `LichessClient` asks `POST /api/users` after a new game's `gameFull` (not on a resync), fills the players' flairs, and signals it; a reply for a game no longer on the board is dropped.
3. `ChessConnector`: `PlayerFlair`, `OpponentFlair` (image URLs, "" when none); `PlayerCard`: a 16-px image after the name, shown only once loaded (offline: nothing).
4. Tests: `FakeLichess` serves `/api/users` and a flair image (never lichess.org); `LichessClientTest` parses the flairs; `tst_lichess` checks the card's flair.

## Resolution

### 2026-11-22 (branch: features/lichessFlair_221126)
Done as planned in [`docs/LI-11-FlairPlan.md`](../../docs/LI-11-FlairPlan.md).
- **The flair:** after a Lichess game's start (never on a resync), `LichessClient::fetchPlayerFlairs` sends one `POST /api/users` with both players' ids (the AI has none). The flairs become image URLs, `https://lichess1.org/assets/flair/img/<flair>.webp`, checked live; the unversioned path works and a name that isn't a flair's gives none. `gamePlayersChanged` reports them. An answer for a game that was left is dropped, and a failure is only logged.
- **The cards:** the opponent's flair is their avatar (`AvatarProvider::setOpponentOnline`). Without one, or while it loads, offline or when it fails, the card shows the new standard avatar `online-player.png`: a white pawn before a blue globe, in the avatars' style with a transparent background (`python scripts/make_icons.py avatar`). It is never in the picker. The player's own card keeps the local avatar and shows their flair small after the Lichess name (`ChessConnector.PlayerFlair`).
- **Qt Image Formats:** added to CI (`modules: qtmultimedia qtimageformats` in `tests.yml` and `release.yml`). `windeployqt` copies `imageformats\qwebp.dll` (checked into a scratch folder), and `installer\Test-Installer.ps1` requires it. README and CLAUDE.md name the add-on; BL-12 has a note to keep the plugin.
- **Tests:**
  - ChessQtTests 347/347 (Release), with `GameStart_AsksForThePlayersFlairs_Once` and `PlayersFlairs_AiNotAsked_LeftGameDropped` new.
  - The UI suite: `tst_lichess` 17/17. test_03 checks that the opponent's flair is the avatar (a WebP the fake makes with Qt's plugin) and that the player's flair shows after the name. test_09 checks that a player with no flair gets the standard avatar. All other files pass, `tst_lichess960` with #223's fix for #222.
  - Debug and Release build.
  - The `clock-online` scene, narrow and 1400×800, shows the standard avatar on the opponent's card.
