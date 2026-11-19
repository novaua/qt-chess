# Active Handover

- **Last completed:** BL-21, `bugs/captureMoveMark_191126`: a capture target shows its corners in the dot's colour (under the pieces, as lichess.org) instead of a dot that a black piece hid; no glow (the owner, after a look).
- **Current focus:** None. BL-19 is `in_progress`: the owner decides whether it is done (left for them: scenario 6's tooltip on hover, 9.2 Windows' contrast theme).
- **Next up:** BL-25 (Analyse enabled without an engine until Settings is opened), then LI-10 or BL-26 (both Easy).
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items, and the move list's piece figures (a `layer.effect`) don't draw.
  - New scenes without baselines: `capture`, `capture-ep`, `premove-capture` (accept after a look: `Compare-Screenshots.ps1 -Update -Name …`).
