---
id: BL-32
title: "Split CLAUDE.md per project"
type: task
status: open
priority: medium
---

# BL-32 Split CLAUDE.md per project

The owner (2026-11-22): CLAUDE.md "got too fat and not every task requires all of the info". For example, the ChessGui section lists every file with its methods. Wanted: a short root CLAUDE.md, per-project descriptions it points to, and Claude Code's newer features used so agents load only what a task needs.

Found: the root `CLAUDE.md` is 207 lines and 40.7k characters, about 10k tokens loaded into every session and every subagent. The ChessGui section is 59% of it, much of it ticket history, and some of it is out of date ("Four projects": there are 8).

**Plan:** [`docs/BL-32-ClaudeMdSplitPlan.md`](../../docs/BL-32-ClaudeMdSplitPlan.md), for the owner's approval. It sets out:
- a root under 120 lines;
- a `CLAUDE.md` per project folder, loaded only when that folder's files are read;
- path-scoped rules (`.claude/rules/` with `paths:`) for the ChessGui C++ catalogue, C++ style and the ticket format;
- skills for procedures (`ticket`; `run-chess-app` split);
- a mechanical check that nothing is lost.

It's done in 7 docs PRs, a pilot first. Saving: about 6–7k tokens for most tasks and about 7.7k per subagent.

Also found (§7 of the plan): `.claude/settings.local.json` is tracked in git, and `ChessCore/ReadMe.txt` is wizard boilerplate.
