import QtQuick
import QtQuick.Shapes
import ChessPlusPlus

// The selected piece's legal moves: a dot on each target square (ChessBoard.squareX/Y), or, where the
// move takes a piece, shaded corners and a red glow (BL-21: a dot lay on the piece and vanished on a
// black one). Two of these: `kind` "captures" under the pieces (the marks around a piece leave it as it
// is, as on lichess.org), "dots" over them. The 64 marks stay; a selection only shows and hides them
// (rebuilding them on every click made slides stop, BL-14).
Item {
    id: navigationLayer
    property string kind: "dots"
    // Read once per change, not by each of the 64 squares: " " = no move to that square.
    readonly property var moves: ChessConnector.PossibleMoves
    readonly property var captures: ChessConnector.CaptureSquares
    // Also once for all squares (BL-11): PremoveDots changes with every selection.
    readonly property bool premove: ChessConnector.PremoveDots
    readonly property color dotColor: premove ? Theme.premoveDot : Theme.legalMoveDot
    readonly property color cornersColor: premove ? Theme.premoveCorners : Theme.captureCorners

    // A capture's mark: the square's corners shaded (a ring-shaped gradient leaves the middle, where the
    // piece stands, clear), and under them a red glow along its edges. Made only on such a square.
    component CaptureMark: Item {
        id: mark
        required property bool glow

        Shape {
            anchors.fill: parent
            visible: mark.glow
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeWidth: -1
                fillGradient: RadialGradient {
                    centerX: mark.width / 2; centerY: mark.height / 2
                    focalX: centerX; focalY: centerY
                    centerRadius: mark.width * 0.71   // the corners: half the square's diagonal
                    GradientStop { position: 0.0;  color: "transparent" }
                    GradientStop { position: 0.5;  color: "transparent" }
                    GradientStop { position: 0.72; color: Qt.alpha(Theme.captureGlow, 0.3) }
                    GradientStop { position: 1.0;  color: Qt.alpha(Theme.captureGlow, 0.65) }
                }
                PathRectangle { width: mark.width; height: mark.height }
            }
        }
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeWidth: -1
                fillGradient: RadialGradient {
                    centerX: mark.width / 2; centerY: mark.height / 2
                    focalX: centerX; focalY: centerY
                    centerRadius: mark.width * 0.71
                    GradientStop { position: 0.0;  color: "transparent" }
                    GradientStop { position: 0.79; color: "transparent" }
                    GradientStop { position: 0.8;  color: navigationLayer.cornersColor }
                    GradientStop { position: 1.0;  color: navigationLayer.cornersColor }
                }
                PathRectangle { width: mark.width; height: mark.height }
            }
        }
    }

    Repeater{
        id:navigationLayerRptr
        objectName: "navigationLayerRptr"
        model: 64
        delegate:
            Item{
            id:navigationLayerItm
            required property int index
            readonly property bool target: AppSettings.showLegalMoves && (navigationLayer.moves[index] ?? " ") !== " "
            readonly property bool capture: target && (navigationLayer.captures[index] ?? false)
            readonly property bool shown: target && capture === (navigationLayer.kind === "captures")
            x: chessBoard.squareX(index)
            y: chessBoard.squareY(index)
            width: chessBoard.squareSize
            height: width

            // The dot or the corners; pops in when it appears.
            Item {
                id: navigationLayerRec
                anchors.fill: parent
                visible: navigationLayerItm.shown
                // Hiding stops it: a quick selection change would otherwise leave an invisible animation
                // running, which keeps the board rendering.
                onVisibleChanged: {
                    if (visible) {
                        if (!Theme.reduceMotion) navigationLayerPopIn.restart()
                    } else {
                        navigationLayerPopIn.stop()
                        navigationLayerRec.scale = 1
                    }
                }

                Rectangle {
                    objectName: "moveDot_" + navigationLayerItm.index
                    visible: navigationLayer.kind === "dots"
                    color: navigationLayer.dotColor
                    anchors.centerIn: parent
                    smooth: true
                    antialiasing: true
                    height: 0.3 * parent.height
                    width: height
                    radius: 0.5 * height
                }
                Loader {
                    objectName: "captureMark_" + navigationLayerItm.index
                    anchors.fill: parent
                    active: navigationLayer.kind === "captures" && navigationLayerItm.capture
                    visible: active
                    sourceComponent: CaptureMark { glow: !navigationLayer.premove && !Theme.highContrast }
                }
            }

            PropertyAnimation {
                id: navigationLayerPopIn
                target: navigationLayerRec;
                property: "scale";
                easing.type: Easing.InOutCirc
                from: 0;
                to: 1.0;
                duration: 450;
                loops: 1;
            }
        }
    }
}
