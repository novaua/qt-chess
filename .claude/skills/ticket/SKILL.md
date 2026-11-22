---
name: ticket
description: File, start, add a step entry to, or close a ticket in .tasks/ (BL-, LI-, CE-). Use when creating a new task or bug (also one found on the way), starting work on a ticket, finishing a PR step of one, or closing it.
---

# Working a ticket

The format (file name, frontmatter, step entry, closing) is `.claude/rules/tasks.md`. **Read it first** when
writing a new ticket: it loads by itself only after a ticket was read or written. The folders, the IDs and
"Saving tokens" are in the root `CLAUDE.md`: never read all of `.tasks/open/`, nor `.tasks/archive/` unless
asked.

## A new ticket
1. Pick the prefix: `BL-` the app (bugs too), `LI-` playing on Lichess, `CE-` the chess rules; a variant of a
   ticket gets a letter (`LI-01A`, `LI-01B`).
2. The next free number, across both folders:
   ```powershell
   $p = 'BL'; (Get-ChildItem .tasks/open, .tasks/archive -Name | Select-String "^$p-(\d+)" |
     ForEach-Object { [int]$_.Matches[0].Groups[1].Value } | Measure-Object -Maximum).Maximum + 1
   ```
3. Write `.tasks/open/<ID>-<short-kebab-title>.md`: the frontmatter (`status: open`), `# <ID> <title>`, then
   what is wanted, or for a bug what was found: the cause if known, where, how to see it.
4. Found on the way: file it now, in the PR at hand, instead of writing it into `PROGRESS.md` or another
   ticket; say so in the PR and in the report.

## Starting one
- Read only that ticket (and its `docs/<ID>-…Plan.md` when it links one).
- A new branch from the latest `origin/develop` (the root's Branching Workflow); set `status: in_progress`.
- A ticket without a plan that needs one (several steps, a design choice): write the plan in `docs/` first, on
  a `docs/…` branch, for the owner's approval.

## Each PR of it
- A ticket done in steps stays in `open/`: append its `### <YYYY-MM-DD> — <step> (branch: <branch>)` entry to
  `## Resolution` (done, checked, what is left).
- Overwrite `PROGRESS.md` (the root's Session Handover: under 15 lines).

## Closing it
1. Only when built, tested and, where the ticket has hand tests (`docs/<ID>-ManualTests.md`), the owner says
   they passed: **ask**, never assume.
2. Append the `## Resolution` entry: the cause (a bug), the fix, the tests run and their result.
3. `status: done`, then `git mv .tasks/open/<file>.md .tasks/archive/<file>.md`, in the same PR.
