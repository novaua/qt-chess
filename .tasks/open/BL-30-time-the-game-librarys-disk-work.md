---
id: BL-30
title: "Time the game library's disk work; move it off the GUI thread only if slow"
type: task
status: open
priority: low
---

# BL-30 Time the game library's disk work; move it off the GUI thread only if slow

Found by the async review of 2026-11-21. Unmeasured: likely invisible on an SSD, plausible hitches on an HDD, a redirected AppData folder or with antivirus scanning.

**Found:**
- Every move writes the game's file on the GUI thread: `ChessConnector::updateRecord` → `GameLibrary::put` → `scheduleSave` (`singleShot(0)`) → `save()` → `QSaveFile` (flushed, renamed), right as the piece starts to slide (a GUI-thread animation). A few KB per game; `rating.json` and `users.json` at the game's end.
- `GameLibrary::reload` → `readProfile` reads and HMACs every game file at start and at every profile switch.
- Changing "Games kept" reads every other profile's files (`finishedBeyond`, `trimOtherProfiles`).

**Step 1 (measure):** `[Timing]` lines (`chess.timing`) for a `save()` over ~8 ms, and for `reload()` / `finishedBeyond` with their file counts. Check with 500 games on a slow disk (a USB stick through `--data-dir`).

**Step 2, only if step 1 shows stalls:**
- Cheapest: save once the board has settled (the move gate idle) instead of `singleShot(0)`.
- Else one writer QObject on its own QThread: the GUI thread builds the bytes; the queue is latest-wins per path; a remove after pending writes of the same id; failures back by signal into `_changed` (its retry stays); `flush()` (profile change, destructor) drains the queue synchronously.
- `readProfile` on a worker, with a loading state that Home's Continue, `continueOnlyGame` and the rating respect.

**Test:** `GameLibraryTest`: the write order, a failed write retried, `flush()` on a profile change and in the destructor writing everything, a reload while writes are queued. UI tests `tst_games`, `tst_restart`, `tst_profiles` pass.

Not to change: the Lichess network code is already asynchronous and correct (replies with context objects, `deleteLater`, members nulled before `abort()`); `QFuture` wrappers or coroutines would add churn, not safety. `std::async` fits nowhere here (a discarded future blocks; no event-loop delivery).
