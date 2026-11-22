---
id: LI-14
title: "Analyse a Lichess game"
type: task
status: open
priority: high
---

# LI-14 Analyse a Lichess game

The owner, after playing Rapid on Lichess (LI-09's hand tests, 2026-11-22): "we definitely have to be able to analyse the game."

**Today:** the engine's analysis (BL-19 steps 6–7: `GameAnalyzer`, `ChessConnector.analyseGame` / `CanAnalyse`, the move marks, `EvalBar`, `MoveArrows`, `AnalysisSummary`) works only on a game kept in the library (`GameRecord`). Lichess games have no record (BL-19, the owner's choice then), so:
- `ChessConnector::refreshBoardAnalysis` makes `CanAnalyse` false for an online game (`kept = … && !isOnlineGame()`);
- the result card shows **Rematch**, **New game**, **Review moves**, but no **Analyse**.

lichess.org's end of a game offers **Analysis board**.

**Wanted:**
- **Analyse** on the result card of a finished Lichess game, and in its More menu, as for a local game.
- The same for a Lichess game opened from the list of recent games (LI-15).
- The analysis works as it does for a kept game: progress, marks, accuracy, eval bar, best-move arrows.

**Decision (the owner, 2026-11-22): finished Lichess games are kept locally.** A finished Lichess game gets a record in the library, as option (b) below: marked as Lichess, with its analysis. It is never rated, continued or Played from here. It survives a restart, the Games page shows it, and Analyse works as for a local game. This reverses BL-19's "Lichess games have no record" for **finished** Lichess games only; an unfinished one stays on Lichess (LI-04's Continue). The plan still decides when the record is written (at the game's end, or when it's analysed), its signed file and fields (`kind: "lichess"`, the Lichess game id, the players' Lichess names and ratings), a Games page filter, and how LI-15's list relates to it (a game from the server already kept here opens its record).

**Plan**, for the owner's approval: [docs/LI-14-LichessAnalysisPlan.md](../../docs/LI-14-LichessAnalysisPlan.md) (three steps, one PR each; its questions answered by the owner on 2026-11-22).

**To decide in the plan:**
1. **Where the analysis lives.** Decided above: (b).
   - (a) In memory for the game on the board only: lost when it's left.
   - (b) Lichess games get a record in the library after all, finished ones only, with their analysis. This reverses BL-19's "Lichess games have no record". They wouldn't be rated, continued or Played from here.
   - (c) A small cache of analyses keyed by the Lichess game id, so reopening a game from LI-15's list shows its analysis again.
2. **Whose engine.** Ours (Stockfish, full strength, the Settings → Computer depth), as for local games. Lichess's own server analysis can only be *read* when someone already requested it on lichess.org (`GET /game/export/{id}?evals=true`: unverified whether the API can request one). Possibly also "Open the analysis on lichess.org" (`https://lichess.org/<id>` + `/analysis`) as a fallback.
3. **Chess960** games: replay from their start FEN (`StartPosition`), as kept Chess960 games are.

**Tests:** FakeUci analysis of a finished Lichess game (from `FakeLichess`), the result card's Analyse, and the marks on the move list (`tst_lichess`). A Chess960 Lichess game too (`tst_lichess960`).

Related: LI-15 (the list of recent Lichess games), LI-16 (the end of a Lichess game: Rematch, New opponent, Analyse).
