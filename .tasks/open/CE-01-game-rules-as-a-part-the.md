---
id: CE-01
title: "Game rules as a part the game is made with"
type: task
status: open
priority: medium
---

# CE-01 Game rules as a part the game is made with

<!-- migrated from TASKS.md:173 (Chess Core Extensions) -->

- [ ] CE-01 Game rules as a part the game is made with [Hard], **not started; needs a plan, made with the owner step by step** (the owner, 2026-11-19). `Game` is made with a variant's **rules**, instead of standard chess's rules hard-wired in `MovesGen`, `Check`, `Game` and `Board`. This is the groundwork for more variants: Lichess's Antichess, Crazyhouse (LI-01B), Atomic, King of the Hill, Three-check, Horde, Racing Kings, and maybe our own.
  - **Today:** the rules are standard chess, spread over:
    - `MovesGen` (castling, en passant, promotion);
    - `Check` (check, mate);
    - `Game` (stalemate, the draw rules, the result; repetition in `BoardPositionsCache`);
    - `Board::CanMate` (mating material);
    - `San` / FEN (notation).
  - Chess960 (LI-01A) is a `StartPosition` plus a few `IsChess960()` branches (castling in `Move.cpp`, `Game.cpp`, `San.cpp`). That works for one variant, but every further variant would add more such branches.
  - **What differs between variants** (the plan lists each variant against these):
    - the start position;
    - the pieces and how they move;
    - whether captures are forced (Antichess);
    - whether there is check at all, and whether the king can be taken (Antichess, Atomic);
    - what a move does besides moving: Atomic's explosion, Crazyhouse's pocket and drops (`P@e4`);
    - promotion (to a king in Antichess; a promoted piece goes back to the pocket as a pawn in Crazyhouse);
    - castling;
    - how a game is won: mate, losing every piece (Antichess), the king in the centre (King of the Hill), three checks, the king to the 8th rank (Racing Kings);
    - the draw rules and mating material;
    - extra state in the position: the pockets, the checks given; FEN's extensions (`[Pp]`, `+1+2`);
    - the SAN / UCI notation.
  - **Design (to choose in the plan; candidates):**
    - **Strategy:** an interface (`Rules` / `Variant`) that `Game` gets at `Restart` (`std::shared_ptr<const Rules>`). Its hooks:
      - the legal moves;
      - the side effects of a move, and their undo;
      - check;
      - the game's outcome;
      - mating material;
      - FEN / SAN.
      - Standard chess is the default implementation; a variant overrides only what it changes. A **Template Method** in the base keeps the common steps (generate, filter, apply), with the variant's hooks inside.
    - **Data-driven rules**, as Fairy-Stockfish does (`variants.ini`): a variant is a set of options (forced captures, a king that can be taken, drops, win conditions, piece types). It is cheap for many variants, but some of them (Atomic's explosion) still need code.
    - **Composition / Decorator:** small rule parts combined into a variant (drops + pocket, forced capture, an extra win condition).
    - **A factory / registry** keyed by Lichess's variant names (`standard`, `chess960`, `antichess`, `crazyhouse`, …), used by records, `LichessClient::playableVariant` and the setup dialogs.
    - Recommended to look at first: a Strategy interface with data options for the simple differences, Chess960 moved into it, and the registry.
  - **Extra state:** the game state gets the variant's own state (pockets, checks given). `HistoryMove` records what is needed to undo it (`Game::UndoMove`, takebacks); `BoardDiff` / `BoardPieces` learn drops and explosions.
  - **Steps, each a PR:**
    - 1 the plan, with the owner;
    - 2 a refactor with **no change in behaviour**: standard chess and Chess960 through the `Rules` interface, `IsChess960()` branches gone, every existing test green (`ChessTests`, the Chess960 reference games, the UI tests);
    - 3 the first new variant, as the proof: the plan picks it (Antichess is small, Crazyhouse is LI-01B's);
    - then one variant per task.
  - **Tests:**
    - per variant, random games checked ply by ply against **python-chess**, which has all of Lichess's variants (`chess.variant`), as `Chess960Reference.h` is made today (`scripts/make_chess960_reference.py`);
    - perft counts for known positions;
    - the outcome rules.
  - **Out of this task, listed for the plan:**
    - the UI per variant: pockets, a checks counter, the explosion;
    - the engine: Stockfish plays only standard and Chess960, so the robot needs a variant engine (Fairy-Stockfish, BL-06) or no robot for those variants;
    - FakeUci;
    - the record's `variant`, PGN's `Variant` tag;
    - Lichess's variants in `playableVariant`.
  - **The interactive planning stage:** the plan (`docs/CE-01-GameRulesPlan.md`) is written with the owner. First a short survey of where the rules live and of the candidate designs, with a small sketch of each, then the owner's choices asked one question at a time before it is final:
    - the design;
    - which variants are in scope and in what order;
    - the first variant;
    - whether variants are only for Lichess or also local and against the robot;
    - the engine.
