import QtQuick
import QtQuick.Window

Rectangle {
    id: dialog

    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    property string _selectedAvatar: ""

    signal confirmed(string opponentAvatarName)  // empty = random
    signal closeRequested()

    width: 340
    height: content.implicitHeight + 48
    color:        isDarkMode ? "#252525" : "#f4f4f4"
    border.color: isDarkMode ? "#555555" : "#cccccc"
    border.width: 1
    radius: 12

    Column {
        id: content
        anchors.centerIn: parent
        width: parent.width - 40
        spacing: 16

        Text {
            text: "Select Second Player"
            font.pixelSize: 20
            font.bold: true
            color: isDarkMode ? "#ffffff" : "#000000"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Flow {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width

            Repeater {
                model: userManager.users
                delegate: Rectangle {
                    // hide the currently logged-in user
                    visible: modelData.id !== userManager.activeUserId
                    width:  visible ? 80  : 0
                    height: visible ? 100 : 0
                    radius: 8
                    color: dialog._selectedAvatar === modelData.avatarName
                        ? (isDarkMode ? "#1a4a7a" : "#cce4ff")
                        : (isDarkMode ? "#2d2d2d" : "#ffffff")
                    border.color: dialog._selectedAvatar === modelData.avatarName
                        ? "#0078d4"
                        : (isDarkMode ? "#555555" : "#dddddd")
                    border.width: 2

                    Column {
                        anchors.centerIn: parent
                        spacing: 6

                        Image {
                            width: 52; height: 52
                            source: modelData.avatarUrl
                            fillMode: Image.PreserveAspectFit
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Text {
                            text: modelData.name
                            color: isDarkMode ? "#ffffff" : "#000000"
                            font.pixelSize: 11
                            width: 72
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: dialog._selectedAvatar = modelData.avatarName
                    }
                }
            }
        }

        Row {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: "OK"
                implicitWidth: 90
                implicitHeight: 32
                font.bold: true
                enabled: dialog._selectedAvatar !== ""
                background: Rectangle {
                    radius: 4
                    color: parent.enabled
                        ? (parent.down ? "#005a9e" : "#0078d4")
                        : (isDarkMode ? "#2a2a2a" : "#cccccc")
                    border.color: parent.enabled ? "#005a9e" : "transparent"
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "#ffffff" : (isDarkMode ? "#666666" : "#aaaaaa")
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: dialog.confirmed(dialog._selectedAvatar)
            }

            Button {
                text: "Random"
                implicitWidth: 90
                implicitHeight: 32
                onClicked: dialog.confirmed("")
            }

            Button {
                text: "Cancel"
                implicitWidth: 80
                implicitHeight: 32
                onClicked: dialog.closeRequested()
            }
        }
    }
}
