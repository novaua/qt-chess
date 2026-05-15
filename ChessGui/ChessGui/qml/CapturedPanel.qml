import QtQuick
import QtQuick.Window

Rectangle {
    id: panel
    color: "transparent"

    property string avatarUrl: ""
    property var    pieces:    []
    property bool   isActive:  false

    readonly property bool _dark: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 6
        spacing: 4

        // Avatar framed like a button card
        Rectangle {
            id: avatarFrame
            width:  panel.height - 8
            height: panel.height - 8
            radius: 6
            color:  panel._dark ? "#3c3c3c" : "#e8e8e8"
            border.color: panel.isActive ? "#27ae60" : (panel._dark ? "#666666" : "#bbbbbb")
            border.width: panel.isActive ? 2 : 1

            Image {
                anchors.fill: parent
                anchors.margins: 3
                fillMode: Image.PreserveAspectFit
                source: panel.avatarUrl
            }

            // Pulsing green tint overlay when it's this player's turn
            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                radius: parent.radius - 1
                color: "#27ae60"
                opacity: 0
                visible: panel.isActive

                SequentialAnimation on opacity {
                    running: panel.isActive
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.22; duration: 850; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 0.0;  duration: 850; easing.type: Easing.InOutSine }
                }
            }
        }

        Repeater {
            model: panel.pieces
            Image {
                height: panel.height - 8
                width:  height
                fillMode: Image.PreserveAspectFit
                source: modelData === modelData.toLowerCase()
                    ? "qrc:/piece/pics/black/" + modelData.toUpperCase() + ".png"
                    : "qrc:/piece/pics/white/" + modelData.toLowerCase() + ".png"
            }
        }
    }
}
