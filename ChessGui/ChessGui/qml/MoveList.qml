import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import ChessPlusPlus

// The moves in two columns (White, Black) after a move-number column. Pieces are small images in
// their own colour with a glow of the other one; pawn moves and castling are text only. Click a move to see that position; the position on the
// board is highlighted.
ListView {
    id: list

    readonly property int shownPly: ChessConnector.ShownPly
    readonly property int numberWidth: 40
    readonly property real cellWidth: (width - numberWidth - Theme.s2) / 2

    function _showCurrent() {
        if (shownPly > 0) positionViewAtIndex((shownPly - 1) >> 1, ListView.Contain)
        else              positionViewAtBeginning()
    }

    clip: true
    model: ChessConnector.MoveHistory
    boundsBehavior: Flickable.StopAtBounds
    onShownPlyChanged: Qt.callLater(_showCurrent)
    onCountChanged:    Qt.callLater(_showCurrent)

    ScrollBar.vertical: ScrollBar {}

    Label {
        anchors.centerIn: parent
        visible: list.count === 0
        text: "No moves yet"
        font: Theme.body
        color: Theme.textSecondary
    }

    // One move: piece image and SAN; a click shows the position after it.
    component MoveCell: AbstractButton {
        id: cell
        property int    moveNumber
        property int    ply
        property string piece
        property string san

        readonly property bool current: ply === list.shownPly

        width: list.cellWidth
        height: 28
        hoverEnabled: true
        focusPolicy: Qt.NoFocus   // ←/→ stay with the window's review shortcuts
        Accessible.name: moveNumber + (ply % 2 === 1 ? ". " : "… ") + piece.toUpperCase() + san

        background: Rectangle {
            radius: 4
            color: cell.current ? Theme.selectedBg
                 : cell.hovered ? Theme.cardBgHover
                 : "transparent"
        }
        contentItem: Row {
            leftPadding: Theme.s1
            spacing: 1
            // The piece in its own colour, so White's and Black's moves differ at a glance. A glow of
            // the other colour keeps it visible on a light or a dark panel: black glows white,
            // white glows black.
            Image {
                visible: cell.piece !== ""
                anchors.verticalCenter: parent.verticalCenter
                width: 18; height: 18
                source: cell.piece === "" ? "" : Theme.piece(cell.piece)
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(36, 36)
                layer.enabled: visible
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: cell.piece === cell.piece.toLowerCase() ? Theme.pieceGlowOnBlack : Theme.pieceGlowOnWhite
                    shadowHorizontalOffset: 0
                    shadowVerticalOffset: 0
                    shadowBlur: 0.2
                    shadowScale: 1.14
                    blurMax: 6
                }
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: cell.san
                font.pixelSize: Theme.body.pixelSize
                font.weight: cell.current ? Font.DemiBold : Font.Normal
                font.features: { "tnum": 1 }
                color: Theme.textPrimary
            }
        }
        onClicked: ChessConnector.reviewTo(ply)
    }

    delegate: Item {
        id: row
        required property var modelData
        required property int index

        width: list.width
        height: 30

        Label {
            id: number
            width: list.numberWidth
            height: parent.height
            leftPadding: Theme.s2
            verticalAlignment: Text.AlignVCenter
            text: row.modelData.n + "."
            font.pixelSize: Theme.caption.pixelSize
            font.features: { "tnum": 1 }
            color: Theme.textSecondary
        }
        MoveCell {
            anchors.verticalCenter: parent.verticalCenter
            x: list.numberWidth
            moveNumber: row.modelData.n
            ply:   row.modelData.wPly
            piece: row.modelData.wPiece
            san:   row.modelData.wText
        }
        MoveCell {
            anchors.verticalCenter: parent.verticalCenter
            x: list.numberWidth + list.cellWidth + Theme.s2
            visible: row.modelData.bPly > 0
            moveNumber: row.modelData.n
            ply:   row.modelData.bPly
            piece: row.modelData.bPiece
            san:   row.modelData.bText
        }
    }
}
