---
paths:
  - ".tasks/**/*.md"
---

# Ticket format (`.tasks/`)

The folders, the IDs and the "Saving tokens" rules are in the root `CLAUDE.md` ("Tasks and Bugs").

## A ticket file

- One Markdown file per task or bug: `<ID>-<short-kebab-title>.md` (e.g. `LI-10-copy-a-games-link-from-its.md`).
- Frontmatter, then `# <ID> <title>` and the ticket's text:
  ```yaml
  ---
  id: LI-10
  title: "Copy a game's link from its card"
  type: task        # task | bug
  status: open      # open | in_progress | done
  priority: medium  # high | medium | low
  ---
  ```
- A new ticket takes the next free number of its prefix, across both folders (`ls .tasks/open .tasks/archive`).
- A bug found on the way: what was found (the cause if known, where, how to see it).
- A ticket's plan and hand tests stay in `docs/` (`docs/<ID>-…Plan.md`, `docs/<ID>-ManualTests.md`), linked
  from the ticket.

## Working a ticket

- Starting it: `status: in_progress`.
- A ticket done in steps (one PR each, like BL-19 or LI-01A) stays in `open/`; each step's PR appends a short
  `### <YYYY-MM-DD> — <step> (branch: <branch>)` entry to its `## Resolution`: done, checked, what is left.
- Closing it (built, tested, and the owner's hand tests passed where the ticket has them):
  1. Append a short `## Resolution` entry to the ticket: the cause (a bug), the fix, the tests run and their result.
  2. Set `status: done` in its frontmatter.
  3. `git mv .tasks/open/<file>.md .tasks/archive/<file>.md`, in the same PR.
