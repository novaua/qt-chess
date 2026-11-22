# .tasks

The tickets: `open/` is the queue, `archive/` the done ones. The rules are in `CLAUDE.md` ("Tasks and Bugs"), the format in `.claude/rules/tasks.md`, the steps in the `ticket` skill (`.claude/skills/ticket/`).

## From the former `TASKS.md`

Its sections, their intro text, and the tickets each held (2026-11-19):

- **In Progress**: BL-02, BL-19, BL-20.
  - "Manual checks, no code (do any time):" TSK-01, TSK-02, TSK-03, TSK-04.
  - **Done**: TSK-05, BL-15, BUG-01, TSK-06, TSK-07, BUG-02, TSK-08, TSK-09, TSK-10, TSK-11, TSK-12.
  - **Backlog**: BL-01, BL-02 (listed twice; merged), BL-03, BL-04, BL-05, BL-06, BL-07, BL-09, BL-08, BL-10, BL-11, BL-16, BL-17, BL-18, BL-12, BL-13, BL-14, BL-21, BL-22, BL-23, BL-24.
- **Chess Core Extensions**: "Work in `ChessCore` (the `Chess::` rules, no Qt) that opens the way to more kinds of chess." CE-01.
- **Lichess Improvement Backlog**: "Improvements to playing on Lichess. Related: BL-07 (polish the Lichess workflows, chat), BL-10 (challenges as notifications)." LI-01A, LI-01B, LI-02, LI-03, LI-04, LI-06, LI-05, LI-07, LI-08, LI-09, LI-10.
- **Network Multiplayer Game**: TSK-13.

Each ticket names its line in the old file (`<!-- migrated from TASKS.md:N (section) -->`).

## From the former `PROGRESS.md`

Its log (newest first, 2026-10-27 to 2026-11-19) is in the tickets' `## Resolution`, oldest first; the 7 entries that match no ticket are in `archive/_unmatched-progress.md`.

Both old files as they were: `git show c0acb37:TASKS.md`, `git show c0acb37:PROGRESS.md`.
