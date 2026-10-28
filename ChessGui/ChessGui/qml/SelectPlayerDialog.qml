import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AppDialog {
    id: dialog

    property string _selectedAvatar: ""

    signal confirmed(string opponentAvatarName)  // empty = random

    title: "Choose the second player"
    width: 380
    acceptEnabled: _selectedAvatar !== ""

    onOpened: _selectedAvatar = ""
    onAccepted: confirmed(_selectedAvatar)

    Flow {
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.s2

        Repeater {
            model: userManager.users
            delegate: Rectangle {
                readonly property bool isSelected: dialog._selectedAvatar === modelData.avatarName
                // hide the currently logged-in user
                visible: modelData.id !== userManager.activeUserId
                width:  visible ? 80  : 0
                height: visible ? 100 : 0
                radius: Theme.radiusCard
                color: isSelected ? Theme.selectedBg
                     : cardMouse.containsMouse ? Theme.cardBgHover : Theme.cardBg
                border.color: isSelected ? Theme.accent : Theme.cardStroke
                border.width: isSelected ? 2 : 1

                Column {
                    anchors.centerIn: parent
                    spacing: Theme.s1

                    Image {
                        width: 52; height: 52
                        source: modelData.avatarUrl
                        fillMode: Image.PreserveAspectFit
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: modelData.name
                        color: Theme.textPrimary
                        font: Theme.caption
                        width: 72
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                MouseArea {
                    id: cardMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dialog._selectedAvatar = modelData.avatarName
                    onDoubleClicked: dialog.accept()
                }
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: "Start game"
            highlighted: true
            enabled: dialog.acceptEnabled
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Guest"   // a second player without a profile, random avatar
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: { dialog.confirmed(""); dialog.close() }
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
