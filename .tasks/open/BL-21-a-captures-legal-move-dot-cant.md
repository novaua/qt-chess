---
id: BL-21
title: "a capture's legal-move dot can't be seen"
type: bug
status: open
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
