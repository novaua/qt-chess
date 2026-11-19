import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import ChessPlusPlus
import "games.js" as Games

// New game for two players on this computer: the player's side, the opponent (another profile or a
// Guest), the variant (LI-01A) and the clock. Play from here (BL-19): openFrom() starts it from a position
// of a kept game, whose start it keeps (no Variant row).
AppDialog {
    id: dialog

    property string _opponentId: ""   // a profile's id; "" = Guest
    property var from: ({})           // Play from here: ChessConnector.PlayFromHere; {} = a new game
    readonly property bool _fromHere: from.gameId !== undefined

    // A new game. chess960: the variant to offer (the result card's New game: the game's own); else the
    // last one chosen.
    function openNew(chess960) {
        from = ({})
        variant.choose(chess960 ?? AppSettings.chess960)
        open()
    }
    // From the position `position` (PlayFromHere): the player takes the side to move.
    function openFrom(position) {
        from = position
        open()
    }

    // Guest first, then every profile except the player's own.
    readonly property var _opponents: [{ id: "", name: "Guest", avatarUrl: "" }].concat(
        UserManager.users.filter(u => u.id !== UserManager.activeUserId))

    title: _fromHere ? "Play from here, two players" : "Two players"
    width: 420
    height: Math.min(implicitHeight, parent ? parent.height : implicitHeight)

    onOpened: {
        _opponentId = ""
        side.checkedIndex = _fromHere ? (from.whiteToMove ? 0 : 1) : ChessConnector.PlayerPlaysWhite ? 0 : 1
        clock.restore()
    }
    onClosed: from = ({})   // the next open() is a new game
    onAccepted: {
        clock.remember()
        if (_fromHere) {
            ChessConnector.startLocalGameFrom(from.gameId, from.ply, side.checkedIndex === 0, _opponentId, clock.limitSec, clock.incSec)
        } else {
            variant.remember()
            ChessConnector.startLocalGame(side.checkedIndex === 0, _opponentId, clock.limitSec, clock.incSec,
                                          variant.startPosition)
        }
    }

    // Many profiles: the opponents wrap and the dialog would outgrow a small window, so this scrolls.
    ScrollView {
        id: body
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: body.availableWidth
            spacing: Theme.s2

            Label {
                objectName: "fromHereText"
                visible: dialog._fromHere
                text: dialog._fromHere ? Games.fromHereText(dialog.from) : ""
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.s2
            }
            Label { text: "You play"; color: Theme.textSecondary; font: Theme.caption }
            ChoiceRow {
                id: side
                objectName: "twoPlayerSide"
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

            VariantPicker {
                id: variant
                objectName: "playersVariant"
                visible: !dialog._fromHere
                Layout.fillWidth: true
                Layout.topMargin: Theme.s2
            }

            Label {
                text: "Clock"
                color: Theme.textSecondary; font: Theme.caption
                Layout.topMargin: Theme.s2
            }
            TimeControlPicker {
                id: clock
                Layout.fillWidth: true
            }
        }
    }

    footer: DialogButtonBox {
        defaultButton: primaryButton // highlighted (the box sets it, Qt 6.11+)
        Button {
            id: primaryButton
            objectName: "startGameButton"
            text: "Start game"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
