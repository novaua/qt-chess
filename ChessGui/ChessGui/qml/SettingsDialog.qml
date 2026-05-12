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

    width: 300
    height: settingsCol.implicitHeight + 48
    color:        isDarkMode ? "#252525" : "#f4f4f4"
    border.color: isDarkMode ? "#555555" : "#cccccc"
    border.width: 1
    radius: 12

    Column {
        id: settingsCol
        anchors.centerIn: parent
        width: parent.width - 40
        spacing: 16

        Text {
            text: "Settings"
            font.pixelSize: 20
            font.bold: true
            color: isDarkMode ? "#ffffff" : "#000000"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Flickable {
            width: parent.width
            height: flickContent.implicitHeight
            contentHeight: flickContent.implicitHeight
            clip: true

            Column {
                id: flickContent
                width: parent.width
                spacing: 4

                Row {
                    width: parent.width
                    Text {
                        text: "Dark Mode"
                        color: isDarkMode ? "#dddddd" : "#222222"
                        font.pixelSize: 15
                        width: parent.width - darkSwitch.width
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Controls.Switch {
                        id: darkSwitch
                        checked: appSettings.darkMode
                        onToggled: appSettings.darkMode = checked
                    }
                }

                Row {
                    width: parent.width
                    Text {
                        text: "Music"
                        color: isDarkMode ? "#dddddd" : "#222222"
                        font.pixelSize: 15
                        width: parent.width - musicSwitch.width
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Controls.Switch {
                        id: musicSwitch
                        checked: appSettings.musicEnabled
                        onToggled: appSettings.musicEnabled = checked
                    }
                }
            }
        }

        Button {
            text: "Done"
            implicitWidth: 100
            implicitHeight: 32
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: dialog.closeRequested()
        }
    }
}
