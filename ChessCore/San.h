#pragma once
#include <string>
#include "Move.h"
#include "Board.h"

namespace Chess {

    bool IsCastling(const HistoryMove& m);

    // Pawn moves diagonally to an empty square — geometrically identifies en passant.
    bool IsEnPassant(const HistoryMove& m);

    // Formats m as Standard Algebraic Notation (SAN).
    // boardBefore: board state BEFORE m was applied, needed for disambiguation.
    // isCheck / isMate: caller must determine these; only the suffix is appended here.
    std::string FormatMoveSan(const HistoryMove& m,
                              const Board& boardBefore,
                              bool isCheck,
                              bool isMate);
}
