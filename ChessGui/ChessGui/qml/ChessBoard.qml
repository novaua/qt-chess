import QtQuick
import ChessPlusPlus

Rectangle {
    id: chessBoard

    property int angle:0;
    rotation:angle;
    color: "transparent"
    readonly property bool turning: chessBoardRotation.running // turning to the other side (pieces turn with it)

    // The squares, in the board's own coordinates: a1 at the bottom left, h8 at the top right.
    // For Black the whole board turns (`rotation`); pieces, dots and labels turn back to stay upright.
    readonly property real squaresSize: Theme.boardSquares * Math.min(width, height)
    readonly property real squareSize: squaresSize / 8
    function squareX(square) { return (square % 8) * squareSize }
    function squareY(square) { return (7 - Math.floor(square / 8)) * squareSize }
    // The square at a point of the squares area (chessBoxes), or -1 outside it.
    function squareAt(x, y) {
        const file = Math.floor(x / squareSize)
        const row = Math.floor(y / squareSize)
        return (file < 0 || file > 7 || row < 0 || row > 7) ? -1 : (7 - row) * 8 + file
    }

    // The piece being dragged (ChessFigures' DragHandler sets it; the drag scene too): its square, its
    // key ("N"), and the pointer in the squares' coordinates. -1: no drag. Esc cancels it.
    property int dragFrom: -1
    property string dragPiece: ""
    property point dragPoint
    property bool dragCancelled: false
    readonly property bool dragging: dragFrom >= 0
    readonly property int dropSquare: dragging ? squareAt(dragPoint.x, dragPoint.y) : -1
    function endDrag() { dragFrom = -1; dragPiece = "" }

    Shortcut {
        sequence: "Escape"
        enabled: chessBoard.dragging
        onActivated: chessBoard.dragCancelled = true
    }

    Behavior on rotation {
        id:chessBoardBehavior
        enabled: !Theme.reduceMotion
        NumberAnimation {
            id: chessBoardRotation
            easing {
                type: Easing.InElastic
                amplitude: 1.0
                period: 40
            }
        }
    }

    ChessBoxes{
        id: chessBoxes
        anchors.centerIn: parent
    }

    ChessFigures{
        id:chessPiecesGrid
        objectName:"chessPiecesGrid"
        anchors.fill: chessBoxes

        // A click on a square: select a piece (its moves show as dots), or play the selected
        // piece's move to it.
        TapHandler {
            onTapped: (eventPoint) => {
                const square = chessBoard.squareAt(eventPoint.position.x, eventPoint.position.y)
                if (square >= 0)
                    ChessConnector.figureSelected(square)
            }
        }
    }

    MarkerColumn {
        id:markerColumnLeft
        visible: !AppSettings.coordinatesInside
        anchors.top: chessBoxes.top
        anchors.topMargin: 0
        anchors.right: chessBoxes.left
        anchors.rightMargin: 0
    }

    MarkerColumn {
        id:markerColumnRight
        visible: !AppSettings.coordinatesInside
        anchors.top: chessBoxes.top
        anchors.topMargin: 0
        anchors.left: chessBoxes.right
        anchors.rightMargin: 0
    }

    MarkerRow {
        id:markerRowTop
        visible: !AppSettings.coordinatesInside
        antialiasing: true
        anchors.bottom:  chessBoxes.top
        anchors.bottomMargin: 0
        anchors.left:  chessBoxes.left
        anchors.leftMargin: 0
    }

    MarkerRow {
        id:markerRowBottom
        visible: !AppSettings.coordinatesInside
        antialiasing: true
        anchors.top:  chessBoxes.bottom
        anchors.bottomMargin: 0
        anchors.left:  chessBoxes.left
        anchors.leftMargin: 0
    }

    // Coordinates on the squares (Settings → Board coordinates): the files along the bottom rank and
    // the ranks along the left file, as the player sees the board. Laid out in the board's own
    // coordinates (a1 bottom left), so when it is turned for Black they go to rank 8 and the h-file,
    // and each label turns back upright.
    Item {
        id: insideCoordinates
        anchors.fill: chessBoxes
        visible: AppSettings.coordinatesInside
        readonly property bool turned: (chessBoard.angle % 360) !== 0
        readonly property real square: width / 8

        component Coordinate: Item {
            id: coordinate
            property int rank    // 0 = rank 1
            property int file    // 0 = a-file
            property alias text: label.text
            property bool alignRight   // seen from the player: bottom right (files) or top left (ranks)
            x: file * insideCoordinates.square
            y: (7 - rank) * insideCoordinates.square
            width: insideCoordinates.square
            height: width
            rotation: insideCoordinates.turned ? 180 : 0
            Text {
                id: label
                anchors {
                    margins: coordinate.width * 0.05
                    right: coordinate.alignRight ? parent.right : undefined
                    bottom: coordinate.alignRight ? parent.bottom : undefined
                    left: coordinate.alignRight ? undefined : parent.left
                    top: coordinate.alignRight ? undefined : parent.top
                }
                font.pixelSize: Math.max(8, coordinate.width * 0.2)
                font.weight: Font.DemiBold
                // The colour of the other squares: a1 (rank + file even) is dark.
                color: (coordinate.rank + coordinate.file) % 2 === 0 ? Theme.squareLight : Theme.squareDark
            }
        }

        Repeater {
            model: 8
            Coordinate {
                required property int index
                rank: insideCoordinates.turned ? 7 : 0
                file: index
                text: "abcdefgh"[index]
                alignRight: true
            }
        }
        Repeater {
            model: 8
            Coordinate {
                required property int index
                rank: index
                file: insideCoordinates.turned ? 7 : 0
                text: index + 1
                alignRight: false
            }
        }
    }

    // Messages (check, saved, …) are InfoBars in ChessGame.qml, not overlays on the board.
    Connections {
        target: ChessConnector
        function onPawnPromotionNotify(index, side) { showPawnPromotionOptions(index, side) }
    }

    NavigationLayer{
        id:navigationLayerGrid
        objectName:"navigationLayerGrid"
        anchors.fill: chessBoxes
    }

    // The dragged piece under the pointer, above the move dots; its own square shows it faded.
    Item {
        anchors.fill: chessBoxes
        visible: chessBoard.dragging
        Image {
            x: chessBoard.dragPoint.x - width / 2
            y: chessBoard.dragPoint.y - height / 2
            width: chessBoard.squareSize
            height: width
            scale: 1.1   // lifted
            rotation: chessBoard.angle   // upright on the turned board, like the pieces
            smooth: true
            antialiasing: true
            fillMode: Image.PreserveAspectFit
            source: chessBoard.dragPiece ? Theme.piece(chessBoard.dragPiece) : ""
        }
    }

    Loader{
        id:loader
        anchors.centerIn: parent
        opacity: 0
        width: parent.height<parent.width?0.5*parent.height:0.5*parent.width
        height:   0.4*width
        // Cancel the board's rotation (180° when playing Black) so the dialog stays upright.
        rotation: -chessBoard.angle

        property int index;
        property int side;

        Behavior on opacity  {
            NumberAnimation {
                easing {
                    type: Easing.InCirc
                    amplitude: 6.0
                    period: 30
                }
            }
        }
    }

    function showPawnPromotionOptions(index, side)
    {
        loader.index=index
        loader.side=side
        loader.source="PawnPromotionOptions.qml";
        loader.opacity=0.8
    }
}
