---
id: LI-10
title: "Copy a game's link from its card"
type: task
status: open
priority: medium
---

# LI-10 Copy a game's link from its card

<!-- migrated from TASKS.md:336 (Lichess Improvement Backlog) -->

- [ ] LI-10 Copy a game's link from its card [Easy], **not started** (the owner, LI-04 hand test 7, 2026-11-19). A Lichess game Chess++ can't play (lobby Blitz, Bullet, Crazyhouse…) has **Open on lichess.org** on its card (`OngoingGameCard`), which only opens the browser.
  - A right click on the card or its button opens a menu: **Open on lichess.org**, **Copy link** (`https://lichess.org/<gameId>`, "Link copied" as a short message).
  - Maybe the same menu on every Lichess game card (Continue, Copy link), and a long press for touch.
  - Tests: a `tst_lichess` check that Copy link puts the URL on the clipboard; a screenshot scene with the menu open.
