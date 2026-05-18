#pragma once
#include <string>
#include "Move.h"
#include "Board.h"

namespace Chess {

    bool IsCastling(const HistoryMove& m);

    // Pawn moves diagonally to an empty square — geometrically identifies en passant.
    bool IsEnPassant(const HistoryMove& m);

    // Formats m as Standard Algebraic Notation (SAN).
    // boardBefore: state before the move — needed for piece disambiguation.
    // boardAfter:  state after the move  — used to detect check.
    // isMate: caller must determine this (requires full legal-move generation).
    std::string FormatMoveSan(const HistoryMove& m,
                              const Board& boardBefore,
                              const Board& boardAfter,
                              bool isMate);
}
