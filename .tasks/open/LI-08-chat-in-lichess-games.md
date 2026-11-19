---
id: LI-08
title: "Chat in Lichess games"
type: task
status: open
priority: medium
---

# LI-08 Chat in Lichess games

<!-- migrated from TASKS.md:295 (Lichess Improvement Backlog) -->

- [ ] LI-08 Chat in Lichess games [Medium], **not started; needs a plan** (the owner, 2026-11-19; BL-07's chat). Chat with the opponent as lichess.org allows, not as a panel: a **round chat button in the game page's bottom right corner**, over the board area, with a count of unread messages, which opens a small chat window above it. It should look familiar to Lichess players.
  - **Board API** (to confirm in the plan): the game stream's `chatLine` lines (`room`: `player` or `spectator`, `username`, `text`); `POST /api/board/game/{id}/chat` (`room=player`, `text`); `GET /api/board/game/{id}/chat` for the chat so far when a game is rejoined (LI-04, BL-18). The `board:play` scope covers it. `gameFull` also has the chat's state; to check whether the opponent or Lichess has turned chat off (kid mode, a closed chat).
  - **The button:** shown only in a Lichess game (not robot, two players or replay) while the chat is on. A badge with the unread count, and a short pulse when a message comes in (still with Reduce animations). A new message plays a soft sound and goes to Recent messages (`MessagesFlyout`). It must not cover the board's squares or the moves panel's controls; in a narrow window it sits above the moves sheet's handle.
  - **The chat window:** a popup anchored to the button, not modal, so the game goes on behind it, and the board stays playable when it doesn't overlap. It holds:
    - the lines as on lichess.org: "name: text", the player's own lines marked, Lichess's own lines (draw offered, takeback…) in grey italics;
    - an input of up to 140 characters (Lichess's limit), sent with Enter;
    - the preset phrases as chips: Hello, Good luck, Have fun!, You too!, Good game, Well played, Thank you, I've got to go, Bye!;
    - a switch to turn chat off for the game, as on lichess.org.
  - Esc or a click outside closes it; opening it marks everything read. It stays open across moves, and closes when the game ends and the result card shows. Keyboard: a shortcut to open it, with focus in the input.
  - **C++:** `LichessClient` parses `chatLine` and posts chat (`chatReceived(username, text, room)`, `sendChat(text)`; spectator lines ignored); `ChessConnector` keeps the game's chat as a model (`Chat`, `ChatUnread`, `sendChat`), cleared when the game changes. Not kept after the game (Lichess games have no record).
  - **Tests (chat):** `LichessStreamTest` / `LichessClientTest` for `chatLine` and the POST; `FakeLichessServer` that chats back; `tst_lichess`: the badge counts, the window opens, a phrase is sent, Esc closes it. Scenes: `chat-badge`, `chat-open`, light and dark, narrow. A hand test on lichess.org with two accounts.
