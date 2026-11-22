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

## More seen (2026-11-22, 18:05)
A full `Run-UiTests.ps1 -Build` (BL-35 step 5's branch, which touches no robot code) failed 4 files at once, all
robot games: `tst_analysis` ("Best was g6" where FakeUci's best was the move played), `tst_chess960` test 6 again
("the robot castled": "r" where "k"), `tst_games` (the PGN "1. e4 d6" where the script says e5), `tst_robot` (test 1
"its pawn on e5": empty; test 4; test 7 "its Qh4#"). The tests that followed in those files failed only from there.
A Debug `ChessGui.exe` with the real Stockfish was running beside it (started at 18:00, not by the tests), so the PC was busy. `tst_robot`
alone, then the whole suite again on the same exe: all green.

The robot playing moves off its script ("1. e4 d6", the wrong castling) is FakeUci's fallback, the first legal
move, when it has no script or its script doesn't fit (`FakeEngine::ChooseMove`). So under load the robot's engine
may play without the script the test set: `TestSetup::setEngine` puts it in `FAKEUCI_MOVES`, read when FakeUci
starts, so an engine started before `setEngine` (or one kept from the test before) plays the old or no script.
FakeUci says so on stderr ("the script's move … isn't legal there"): look for it in the failing file's log.
