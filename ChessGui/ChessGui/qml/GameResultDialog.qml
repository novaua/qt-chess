import QtQuick
import QtQuick.Window

Rectangle {
    id: dialog

    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    property string winner: ""
    signal okClicked()

    width: 340
    height: dialogContent.implicitHeight + 48
    color: isDarkMode ? "#cc252525" : "#ccf4f4f4"
    border.color: isDarkMode ? "#555555" : "#cccccc"
    border.width: 1
    radius: 12

    Column {
        id: dialogContent
        anchors.centerIn: parent
        spacing: 14
        width: parent.width - 48

        Text {
            text: dialog.winner
            font.pixelSize: 26
            font.bold: true
            color: isDarkMode ? "#ffffff" : "#000000"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Rectangle {
            width: parent.width
            height: 1
            color: isDarkMode ? "#444444" : "#dddddd"
        }

        Text {
            text: "Games played: " + userManager.gamesPlayed
            color: isDarkMode ? "#dddddd" : "#222222"
            font.pixelSize: 16
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "Human won: " + userManager.humanWins
            color: isDarkMode ? "#dddddd" : "#222222"
            font.pixelSize: 16
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "Robot won: " + userManager.computerWins
            color: isDarkMode ? "#dddddd" : "#222222"
            font.pixelSize: 16
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "Since: " + userManager.statsCreatedDate
            color: isDarkMode ? "#888888" : "#888888"
            font.pixelSize: 13
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Button {
            text: "Ok"
            implicitHeight: 36
            implicitWidth: 110
            anchors.horizontalCenter: parent.horizontalCenter
            font.bold: true
            onClicked: dialog.okClicked()
        }
    }
}
