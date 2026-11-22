---
id: BL-35
title: "Split ChessGame.qml: the sounds and the window's other parts into their own files"
type: task
status: open
priority: medium
---

# BL-35 Split ChessGame.qml: the sounds and the window's other parts into their own files

`ChessGui/ChessGui/qml/ChessGame.qml` (the window) has grown to about 1480 lines. The owner wants every sound the app
plays in one module of its own, and the window's other parts (messages, Lichess challenges, profile files, …) in
their own files where that pays.

Plan, for the owner's approval: [docs/BL-35-ChessGameSplitPlan.md](../../docs/BL-35-ChessGameSplitPlan.md). One PR
per step, the sound module first; the UI tests and the screenshots of the scenes named in the step green after
each.
