---
id: BL-37
title: "tst_chess960 test_6 (robot and premove) failed once: e5 empty after the robot's answer"
type: bug
status: open
priority: low
---

# BL-37 tst_chess960 test_6 (robot and premove) failed once: e5 empty after the robot's answer

Seen once, locally (2026-11-22, Release, `develop` at d26cca9, during BL-35 step 3's checks): `Run-UiTests.ps1
-Build -Name tst_chess960`, `test_6_robotAndPremove`: "its e5", Actual "" Expected "p"
(`ChessGuiTests/qml/tst_chess960.qml:175`). It failed after `tryCompare(MoveCount, 3)` and `waitBoardIdle()`:
the robot's 1… e5 and the premoved O-O were counted, but e5 showed no piece. Tests 7 and 8 then failed only
because the page was still the game ("page home (on game)"). The next run of the same build passed, and so did
every full run that day (5).

Cause unknown. To look at: whether `waitBoardIdle()` can come true before `BoardPieces` has synced the robot's
answer (`schedulePiecesSync`) when the premove is played right after it; else `pieceAt` reading a piece in its
fade. If it fails again, keep `test-results\ui\Chess960-test_6_robotAndPremove.png` (a later run clears it).
Not BL-28 (there the robot never answers).
