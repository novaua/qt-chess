#include "stdafx.h"
#include "San.h"
#include "MovesGen.h"
#include <cstdlib>

using namespace Chess;

namespace {
    // Indexed by PieceTypes: EMPTY=0 KNIGHT=1 BISHOP=2 ROOK=3 QUEEN=4 KING=5
    static const char sanLetters[] = " NBRQK";

    char sanPieceLetter(PieceTypes type) {
        return (type >= KNIGHT && type <= KING) ? sanLetters[type] : '\0';
    }

    std::string posFile(BoardPosition pos) {
        return std::string(1, static_cast<char>('a' + (int)pos % 8));
    }

    std::string posRank(BoardPosition pos) {
        return std::string(1, static_cast<char>('1' + (int)pos / 8));
    }
}

bool Chess::IsCastling(const HistoryMove& m) {
    return m.From.Piece.Type == KING
        && std::abs((int)m.To.Position % 8 - (int)m.From.Position % 8) == 2;
}

bool Chess::IsEnPassant(const HistoryMove& m) {
    return m.From.Piece.Type == PAWN
        && m.To.Piece.Type == EMPTY
        && (int)m.To.Position % 8 != (int)m.From.Position % 8;
}

std::string Chess::FormatMoveSan(const HistoryMove& m,
                                 const Board& boardBefore,
                                 bool isCheck,
                                 bool isMate)
{
    auto fromPos = m.From.Position;
    auto toPos   = m.To.Position;
    auto piece   = m.From.Piece;

    if (IsCastling(m))
        return ((int)toPos % 8 > (int)fromPos % 8) ? "O-O" : "O-O-O";

    std::string result;

    if (piece.Type == PAWN) {
        bool isCapture = m.IsCapturingMove() || IsEnPassant(m);
        result = isCapture
            ? posFile(fromPos) + "x" + BoardPositionToString(toPos)
            : BoardPositionToString(toPos);
        if (m.IsPawnPromotionMove()) {
            result += "=";
            result += sanPieceLetter(m.PromotedTo.Type);
        }
    } else {
        char sym = sanPieceLetter(piece.Type);
        auto candidates = MoveGeneration::GetPositionsOf(boardBefore, piece.Type, piece.Color);
        bool needsRank = false, needsFile = false;
        for (const auto& pp : candidates) {
            if (pp.Position == fromPos) continue;
            for (const auto& mv : MoveGeneration::GenerateBasicMoves(
                    boardBefore, pp.Position, piece.Color)) {
                if (mv.To == toPos) {
                    if ((int)pp.Position % 8 == (int)fromPos % 8)
                        needsRank = true;
                    else
                        needsFile = true;
                }
            }
        }
        std::string disambig;
        if      (needsRank && needsFile) disambig = posFile(fromPos) + posRank(fromPos);
        else if (needsRank)              disambig = posRank(fromPos);
        else if (needsFile)              disambig = posFile(fromPos);

        result  = sym;
        result += disambig;
        result += (m.IsCapturingMove() ? "x" : "");
        result += BoardPositionToString(toPos);
    }

    if (isMate)       result += "#";
    else if (isCheck) result += "+";
    return result;
}
