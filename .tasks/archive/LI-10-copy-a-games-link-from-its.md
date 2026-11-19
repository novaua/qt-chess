---
id: LI-10
title: "Copy a game's link from its card"
type: task
status: done
priority: medium
---

# LI-10 Copy a game's link from its card

<!-- migrated from TASKS.md:336 (Lichess Improvement Backlog) -->

- [ ] LI-10 Copy a game's link from its card [Easy], **not started** (the owner, LI-04 hand test 7, 2026-11-19). A Lichess game Chess++ can't play (lobby Blitz, Bullet, Crazyhouse…) has **Open on lichess.org** on its card (`OngoingGameCard`), which only opens the browser.
  - A right click on the card or its button opens a menu: **Open on lichess.org**, **Copy link** (`https://lichess.org/<gameId>`, "Link copied" as a short message).
  - Maybe the same menu on every Lichess game card (Continue, Copy link), and a long press for touch.
  - Tests: a `tst_lichess` check that Copy link puts the URL on the clipboard; a screenshot scene with the menu open.

## Resolution

### 2026-11-20 — LI-10 (branch: features/copyLinkAndMessageCount_201126, with BL-26)

- **Done:** a right click on a Lichess game's card (`OngoingGameCard`: Your games on the Online page, the Continue list; also on its board and button) opens its menu: **Continue** (only a game playable here), **Open on lichess.org**, **Copy link** (`https://lichess.org/<gameId>` on the clipboard, "Link copied"). The clipboard: `AppInfo.copyToClipboard(text, message)` (QML has none; not `ChessConnector`: a link isn't game logic), its `copied(message)` the window's short message.
- **Not done:** a long press for touch (the ticket's "maybe"): a touch handler on the card could take the taps the board and its button need; for later, with Android.
- **Tests:** `tst_lichess` test 12: a Crazyhouse game's card, right click: no Continue, Open on lichess.org; Copy link puts its URL on the clipboard (`TestRun.clipboardText()`, new) and says "Link copied". Scene `online-game-menu` (Your games scrolled into view, the Blitz game's menu), looked at.

### 2026-11-20 — LI-10: the code review of #205 (same branch)

- The menu by the keyboard: the Menu key or Shift+F10 on a card's button (`GameCard.hasMenu`, `menuRequested`), so `openMenu()` isn't for the scene only.
- Copy link reads the clipboard back: "Couldn't copy to the clipboard…" when another program held it (`AppInfo.copied(message, ok)`).
- `url` / `objectName` guard a missing `game`; the scene checks for its card and opens the menu once the scroll is laid out; `AppInfo`'s header names the clipboard.
- Left: the menu closes if the list refreshes while it is open (rare; a window-level menu would need both pages' plumbing); the message through C++ and the plural in two places (style).
- Tests: `tst_lichess` test 12 + the Menu key and Shift+F10. UI tests (Release): 13 files, 0 failed. Scene `online-game-menu` taken offscreen (the screen was off: every popup scene timed out in a real window, the existing `online-challenges-decline` too).
