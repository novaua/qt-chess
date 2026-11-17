"""Reference games for ChessTests/Chess960Tests.cpp (LI-01A), made with python-chess, a move generator
independent of ChessCore.

    python scripts/make_chess960_reference.py     (needs python-chess: pip install chess)

Writes ChessTests/Chess960Reference.h: random Chess960 games (a fixed seed, so the same every time) that
castle most times they may, each with
  - the number of legal moves before each move and after the last (distinct from-to pairs: a promotion
    counts once, as ChessCore makes one move of it),
  - the final FEN (X-FEN castling; en passant after every double step, as Game::MakeFen writes it),
  - the plies that castle, with their SAN.
"""
from pathlib import Path
import random
import chess

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'ChessTests' / 'Chess960Reference.h'
GAMES = 60
MAX_PLIES = 32


def legal_count(board):
    return len({(m.from_square, m.to_square) for m in board.legal_moves})


def play(number, rng):
    board = chess.Board.from_chess960_pos(number)
    board.chess960 = True
    moves, counts, castlings = [], [], []
    for ply in range(MAX_PLIES):
        legal = list(board.legal_moves)
        if not legal:
            break
        counts.append(legal_count(board))
        castling = [m for m in legal if board.is_castling(m)]
        if castling and rng.random() < 0.8:
            move = rng.choice(castling)
        else:
            # Pieces off the back rank first, half the time, so the castling squares clear.
            back = [m for m in legal if chess.square_rank(m.from_square) in (0, 7)
                    and board.piece_type_at(m.from_square) != chess.KING]
            move = rng.choice(back) if back and rng.random() < 0.5 else rng.choice(legal)
        if board.is_castling(move):
            castlings.append(f'{ply}:{board.san(move)}')
        moves.append(board.uci(move, chess960=True))
        board.push(move)
    counts.append(legal_count(board))
    return number, moves, counts, board.fen(en_passant='fen'), castlings


def main():
    rng = random.Random(960)
    games = [play(n, rng) for n in rng.sample(range(960), GAMES)]
    rows = []
    for number, moves, counts, fen, castlings in games:
        rows.append('    { %d,\n      "%s",\n      "%s",\n      "%s",\n      "%s" },' % (
            number, ' '.join(moves), ' '.join(map(str, counts)), fen, ' '.join(castlings)))
    plies = sum(len(g[1]) for g in games)
    castled = sum(len(g[4]) for g in games)
    OUT.write_text(
        '#pragma once\n'
        '// Made by scripts/make_chess960_reference.py (python-chess); don\'t edit by hand.\n'
        f'// {GAMES} Chess960 games, {plies} plies, {castled} castlings.\n\n'
        'namespace ChessTests {\n\n'
        'struct Reference960Game\n'
        '{\n'
        '    int Number;            // the start position (Scharnagl)\n'
        '    const char* Moves;     // UCI, castling as the king onto its rook\n'
        '    const char* Counts;    // legal moves (distinct from-to) before each move, and after the last\n'
        '    const char* Fen;       // the final position\n'
        '    const char* Castlings; // "ply:SAN" of each castling\n'
        '};\n\n'
        'static const Reference960Game Reference960Games[] = {\n'
        + '\n'.join(rows) + '\n'
        '};\n\n'
        '}\n',
        encoding='utf-8', newline='\r\n')
    print(f'{OUT}: {GAMES} games, {plies} plies, {castled} castlings')


if __name__ == '__main__':
    main()
