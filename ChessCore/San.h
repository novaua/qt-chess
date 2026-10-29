#pragma once
#include <string>
#include <vector>
#include "Move.h"
#include "Board.h"

namespace Chess {

    // Formats m as Standard Algebraic Notation (SAN).
    // boardAfter: state after the move; BeforeLastMove() is used for disambiguation.
    // isMate: caller must determine this (requires full legal-move generation).
    std::string FormatMoveSan(const HistoryMove& m,
                              const Board& boardAfter,
                              bool isMate);

    struct PgnTag
    {
        std::string Name;
        std::string Value;
    };

    // A PGN game: the tags, a blank line, then the numbered moves wrapped at 80 columns and
    // the result ("1-0", "0-1", "1/2-1/2" or "*"). Ends with a newline.
    std::string FormatPgn(const std::vector<PgnTag>& tags,
                          const std::vector<std::string>& sanMoves,
                          const std::string& result);
}
