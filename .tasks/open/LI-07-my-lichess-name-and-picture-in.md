---
id: LI-07
title: "My Lichess name and picture in a Lichess game"
type: task
status: open
priority: medium
---

# LI-07 My Lichess name and picture in a Lichess game

<!-- migrated from TASKS.md:287 (Lichess Improvement Backlog) -->

- [ ] LI-07 My Lichess name and picture in a Lichess game [Medium], **not started; the owner's choice below is open** (the owner, 2026-11-19). In a Lichess game the player's card shows the local profile (e.g. "Ninja" with its local avatar), while the opponent sees the Lichess account's name. The owner would like to see the Lichess name, and the Lichess picture if there is one. Today (`chessconnector.cpp`):
  - the player's card shows `playerName()`, the local profile's name, and `AvatarProvider.playerUrl`, the local avatar;
  - the opponent's card shows only the Lichess name, with the `unicorn` avatar (`LichessClient` sends no avatar: "Lichess doesn't provide avatar in stream"). Their rating, title (GM, BOT…) and provisional mark come in `gameFull` and aren't shown.
  - Lichess has no uploaded profile pictures. The nearest is the **flair**, a small emoji the player picks on lichess.org (`flair` in the API's user; images on lichess1.org): to check in the plan whether `gameFull` or `/api/account` gives it. So the local avatar stays the picture, and the flair is shown beside the name.
  - **Recommended, without a setting:** in a Lichess game both cards show the Lichess identity, as lichess.org does: title, name, flair, rating (`?` when provisional). The player's card keeps the local avatar and adds the local name in its detail line ("Ninja · 1523?"), so both stay visible. The robot and two-player games stay as today.
  - Alternatives: (a) a setting, Settings → Online "Show my Lichess name in games" (on by default); (b) the owner's idea of one card in two halves, the Lichess side on the left and the local side on the right. (b) crowds the card in a narrow window and in the wide layout's column (BL-13), so it isn't recommended.
  - Also the game's title ("Lichess · vs X"), the result card and the recent messages: the Lichess names.
  - Tests: `FakeLichessServer` with a title, flair and rating, and `tst_lichess` checks of both cards; scenes `online` and narrow with them; `LichessClientTest` parsing `gameFull`'s players.
