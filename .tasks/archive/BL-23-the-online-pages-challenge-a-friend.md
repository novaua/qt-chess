---
id: BL-23
title: "the Online page's \"Challenge a friend\" card spills out of its frame"
type: bug
status: done
priority: medium
---

# BL-23 the Online page's "Challenge a friend" card spills out of its frame

<!-- migrated from TASKS.md:148 (In Progress / Backlog) -->

- [x] BL-23 **Bug**: the Online page's "Challenge a friend" card spills out of its frame [Easy], **fixed (branch `bugs/challengeCardOverflow_191126`): the cause confirmed (710 px of day buttons in a 640 px card); narrow day buttons (`ChoiceRow.buttonWidth`) in a `Flow` with their label; scene `online-days` (also 640×900); `tst_lichess` test 11 at the default and the minimum window width** (the owner, LI-04 hand test 6, 2026-11-19). With **Days per move** chosen, the buttons **10** and **14** go out of the card, and the **Lichess username (optional)** field reaches past the card's right edge too.
  - **Likely cause (not confirmed):** the days row is a `RowLayout` of a `ChoiceRow` (seven buttons) and the "days per move" label, without a width limit (`OnlinePage.qml`). It is wider than the card, so it overflows. The card's column then takes the widest child's width, so the `Layout.fillWidth` items, the username `TextField` among them, get that width and spill out with it.
  - **Fix:** let the row wrap or shrink to the card's width (the label under the buttons, or the buttons in a `Flow`), and keep every row within the card. Check the rest of the page's cards the same way: the time-control tiles, the Variant row, Side.
  - **Tests:** screenshot scenes of the Online page with Days per move chosen, at the default size and narrow (e.g. `-Size 600x900`); a UI test check that the days row and the username field lie inside the card.

## Resolution

### 2026-11-19 — BL-23: the "Challenge a friend" card spilled out of its frame (branch: bugs/challengeCardOverflow_191126)

#### Done
- The cause: with Days per move chosen, the seven day buttons (FluentWinUI3 makes each about 96 px wide) and their label were a `RowLayout`, 710 px in a 640 px card. A row in a layout keeps its whole width, so the card's column grew past the card, and the username field (`Layout.fillWidth`) with it.
- The fix (`OnlinePage.qml`):
  - the day buttons are 48 px wide (`ChoiceRow.buttonWidth`, new; 0 keeps the style's width everywhere else);
  - the buttons and "days per move" are in a `Flow`, so the label wraps under them if even that is too wide.
  - It now fits the narrowest window (640 px).
- Scene `online-days` (Days per move, 14 days, a username), also at 640×900, the narrowest window, in `all`. `tst_lichess` test 11 checks the day buttons, their label and the username field inside the card at the default and at the minimum window width. Against the old row it fails ("16 + 710 of 640").
- objectNames: `challengeCard`, `challengeTimeType`, `challengeDays`, `challengeDaysLabel`.

#### Checked
- UI tests (Debug) 12 files, 0 failed.
- Screenshots `online-days` (default, 640 and 600 wide), `online-blitz` at 640, and `players` / `computer` with `-Offscreen`: the other ChoiceRows unchanged.
- Without `-Offscreen` the setup dialogs' scenes showed only the dimmed Home (on plain develop too): the dialog's fade-in doesn't run when the screen is locked, as `Take-Screenshot.ps1` warns.
