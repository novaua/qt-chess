#pragma once
#include "UciConnector.h"
#include "pstream.h"

namespace Chess
{
    class UciBasicConnector
    {
    public:
        UciBasicConnector();

        void NewGame();
        std::pair<Move, Move> MoveTo(const Move &move);

        ~UciBasicConnector();

    private:
        redi::pstream _engine;
        UciConnector _connector;
    };
}