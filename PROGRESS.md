# Active Handover

- **Last completed:** BL-19's hand tests run by Claude, `docs/bl19HandTestRun_191126`: all 9 scenarios with Stockfish 19 (results in `docs/BL-19-ManualTests.md`); found BL-25 and BL-26.
- **Current focus:** None. BL-19 is `in_progress`: the owner decides whether it is done (left for them: scenario 6's tooltip on hover, 9.2 Windows' contrast theme).
- **Next up:** BL-21 (a capture's legal-move dot), waiting on the owner's choice: corners, a red glow, or both.
- **Active gotchas:**
  - `.tasks/archive/_unmatched-progress.md`: 7 old log entries that match no ticket, for the owner to file or drop.
  - UI tests offscreen: hover doesn't reach items, and the move list's piece figures (a `layer.effect`) don't draw.
