import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Window

Rectangle {
    id: dialog

    signal closeRequested()

    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    width: 440
    height: content.implicitHeight + 48
    color:        isDarkMode ? "#252525" : "#f4f4f4"
    border.color: isDarkMode ? "#555555" : "#cccccc"
    border.width: 1
    radius: 12

    function focusInput() { movesField.forceActiveFocus() }

    Column {
        id: content
        anchors.centerIn: parent
        spacing: 14
        width: parent.width - 48

        Text {
            text: "Enter Moves"
            font.pixelSize: 18
            font.bold: true
            color: isDarkMode ? "#ffffff" : "#000000"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Controls.TextField {
            id: movesField
            width: parent.width
            placeholderText: "e2e4 d7d5 g1f3 ..."
            color: isDarkMode ? "#ffffff" : "#000000"
            background: Rectangle {
                color: isDarkMode ? "#2d2d2d" : "#ffffff"
                border.color: isDarkMode ? "#555555" : "#bbbbbb"
                radius: 4
            }
            Keys.onReturnPressed: dialog.apply()
            Keys.onEscapePressed: dialog.cancel()
        }

        Row {
            spacing: 16
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: "Apply"
                font.bold: true
                implicitHeight: 32
                implicitWidth: 90
                onClicked: dialog.apply()
            }
            Button {
                text: "Cancel"
                implicitHeight: 32
                implicitWidth: 80
                onClicked: dialog.cancel()
            }
        }
    }

    function apply() {
        chessConnector.applyMoves(movesField.text)
        movesField.text = ""
        dialog.closeRequested()
    }

    function cancel() {
        movesField.text = ""
        dialog.closeRequested()
    }
}
