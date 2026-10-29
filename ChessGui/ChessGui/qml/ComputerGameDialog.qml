import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus

// New game against the robot: the player's side and the level.
AppDialog {
    id: dialog

    property alias level: levelSlider.value   // the levels help can pick one

    signal levelsHelpRequested()

    // Open with the last game's choices; open() keeps the current ones (back from the levels help).
    function openNew() {
        level = ChessConnector.LastLevel
        side.checkedIndex = ChessConnector.PlayerPlaysWhite ? 0 : 1
        open()
    }

    readonly property var _info: ChessConnector.Levels[Math.round(levelSlider.value) - 1]

    title: "Play vs computer"
    width: 420

    onAccepted: ChessConnector.startComputerGame(Math.round(levelSlider.value), side.checkedIndex)

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.s2

        Label { text: "You play"; color: Theme.textSecondary; font: Theme.caption }
        ChoiceRow {
            id: side
            options: [{ label: "White" }, { label: "Black" }, { label: "Random" }]
        }

        Label {
            text: "Level"
            color: Theme.textSecondary; font: Theme.caption
            Layout.topMargin: Theme.s2
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.s1

            Label {
                text: dialog._info.level + " · " + dialog._info.name
                font: Theme.bodyStrong
            }
            Label {
                text: dialog._info.rating
                color: Theme.textSecondary
                Layout.fillWidth: true
            }
            IconButton {
                dense: true
                icon.source: Theme.icon("info")
                tip: "What do the levels mean?"
                onClicked: dialog.levelsHelpRequested()
            }
        }
        Slider {
            id: levelSlider
            from: 1; to: ChessConnector.Levels.length; stepSize: 1
            snapMode: Slider.SnapAlways
            Layout.fillWidth: true
            Accessible.name: "Computer level"
        }
        Label {
            text: dialog._info.hint
            font: Theme.caption
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
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
