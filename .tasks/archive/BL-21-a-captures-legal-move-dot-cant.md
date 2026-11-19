---
id: BL-21
title: "a capture's legal-move dot can't be seen"
type: bug
status: done
priority: medium
---

# BL-21 a capture's legal-move dot can't be seen

<!-- migrated from TASKS.md:98 (In Progress / Backlog) -->

- [ ] BL-21 **Bug**: a capture's legal-move dot can't be seen [Easy], **not started** (the owner, 2026-11-19). When a selected piece can take, the target square shows no visible dot: the player can't see which pieces can be taken. Noticed after the redesign (BL-02).
  - **Likely cause (not confirmed):** `NavigationLayer.qml` draws the same dot on every target square, over the pieces: `Theme.legalMoveDot`, black at 22 % opacity, 30 % of the square. On an empty square it shows; on a capture it lies on the piece, and on a black piece it disappears. Before the redesign the dots were probably more opaque or coloured.
  - **Fix, as on lichess.org:** a capture target gets its own mark instead of the dot. Lichess shades the square's four **corners** (small triangles left by a ring-shaped gradient), so the piece stays fully visible and the mark shows on light and dark squares, under white and black pieces.
  - **The owner's idea:** additionally or instead, a **red glow** around the square (or the piece) that can be taken. To decide when it is done:
    - corners only, as on lichess.org (recommended: familiar, and it doesn't clash with the red premove squares or the check's red);
    - the red glow only;
    - both.
  - **Details:**
    - `ChessConnector.PossibleMoves` would say which targets are captures (en passant too: the square is empty, but it is a capture), or QML checks for a piece on the square.
    - Premove targets (red dots, LI-05) get the same corners in the premove colour.
    - Settings → Gameplay "Show legal moves" off hides them as it hides the dots.
    - The pop-in animation as for the dots; none with Reduce animations.
    - High contrast: a solid colour.
  - **Tests:** a screenshot scene with a selected piece that can take a white and a black piece, on light and dark squares and in the dark theme; a `tst_board` check that a capture square has its mark visible.

## Resolution

### 2026-11-19 — BL-21: the capture mark (branch: bugs/captureMoveMark_191126)

The owner's choice: **both**, lichess.org's corners and a red glow; after a look at the first version ("the attacked piece is too aggressively red"): **no glow**, the corners in the dot's colour, "as if the dot was cut in four and put into the corners"; the square and the piece as they are.
- **Cause:** as the ticket guessed: `NavigationLayer.qml` drew the same dark dot (black, 22 %) on every target, over the pieces; on a black piece it vanished.
- **Fix:**
  - `ChessConnector.CaptureSquares` (C++, per square): the move there takes a piece: the other side's piece is there, or en passant (a pawn going aside onto an empty square; not for a premove). A Chess960 king onto its own rook castles: a dot, as before.
  - `NavigationLayer.qml` has a `kind`: a second instance **under the pieces** (`captureLayer`) draws a capture target's mark: its four corners in the dot's colour (`Theme.legalMoveDot`; a ring-shaped gradient, as lichess.org's), so the piece and the square stay as they are. The dots stay in the instance over the pieces.
  - A premove target with the other side's piece: the corners in the premove dot's colour. "Show legal moves" off hides them; the pop-in as the dots', none with Reduce animations.
- **Tests:** `tst_board` test 5: the h5 queen's captures on e5 (dark square), f7, h7 (light) show the mark and no dot, its other targets a dot; Black's en passant onto the empty e3 and the c3 pawn; a premove capture vs FakeUci. Scenes `capture`, `capture-ep`, `premove-capture` (light, dark), looked at.
