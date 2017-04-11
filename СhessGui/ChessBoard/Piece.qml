import QtQuick 2.0

Item  {
    id: piece
    property int eType: 0
    property int eColor: 1
    Image {
        id: img
        source : {
            if (eColor == 1) //white
            {
                if (eType == 0)
                {
                    ":/images/pawn_white.svg"
                } else if (eType == 1)
                {
                    ":/images/knight_white.svg"
                }else if (eType == 2)
                {
                    ":/images/bishop_white.svg"
                }else if (eType == 3)
                {
                    ":/images/rook_white.svg"
                } else
                {
                    ":/images/exp.png"
                }
            } else {
                ":/images/pawn_black.svg"
            }
        }
    }
}

