---
id: TSK-05
title: "Rematch swaps the colours in a robot or two-player game too"
type: task
status: done
priority: medium
---

# TSK-05 Rematch swaps the colours in a robot or two-player game too

<!-- migrated from TASKS.md:58 (In Progress / Done) -->

- [x] Rematch swaps the colours in a robot or two-player game too (#193, branch `bugs/rematchSwapsColours_191126`): `ChessConnector::rematch()` gives the player the other side, as a Lichess rematch always did; Settings → Gameplay "Rematch swaps colours" (`AppSettings.rematchSwapsColours`, `ChessConnector.RematchSwapsColours`), on by default; the swapped side is for that game only, and the setup dialogs offer the side last chosen in them. The review of #194 (7 findings) fixed.

## Resolution

### 2026-11-19 — Rematch swaps the colours (#193, branch: bugs/rematchSwapsColours_191126)

#### Done
- A rematch of a robot or two-player game kept the colours; it now gives the player the other side (the board turns; against the robot, the robot opens when it has White), as a Lichess rematch always did (`LichessClient::createRematch`).
- Settings → Gameplay **Rematch swaps colours**, on by default (`AppSettings.rematchSwapsColours` → `ChessConnector.RematchSwapsColours`); off keeps the colours. Lichess rematches always swap.
- The swapped side is for that game only (`ChessConnector::applyPlayerSide`, not written to `config.json`). The setup dialogs offer the side last chosen in them (`AppSettings.computerSide`, `twoPlayerSide`; until one is chosen, the last game's side as before), so a rematch never changes what New game offers. The UI tests' `startComputerGame` / `startTwoPlayerGame` click White (`computerSide_0`, `twoPlayerSide_0`).
- UI tests:
  - `tst_twoPlayers` test 4: Fool's mate, then the result card's Rematch (Black, the board turned); then two more rematches, the second with the setting off; a move before each, so "no moves" shows a new game. Last, the Two players dialog still offers White. `cleanup()` turns the setting back on.
  - `tst_robot` test 7: a game lost to the robot, then the result card's Rematch; the robot opens as White.
- The code review of #194 (7 findings) fixed: the side not saved; the user's path through the Rematch button; the setting-off check that couldn't fail; the setting restored in `cleanup()`; the helpers click instead of setting the property; the screenshots checked; `config.json` written once per robot rematch.

#### Checked
- UI tests (Debug) 12 files, 0 failed (tst_robot rerun after its fix).
- Screenshots `settings`, `computer`, `players` (light and dark). Against the old local baselines (2026-11-12), `settings` shows only the new row and everything below it shifted down. `computer` shows only the taller dialog (the Variant row of LI-01A), with White still chosen. The baselines are not updated.
