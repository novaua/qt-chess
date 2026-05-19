#pragma once
#include <string>
#include "Move.h"
#include "Board.h"

namespace Chess {

    // Formats m as Standard Algebraic Notation (SAN).
    // boardAfter: state after the move; BeforeLastMove() is used for disambiguation.
    // isMate: caller must determine this (requires full legal-move generation).
    std::string FormatMoveSan(const HistoryMove& m,
                              const Board& boardAfter,
                              bool isMate);
}
