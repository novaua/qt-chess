import QtQuick
import ChessPlusPlus

// The selected piece's legal moves: a dot on each target square (ChessBoard.squareX/Y), or, where the
// move takes a piece, the square's four corners in the dot's colour, as on lichess.org (BL-21: a dot lay
// on the piece and vanished on a black one): the dot's colour moved out to where no piece covers it; the
// square and the piece stay as they are. Two of these: `kind` "captures" under the pieces, "dots" over
// them; each makes only its own marks. The 64 marks stay; a selection only shows and hides them
// (rebuilding them on every click made slides stop, BL-14).
Item {
    id: navigationLayer
    property string kind: "dots"
    // From ChessBoard, read there once per change for both layers (BL-11): " " = no move to that square;
    // which targets take a piece (ChessConnector.CaptureSquares); premove targets.
    required property var moves
    required property var captures
    required property bool premove
    readonly property color dotColor: premove ? Theme.premoveDot : Theme.legalMoveDot
    readonly property bool captureLayer: kind === "captures"

    // One square's mark, on its square: shown while the square is a target of this layer's kind (a
    // capture or not), popping in when it appears.
    component MoveMark: Item {
        id: mark
        required property int index
        default property alias content: body.data
        readonly property bool target: AppSettings.showLegalMoves && (navigationLayer.moves[index] ?? " ") !== " "
        readonly property bool capture: target && (navigationLayer.captures[index] ?? false)
        x: chessBoard.squareX(index)
        y: chessBoard.squareY(index)
        width: chessBoard.squareSize
        height: width

        Item {
            id: body
            anchors.fill: parent
            visible: mark.target && mark.capture === navigationLayer.captureLayer
            // Hiding stops the pop-in: a quick selection change would otherwise leave an invisible
            // animation running, which keeps the board rendering.
            onVisibleChanged: {
                if (visible) {
                    if (!Theme.reduceMotion) popIn.restart()
                } else {
                    popIn.stop()
                    body.scale = 1
                }
            }
        }
        PropertyAnimation {
            id: popIn
            target: body
            property: "scale"
            easing.type: Easing.InOutCirc
            from: 0
            to: 1.0
            duration: 450
        }
    }

    Component {
        id: dotMark
        MoveMark {
            id: square
            Rectangle {
                objectName: "moveDot_" + square.index
                color: navigationLayer.dotColor
                anchors.centerIn: parent
                antialiasing: true
                height: 0.3 * parent.height
                width: height
                radius: 0.5 * height
            }
        }
    }
    Component {
        id: cornersMark
        MoveMark {
            id: square
            // The square minus a circle (its radius 0.8 of the half diagonal, as lichess.org's gradient):
            // a ring whose inner edge is that circle, wide enough to reach past the corners, clipped
            // to the square.
            Item {
                objectName: "captureMark_" + square.index
                anchors.fill: parent
                clip: true
                Rectangle {
                    readonly property real inner: 0.8 * Math.SQRT1_2 * parent.width
                    readonly property real outer: Math.SQRT1_2 * parent.width + 1
                    anchors.centerIn: parent
                    width: 2 * outer
                    height: width
                    radius: outer
                    color: "transparent"
                    border.width: outer - inner
                    border.color: navigationLayer.dotColor
                    antialiasing: true
                }
            }
        }
    }

    Repeater {
        id: navigationLayerRptr
        objectName: "navigationLayerRptr"
        model: 64
        delegate: navigationLayer.captureLayer ? cornersMark : dotMark
    }
}
