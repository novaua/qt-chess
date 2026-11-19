---
id: LI-01B
title: "Crazyhouse"
type: task
status: open
priority: medium
---

# LI-01B Crazyhouse

<!-- migrated from TASKS.md:248 (Lichess Improvement Backlog) -->

- [ ] LI-01B Crazyhouse [Hard], **needs a plan first; not started** (after LI-01A; on CE-01's game rules). Chess-logic work in `ChessCore`, not only the Lichess dialog. The plan should cover:
  - captured pieces go to the capturer's pocket and can be dropped instead of a move (UCI `P@e4`; no pawn drops on the 1st/8th rank);
  - a promoted piece goes back to the pocket as a pawn;
  - drops can give check and parry it (mate detection);
  - the UI: two pockets with counts by the player cards (in place of the captured strip), dragging or clicking a piece from the pocket to a square;
  - `BoardPieces` adds a row for a dropped piece;
  - the move list (`P@e4` / SAN `@e4`);
  - Stockfish has no Crazyhouse in the default build, so no robot mode unless a variant engine is added (BL-06).
  - Tests in `ChessTests` for each rule; a screenshot scene for a Crazyhouse position with pockets.
  - The `crazyhouse` rating pool in `parseRatings` and the profile.
