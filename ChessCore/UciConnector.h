#pragma once
#include "Move.h"
namespace Chess
{
    typedef std::map<std::string, std::string> UciOptionMapRaw;

    class UciConnector
    {
    public:
        UciConnector(std::istream &istr, std::ostream &ostr);

        UciOptionMapRaw Init();

        void StartPosition();

        bool IsReady();

        void PositionMoves(Move move);

        std::pair<Move, Move> Go();

        ~UciConnector();
    private:
        std::istream &_istr;
        std::ostream &_ostr;
    };
}

