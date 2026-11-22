---
id: BL-40
title: "A check's sound and \"Check\" come after a takeback ended the check"
type: bug
status: done
priority: medium
---

# BL-40 A check's sound and "Check" come after a takeback ended the check

Found by the code review of BL-35's PRs (2026-11-22). Since BL-35 step 1b a move's check waits for the board
(`GameSounds.playWhenSeen("check", …)`, at `ChessConnector.boardIdle`), and `_playWaiting()` told whatever waited
without asking whether the board still shows a check. Seen: against the robot, 2. Qh5+ taken back while it still
slides: the check sound plays and "Check" shows, though no king is in check. On Lichess a premove that answers the
check before the board settles could do the same, and two checks before one idle played twice. (Typed-in moves
don't: `applyMoves` announces only the final position's check.)

## Resolution

### 2026-11-22 — fixed (branch: bugs/staleCheckSound_221126)
- Cause: two parts. `_playWaiting()` didn't look at the position; and a takeback against the robot clears the
  gate (`dropRobotAnswer()` → `MoveGate::clear()`), which emits `boardIdle` inside `movePrev()`, before the move
  is undone: the queue was told while the board still had the check.
- Fix (`GameSounds.qml`): `boardIdle` is answered on the next pass, and only if the board is still idle (the
  takeback has marked it busy again by then: `moveShown()`). A waiting check is told once, and only if
  `ChessConnector.CheckedKing` still shows one and no mate waits with it (in review it plays at once, as before).
- With it, from the same review: a sound still loading plays once loaded again (step 1 had dropped it), except a
  move's own; the header comments of `MessageCenter.qml`, `LichessChallenges.qml` (their ids) and `AppDialog.qml`
  (who owns the dialogs, a page's `dialogOpen`) fixed.
- Checked: new `tst_robot` test 8 (Qh5+ taken back in its slide: no check sound, no "Check"); it fails on the old
  `GameSounds.qml` (`move,moveOpponent,move,check,moveOpponent`). `Run-UiTests.ps1 -Build`: 13 files green;
  `tst_robot` twice more, green.
- Not changed: a takeback plays a move sound (`onMoveCountChanged`, older than BL-35).
