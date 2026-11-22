---
id: BL-34
title: "The checkmate sound plays before the mating move is seen"
type: bug
status: done
priority: medium
---

# BL-34 The checkmate sound plays before the mating move is seen

The owner: on checkmate the checkmate sound plays before the opponent's move, so you hear the mate first and see
it after. It should be heard after the opponent's move.

**Cause:** `ChessConnector` emits `checkMateNotify` (and `checkMateResult`, the win sound) from the game's
`EtCheckMate`, in the same call that plays the mating move. `ChessGame.qml` plays `sndCheckmate` / `sndWin`
there at once, while the piece only starts its slide (`MoveGate`: slide 250 ms + a 150 ms beat). The result card
already waits for the board (`GamePage`: `_resultPending` until `boardIdle`).

**How to see it:** play the robot until it mates you (or two players, the mating move by click): the mate sound
comes with the move's own sound, before the piece lands.

## Resolution

### 2026-11-22 — fix (branch: bugs/checkmateSoundAfterMove_221126)
- **Cause:** as above: the mate and win sounds played in the call that plays the mating move.
- **Fix:** `ChessGame.qml` queues them (`_endSoundWhenIdle`) and plays them on `ChessConnector.boardIdle`, so
  after the slide and the gate's beat (about 0.4 s; 0.15 s with Reduce animations), before the result card
  (0.6 s after idle). A result without a move, or one that comes while reviewing, plays at once (asked on the
  next pass, as `GamePage`'s result). Dropped by a takeback, a new game or leaving the game. The check sound is
  unchanged (it still comes with the move).
- **Tests:** `scripts\Run-UiTests.ps1 -Build`: 13 files, 0 failed (`tst_robot` and `tst_twoPlayers` play to
  mate; no QML warnings). The timing itself is checked by ear (screenshot mode has no animations).
