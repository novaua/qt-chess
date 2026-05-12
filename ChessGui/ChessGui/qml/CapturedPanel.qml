import QtQuick
import QtQuick.Window

Rectangle {
    id: panel
    color: "transparent"

    property string avatarUrl: ""
    property var    pieces:   []

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
            width:  panel.height - 8
            height: panel.height - 8
            radius: 6
            color:  panel._dark ? "#3c3c3c" : "#e8e8e8"
            border.color: panel._dark ? "#666666" : "#bbbbbb"
            border.width: 1

            Image {
                id: avatar
                anchors.fill: parent
                anchors.margins: 3
                fillMode: Image.PreserveAspectFit
                source: panel.avatarUrl
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
