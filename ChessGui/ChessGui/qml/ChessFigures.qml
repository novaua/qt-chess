import QtQuick
import ChessPlusPlus

// The pieces of ChessConnector.Pieces, each on its square (ChessBoard.squareX/Y). A piece keeps
// its item while it moves, so it slides; a captured one fades out (BL-04). Reduce animations:
// pieces jump and appear at once.
// A piece of the side to move can also be dragged: ChessBoard shows it under the pointer
// (dragFrom/dragPoint) while it stays faded on its square; a drop plays the move or it slides back.
Item {
    id: chessFigures

    Repeater {
        id:chessPiecesRptr
        objectName: "chessPiecesRptr"
        model: ChessConnector.Pieces

        delegate: Item {
            id:chessPiecesItm

            required property int pieceId
            required property int square
            required property string piece
            required property bool taken

            // A piece that changes square slides there: slideFile/slideRank are how far (in squares)
            // it still is from it, animated to 0. In squares, not pixels, so resizing the window
            // doesn't slide anything. A new position (model reset) has new items, which fade in.
            property real slideFile: 0
            property real slideRank: 0
            property int lastSquare: -1
            x: chessBoard.squareX(square) + slideFile * chessBoard.squareSize
            y: chessBoard.squareY(square) - slideRank * chessBoard.squareSize
            width: chessBoard.squareSize
            height: width
            z: taken ? 0 : 1   // the piece that captures slides over the one fading out

            onSquareChanged: {
                if (lastSquare >= 0) {
                    // From where it is drawn now, also when a slide is still under way, or from
                    // where it was dropped (then it only settles into the square).
                    slideFile += lastSquare % 8 - square % 8
                    slideRank += Math.floor(lastSquare / 8) - Math.floor(square / 8)
                    settle()
                }
                lastSquare = square
                dropTimer.stop()
            }
            function settle() {
                if (Theme.reduceMotion) {
                    chessPiecesSlide.stop()
                    slideFile = 0
                    slideRank = 0
                    shownPiece = piece
                } else {
                    chessPiecesSlide.restart()
                }
            }
            ParallelAnimation {
                id: chessPiecesSlide
                NumberAnimation {
                    target: chessPiecesItm; property: "slideFile"; to: 0
                    duration: Theme.slow; easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    target: chessPiecesItm; property: "slideRank"; to: 0
                    duration: Theme.slow; easing.type: Easing.OutCubic
                }
                onFinished: chessPiecesItm.shownPiece = chessPiecesItm.piece
            }

            // A promoting pawn stays a pawn until it has arrived (and a queen taken back turns into
            // a pawn once back on the 7th rank). The choice after a pawn's move changes it in place.
            property string shownPiece: piece
            onPieceChanged: Qt.callLater(() => { if (!chessPiecesSlide.running) shownPiece = piece })

            // Fades in when it appears (shown is set once created), fades out when captured.
            property bool shown: false
            opacity: shown && !taken ? 1 : 0
            visible: opacity > 0
            Behavior on opacity {
                enabled: !Theme.reduceMotion
                NumberAnimation { duration: Theme.normal; easing.type: Easing.InOutQuad }
            }
            Component.onCompleted: {
                lastSquare = square
                shown = true
            }
            Component.onDestruction: if (dragged) chessBoard.endDrag()   // e.g. a new position

            // ── Dragging ──
            readonly property bool dragged: chessBoard.dragFrom === square && !taken
            // Only the side to move, when the player may move (ChessConnector.MovableColor).
            readonly property bool movable: !taken
                && ChessConnector.MovableColor === (piece === piece.toUpperCase() ? 1 : 2)

            HoverHandler {
                enabled: chessPiecesItm.movable
                cursorShape: Qt.OpenHandCursor
            }

            // target null: the item stays bound to its square; ChessBoard draws the piece under the
            // pointer. A press without a drag is a click (ChessBoard's TapHandler). Stays enabled
            // until released, so a drag that can't be played any more (Left: review) still ends.
            DragHandler {
                id: dragHandler
                target: null
                enabled: chessPiecesItm.movable || active
                cursorShape: Qt.ClosedHandCursor
                onActiveChanged: {
                    if (active)                        chessPiecesItm.startDrag()
                    else if (chessPiecesItm.dragged)   chessPiecesItm.drop()
                    else                               chessBoard.dragCancelled = false   // released after Esc
                }
                onCentroidChanged: if (active && chessPiecesItm.dragged) chessPiecesItm.followPointer()
            }
            // Esc: put back at once; the release that follows does nothing more.
            Connections {
                target: chessBoard
                enabled: chessPiecesItm.dragged
                function onDragCancelledChanged() { if (chessBoard.dragCancelled) chessPiecesItm.drop() }
            }

            function followPointer() {
                // In the squares' coordinates, so it is right on the turned board too.
                chessBoard.dragPoint = chessFigures.mapFromItem(null, dragHandler.centroid.scenePosition)
            }

            function startDrag() {
                chessBoard.dragCancelled = false
                if (ChessConnector.SelectedSquare !== square)
                    ChessConnector.figureSelected(square)   // its move dots
                chessBoard.dragPiece = piece
                followPointer()
                chessBoard.dragFrom = square
            }

            function drop() {
                const from = square
                const to = chessBoard.dragCancelled ? -1 : chessBoard.dropSquare
                // Drawn where it was dropped: the offset from its square, then it slides from there.
                slideFile = (chessBoard.dragPoint.x - width / 2 - chessBoard.squareX(square)) / width
                slideRank = -(chessBoard.dragPoint.y - height / 2 - chessBoard.squareY(square)) / width
                chessBoard.endDrag()
                if (to === from)
                    settle()            // back on its square, still selected
                else if (ChessConnector.movePiece(from, to))
                    dropTimer.start()   // played: its square changes with the next sync
                else
                    settle()            // illegal square, off the board (-1), Esc: slides back, dots cleared
            }
            // A played drop that didn't change the square after all: back to it.
            Timer { id: dropTimer; interval: 500; onTriggered: chessPiecesItm.settle() }

            // The board turns 180° for Black; the piece turns with it the other way, to stay upright.
            rotation: chessBoard.angle
            Behavior on rotation {
                enabled: !Theme.reduceMotion
                NumberAnimation {
                    id: chessPiecesRotation
                    easing {
                        type: Easing.InCirc
                        amplitude: 6.0
                        period: 30
                    }
                }
            }

            Image{
                id:chessPiecesImg
                anchors.centerIn: chessPiecesItm
                smooth: true
                antialiasing:true
                height:0.9*parent.height
                width:height
                fillMode: Image.PreserveAspectFit
                source: Theme.piece(chessPiecesItm.shownPiece)
                opacity: chessPiecesItm.dragged ? 0.35 : 1   // the dragged piece's place (ChessBoard draws it)
            }

            // Red flag on the king that resigned / lost on time
            Text {
                visible: chessPiecesItm.square === ChessConnector.FlaggedKing
                anchors { top: parent.top; right: parent.right; margins: parent.width * 0.04 }
                text: "⚑"
                color: Theme.flaggedKing
                style: Text.Outline
                styleColor: Theme.textOnColor
                font.pixelSize: parent.height * 0.36
                font.bold: true
            }
        }
    }
}
