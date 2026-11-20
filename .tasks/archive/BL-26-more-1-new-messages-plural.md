---
id: BL-26
title: "The More button says \"1 new messages\""
type: bug
status: done
priority: low
---

# BL-26 The More button says "1 new messages"

Found in Claude's run of `docs/BL-19-ManualTests.md` (2026-11-19) [Easy].

- **What happens:** with one unseen message, the game page's More button's tooltip (and its accessible name, if it uses the tip) reads "More (1 new messages)".
- **Where:** `ChessGui/ChessGui/qml/GamePage.qml:297`: `"More (" + page.unseenMessages + " new messages)"`, no singular.
- **Fix:** "1 new message" for one; check the other places that count new messages (Home's account menu, `RecentMessagesMenuItem`, `NewMessagesDot`) for the same.

## Resolution

### 2026-11-20 — BL-26 (branch: features/copyLinkAndMessageCount_201126, with LI-10)

- **Fix:** "1 new message" for one, "N new messages" otherwise: the game page's More tip (`GamePage.qml`) and Home's account button's accessible name (`HomePage.qml`). "Recent messages (1 new)" was right already.
- **Tests:** `tst_board` test 6: the More tip with 1, 2 and 0 new messages.
