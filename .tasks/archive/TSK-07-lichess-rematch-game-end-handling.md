---
id: TSK-07
title: "Lichess rematch + game end handling"
type: task
status: done
priority: medium
---

# TSK-07 Lichess rematch + game end handling

<!-- migrated from TASKS.md:63 (In Progress / Done) -->

- [x] Lichess rematch + game end handling (#70) — plan: [`docs/LichessRematchPlan.md`](docs/LichessRematchPlan.md)
  - `LichessClient::createRematch()`: direct challenge (`keepAliveStream`) to a registered opponent, open challenge for anonymous; `parseRematchInfo()` + 4 unit tests
  - `ChessConnector::CanRematch`, Rematch button for online games, `ChallengeDialog` direct-challenge mode
  - Game end visible everywhere: `GameResultCard.qml` (portrait), board notification, red flag on the losing king (`FlaggedKing`), board frozen
  - `OpponentGoneBanner.qml` + Claim victory; `gameFull` of a finished game ends it
  - Local fixes: resign vs Robot, undo after mate, stats once per game, no autosave of finished games, `#` only for mate
  - Live check 2026-10-28: resign from the app and from an anonymous Lichess opponent ✅

## Resolution

### 2026-10-28 — Lichess Rematch (branch: feLichessRematch_281026)

#### What was done
- Merged earlier today: computer levels rework + levels help (#68), promotion dialog rotation fix (#69).
- Analysed why Rematch doesn't work vs Lichess: the button is hidden for online games and the Board API has no rematch endpoint. Plan written to `docs/LichessRematchPlan.md`: Rematch creates a new challenge (direct to a registered opponent, open link for anonymous).
- Implemented steps 1–4 (see `TASKS.md`): `LichessClient::createRematch()` / `parseRematchInfo()`, `ChessConnector::CanRematch`, Rematch button for online games, `ChallengeDialog` direct mode. Direct challenges use `keepAliveStream=true` (otherwise they expire after 20 s).
- Debug + Release build OK; `LichessStaticTests` 21/21 (4 new); app starts without QML errors.
- Testing showed an opponent's resignation was received (`status:"resign"`) but nothing was shown: result and Rematch lived only in the landscape side panels. Added step 4b (result card, notification, red flag on the losing king, frozen board, `opponentGone` + Claim victory).
- Note for testers: resigning on lichess.org needs the flag **and** the ✓ confirm within ~3 s; otherwise nothing reaches the server.

- Code review fixes (step 4d) and PR #70 opened. Live check OK for resign from the app and from an anonymous Lichess opponent.

- Merged: #70 (rematch + game end) and #71 (test host crash fix). `ChessQtTests` engine tests now run reliably (10/10).

#### Where to continue
1. Play a game against a second Lichess account and run the manual scenarios in plan §5 (accept, decline, cancel, anonymous link, vs Robot unchanged, restart, leave while pending).
2. Watch the log for `LichessClient: challenge stream line:`. It confirms the keep-alive challenge JSON and the final `{"done":...}` shape this code expects.
3. If a live scenario fails, fix it on a new branch from `develop`.

#### Notes
- If the opponent clicks Rematch in their browser, the Board API client probably never sees it; the app must send the challenge.
