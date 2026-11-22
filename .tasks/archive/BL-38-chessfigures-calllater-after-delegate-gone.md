---
id: BL-38
title: "ChessFigures.qml:74: TypeError 'running' of null in the UI test logs"
type: bug
status: done
priority: low
---

# BL-38 ChessFigures.qml:74: TypeError 'running' of null in the UI test logs

The UI test logs carry `qrc:/qml/ChessFigures.qml:74: TypeError: Cannot read property 'running' of null
(exception occurred during delayed function evaluation)`, 32 times in `tst_chess960` (`test_8_variantRowComputer`)
and 32 times in `tst_games` (`test_9_rating`). Seen on `develop` at d26cca9 (2026-11-22), so it predates BL-35.

Where: each piece's `onPieceChanged: Qt.callLater(() => { if (!chessPiecesSlide.running) shownPiece = piece })`.
The delegate is destroyed before the delayed call runs (32: a whole set of pieces, e.g. the board of a new
game), so `chessPiecesSlide` is null. No visible harm, but it fills the logs and hides real warnings.

Fix to try: check that the delegate is still alive in the callback (`if (chessPiecesSlide && …)`), or set
`shownPiece` at once when no slide runs.
## Resolution

### 2026-11-22 — fixed (branch: bugs/figuresCallLater_221126)
- Cause: as found above. A piece's delayed `onPieceChanged` call ran after its delegate was destroyed (a new
  position replaces the pieces), where `chessPiecesSlide` is null. Seen 64 times each in `tst_games` and
  `tst_chess960` (`test_5_castleOnTheKingsSquare` too).
- Fix: the callback checks that `chessPiecesSlide` is still there. Behaviour unchanged.
- Kept out from now on: `Run-UiTests.ps1` fails a file whose log has a QML `TypeError` or `ReferenceError`, even
  with every test passed (GitHub runs it too).
- Checked: `Run-UiTests.ps1 -Build`, 13 files green, no QML errors left in any log, Release and Debug (as
  GitHub). The old `ChessFigures.qml` with the new check: `tst_games` fails, "64 QML errors in the log".
