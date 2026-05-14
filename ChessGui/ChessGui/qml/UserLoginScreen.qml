import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Window

Rectangle {
    id: loginScreen
    anchors.fill: parent

    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    color: isDarkMode ? "#1e1e1e" : "#f0f0f0"

    Image {
        anchors.fill: parent
        source: "qrc:/app/pics/ChessBackground.jpg"
        fillMode: Image.PreserveAspectCrop
        opacity: 0.35
    }

    Column {
        anchors.centerIn: parent
        spacing: 24
        width: Math.min(parent.width - 48, 480)

        Text {
            text: "Chess ++"
            font.pixelSize: 30
            font.bold: true
            color: isDarkMode ? "#ffffff" : "#000000"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "Who's playing?"
            font.pixelSize: 16
            color: isDarkMode ? "#bbbbbb" : "#555555"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Flow {
            id: userFlow
            spacing: 14
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width

            Repeater {
                model: userManager.users
                delegate: Rectangle {
                    readonly property bool isActive: modelData.id === userManager.activeUserId

                    width: 90
                    height: 110
                    radius: 10
                    color: isActive
                        ? (isDarkMode ? "#1a4a7a" : "#cce4ff")
                        : (isDarkMode ? "#2d2d2d" : "#ffffff")
                    border.color: isActive ? "#0078d4" : (isDarkMode ? "#555555" : "#dddddd")
                    border.width: isActive ? 2 : 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 8

                        Image {
                            width: 56; height: 56
                            source: modelData.avatarUrl
                            fillMode: Image.PreserveAspectFit
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Text {
                            text: modelData.name
                            color: isDarkMode ? "#ffffff" : "#000000"
                            font.pixelSize: 12
                            font.bold: isActive
                            horizontalAlignment: Text.AlignHCenter
                            width: 80
                            elide: Text.ElideRight
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onEntered: if (!isActive) parent.border.color = "#0078d4"
                        onExited:  if (!isActive) parent.border.color = isDarkMode ? "#555555" : "#dddddd"
                        onClicked: userManager.login(modelData.id)
                    }
                }
            }
        }

        Button {
            text: "+ Add User"
            implicitWidth: 130
            implicitHeight: 32
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: loginScreen.addUserRequested()
        }
    }

    signal addUserRequested()
}
