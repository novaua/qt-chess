import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import ChessPlusPlus

// New game for two players on this computer: the player's side and the opponent, another
// profile or a Guest.
AppDialog {
    id: dialog

    property string _opponentId: ""   // a profile's id; "" = Guest

    // Guest first, then every profile except the player's own.
    readonly property var _opponents: [{ id: "", name: "Guest", avatarUrl: "" }].concat(
        UserManager.users.filter(u => u.id !== UserManager.activeUserId))

    title: "Two players"
    width: 420

    onOpened: {
        _opponentId = ""
        side.checkedIndex = ChessConnector.PlayerPlaysWhite ? 0 : 1
    }
    onAccepted: ChessConnector.startLocalGame(side.checkedIndex === 0, _opponentId)

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.s2

        Label { text: "You play"; color: Theme.textSecondary; font: Theme.caption }
        ChoiceRow {
            id: side
            options: [{ label: "White" }, { label: "Black" }]
        }

        Label {
            text: "Opponent"
            color: Theme.textSecondary; font: Theme.caption
            Layout.topMargin: Theme.s2
        }
        // One Tab stop, like radio buttons: ← → pick the opponent.
        Flow {
            id: opponents
            Layout.fillWidth: true
            spacing: Theme.s2
            activeFocusOnTab: true
            Accessible.role: Accessible.List
            Accessible.name: "Opponent"

            function step(delta) {
                var list = dialog._opponents
                var next = list.findIndex(o => o.id === dialog._opponentId) + delta
                if (next >= 0 && next < list.length)
                    dialog._opponentId = list[next].id
            }
            Keys.onLeftPressed:  step(-1)
            Keys.onRightPressed: step(1)

            Repeater {
                model: dialog._opponents
                delegate: Rectangle {
                    required property var modelData
                    readonly property bool isSelected: dialog._opponentId === modelData.id

                    width: 80; height: 100
                    radius: Theme.radiusCard
                    color: isSelected ? Theme.selectedBg
                         : cardMouse.containsMouse ? Theme.cardBgHover : Theme.cardBg
                    border.color: isSelected ? Theme.accent : Theme.cardStroke
                    border.width: isSelected ? 2 : 1

                    Accessible.role: Accessible.RadioButton
                    Accessible.name: modelData.name
                    Accessible.checked: isSelected

                    Column {
                        anchors.centerIn: parent
                        spacing: Theme.s1

                        Item {
                            width: 52; height: 52
                            anchors.horizontalCenter: parent.horizontalCenter
                            Image {
                                anchors.fill: parent
                                visible: modelData.avatarUrl !== ""
                                source: modelData.avatarUrl
                                fillMode: Image.PreserveAspectFit
                            }
                            IconImage {   // a Guest has no avatar
                                anchors.centerIn: parent
                                visible: modelData.avatarUrl === ""
                                source: Theme.icon("person")
                                sourceSize.width: 32; sourceSize.height: 32
                                color: Theme.textSecondary
                            }
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

                    FocusRing { visible: opponents.activeFocus && isSelected }

                    MouseArea {
                        id: cardMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: dialog._opponentId = modelData.id
                        onDoubleClicked: dialog.accept()
                    }
                }
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: "Start game"
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
