#pragma once
#include <string>
#include "Move.h"
#include "Board.h"

namespace Chess {

    bool IsCastling(const HistoryMove& m);

    // Pawn moves diagonally to an empty square — geometrically identifies en passant.
    bool IsEnPassant(const HistoryMove& m);

    // Formats m as Standard Algebraic Notation (SAN).
    // boardAfter: state after the move; BeforeLastMove() is used for disambiguation.
    // isMate: caller must determine this (requires full legal-move generation).
    std::string FormatMoveSan(const HistoryMove& m,
                              const Board& boardAfter,
                              bool isMate);
}
