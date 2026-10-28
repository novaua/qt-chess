import QtQuick
import QtQuick.Controls

Rectangle {
    id: panel
    color: "transparent"

    property string avatarUrl: ""
    property string avatarTip: ""      // hover tooltip; empty = none, and the avatar is not clickable
    property var    pieces:    []
    property bool   isActive:  false

    signal avatarClicked()

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
            radius: Theme.radiusAvatar
            color:  Theme.cardBg
            border.color: panel.isActive ? Theme.success : Theme.cardStroke
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
                color: Theme.success
                opacity: 0
                visible: panel.isActive

                SequentialAnimation on opacity {
                    running: panel.isActive
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.22; duration: 850; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 0.0;  duration: 850; easing.type: Easing.InOutSine }
                }
            }

            MouseArea {
                id: avatarMouse
                anchors.fill: parent
                enabled: panel.avatarTip !== ""
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: panel.avatarClicked()
                ToolTip.visible: containsMouse
                ToolTip.text: panel.avatarTip
                ToolTip.delay: 300
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
