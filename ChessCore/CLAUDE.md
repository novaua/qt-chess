# ChessCore/

The chess rules: a static library, namespace `Chess::`, plain C++ with **no Qt** (FakeUci and ChessTests link it
without Qt). Its tests: `ChessTests/`.

## Key types
- `Board` — the 64 squares and where the pieces are; `CanMate` (whether a side has mating material: a flag then
  loses, else draws).
- `Game` — turns, move validation, events to listeners (observer: `EventBase` / `PawnPromotionEvent`);
  `GetPremoveTargets` (a premove's squares by the piece's shape, legality decided when it's played, LI-05);
  `MakeFen`; `FormatHistorySan(history, start)`; `LoadMoves` (UCI moves into the loaded history: a kept game's
  replay); `Save` / `Load` only migrate the binary saves of versions before BL-19 (`Serializer.h`).
- `Move` / `HistoryMove` — a move (from, to, capture, promotion) and a played one; `HistoryMove::Castling()`.
- `MoveGeneration` (`MovesGen.h`) — pseudo-legal and legal moves with castling and en passant; `Play` plays a
  validated move (castling with its rook, en passant); `HasCastlingRight`, `CastlingPath` (a side's castling
  squares when nothing stands in the way: the legal moves and the premove targets share it), `IsCastling`.
- `GameChecks` (`Check.h`) — check, mate, promotion state; `BoardPositionsCache` (attack maps, LRU) under it.
- `StartPosition` (LI-01A) — the set-up a game starts from: standard, or Chess960 #0–959 (Scharnagl, numbered as
  lichess.org does; #518 is the standard one); `Random()`, `FromFen` (a start position only), `Fen()`.
  `Game::Restart(whiteFirst, start)` (standard when none is given), `GetStartPosition()`; `LoadMoves`, `EndGame`
  and `FormatHistorySan` keep it.
- `BoardDiff` / `PieceTravel` — which piece went where between two positions (moved, removed, added): the board's
  piece animations.
- `HistoryPlayer` — steps through a history; `San.h` — `FormatMoveSan`, `FormatPgn` / `PgnTag`.

## Castling and FEN, any set-up
- In a **Chess960** game castling is written as the king onto its own rook (`f1g1`, `b1a1`), as Stockfish's
  `UCI_Chess960` and Lichess write it; a **standard** game keeps `e1g1`.
- `Board::DoCastling` moves both pieces in one step: they may swap, or one may stay where it is.
- FEN castling is X-FEN (`KQkq`, a file letter only when ambiguous): `Game::CastlingFen`.

## Invariants
- A castling is never a capture.
- No Qt and no GUI types here; nothing outside `Chess::`.
