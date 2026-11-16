import QtQuick
import QtQuick.Controls
import ChessPlusPlus

// The analysis's evaluation of the position on the board (BL-19 step 7), beside the board: White's
// share of the height is White's chance to win, from the board's White side (the bottom, or the top
// when the board is turned). The number shows on hover.
Rectangle {
    id: bar

    property real whitePercent: 50
    property string evalText
    property bool whiteAtBottom: true

    objectName: "evalBar"
    radius: width / 2
    color: "#404040"

    Accessible.role: Accessible.Indicator
    Accessible.name: "Evaluation " + evalText

    Rectangle {
        width: parent.width
        height: Math.round(parent.height * Math.max(0, Math.min(100, bar.whitePercent)) / 100)
        y: bar.whiteAtBottom ? parent.height - height : 0
        radius: bar.radius
        color: "#FFFFFF"
        Behavior on height { enabled: !Theme.reduceMotion; NumberAnimation { duration: Theme.slow; easing.type: Easing.OutCubic } }
    }
    // The middle: even.
    Rectangle {
        width: parent.width
        height: 1
        y: Math.round(parent.height / 2)
        color: "#808080"
    }
    // The outline over the fill, so White's part stands out from a light page too.
    Rectangle {
        anchors.fill: parent
        radius: bar.radius
        color: "transparent"
        border.color: Theme.textSecondary
    }

    HoverHandler { id: hover }
    ToolTip.visible: hover.hovered
    ToolTip.text: "Evaluation " + bar.evalText
}
