---
id: TSK-13
title: "Re-implement network game support with local games without server"
type: task
status: open
priority: medium
---

# TSK-13 Re-implement network game support with local games without server

<!-- migrated from TASKS.md:345 (Network Multiplayer Game) -->

- [ ] Re-implement network game support with local games without server
     - Consider if it's worth doing at all

## Future direction: fast games on a service of our own (the owner, 2026-11-22)

Lichess's Board API, through which Chess++ plays on Lichess, never allows **Bullet** (1+0, 2+1) or
UltraBullet, and **Blitz** (3+2, 5+0, …) only in a challenge to a named player: no Blitz seek or open
challenge (lila's `Game.isBoardCompatible`; checked for LI-02, LI-01A and LI-09). Lichess's own website and
mobile app play every speed, but only they may: it is Lichess's rule for third-party clients. So the Quick
pairing card's Bullet and Blitz tiles are greyed and open lichess.org.

That leaves room for games between Chess++ players on a network service of our own, where every clock
works, Bullet and Blitz included. Open questions before it is a plan:
- hosting and cost (a server, or peer to peer as this ticket's "without server");
- accounts and identity (Chess++ profiles are local; Lichess accounts can't be used for it);
- matchmaking (a seek like LI-09's), ratings, and fair play (the reason Lichess limits fast games for apps);
- the app's side: the same game page, clock and premove as a Lichess game.
