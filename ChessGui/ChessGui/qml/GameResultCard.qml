import QtQuick
import QtQuick.Window

// Result of a finished game, with Rematch, shown on the board itself when the side panels
// (GameResultDialog / MoveHistoryPanel) are hidden, e.g. in the default portrait window.
Rectangle {
    id: card

    property string resultText: ""
    property bool   canRematch: false

    signal rematchRequested()
    signal closeRequested()

    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    height: resultCol.implicitHeight + 28
    radius: 12
    color:        isDarkMode ? "#e6252525" : "#e6f4f4f4"
    border.color: isDarkMode ? "#555555" : "#cccccc"

    MouseArea { anchors.fill: parent } // keep clicks off the board

    Column {
        id: resultCol
        anchors.centerIn: parent
        width: parent.width - 28
        spacing: 12

        Text {
            width: parent.width
            text: card.resultText
            font.pixelSize: 18
            font.bold: true
            color: card.isDarkMode ? "#ffffff" : "#000000"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        Row {
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter

            ActionButton {
                label: "Rematch"
                visible: card.canRematch
                onAction: card.rematchRequested()
            }
            ActionButton {
                label: "Close"
                onAction: card.closeRequested()
            }
        }
    }
}
