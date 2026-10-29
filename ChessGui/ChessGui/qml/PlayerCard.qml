import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A player above or below the board: avatar, name, a detail (the robot's level), the pieces they
// have taken with "+N", and whose turn it is (an accent bar and a pulse on the avatar).
Rectangle {
    id: card

    property string avatarUrl
    property string avatarTip: ""   // tooltip; empty = the avatar is not clickable
    property string name
    property string detail
    property bool   isActive:  false
    property var    pieces:    []
    property int    advantage: 0

    signal avatarClicked()

    implicitHeight: Theme.playerCardHeight
    radius: Theme.radiusCard
    color: Theme.cardBg
    border.color: Theme.cardStroke

    Accessible.role: Accessible.Pane
    Accessible.name: name + (isActive ? ", to move" : "")

    // The side to move: an accent bar on the left edge.
    Rectangle {
        visible: card.isActive
        width: 3
        radius: 1.5
        color: Theme.accent
        anchors {
            left: parent.left; leftMargin: 3
            top: parent.top; topMargin: Theme.s3
            bottom: parent.bottom; bottomMargin: Theme.s3
        }
    }

    RowLayout {
        anchors {
            fill: parent
            leftMargin: Theme.s3; rightMargin: Theme.s3
        }
        spacing: Theme.s3

        Rectangle {
            id: avatarFrame
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            radius: Theme.radiusAvatar
            color: Theme.layerBg
            border.color: card.isActive ? Theme.accent : Theme.cardStroke
            border.width: card.isActive ? 2 : 1

            Image {
                anchors.fill: parent
                anchors.margins: 3
                fillMode: Image.PreserveAspectFit
                source: card.avatarUrl
                sourceSize: Qt.size(80, 80)
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                radius: parent.radius - 1
                color: Theme.accent
                opacity: 0
                visible: card.isActive

                SequentialAnimation on opacity {
                    running: card.isActive && !Theme.reduceMotion
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.2; duration: 850; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 0.0; duration: 850; easing.type: Easing.InOutSine }
                }
            }

            MouseArea {
                anchors.fill: parent
                enabled: card.avatarTip !== ""
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: card.avatarClicked()
                ToolTip.visible: containsMouse
                ToolTip.text: card.avatarTip
                ToolTip.delay: 300
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.s2

                Label {
                    text: card.name
                    font: Theme.bodyStrong
                    elide: Text.ElideRight
                    Layout.fillWidth: !detailLabel.visible
                }
                Label {
                    id: detailLabel
                    text: card.detail
                    visible: text !== ""
                    font: Theme.caption
                    color: Theme.textSecondary
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            CapturedStrip {
                pieces: card.pieces
                advantage: card.advantage
                pieceSize: 18
                Layout.preferredHeight: 18
            }
        }
    }
}
