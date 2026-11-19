import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus
import "games.js" as Games

// New game against the robot: the player's side, the level, the variant (LI-01A) and the clock. Play from
// here (BL-19): openFrom() starts it from a position of a kept game instead (ChessConnector.PlayFromHere),
// whose start it keeps (no Variant row).
AppDialog {
    id: dialog

    property alias level: levelSlider.value   // the levels help can pick one
    property var from: ({})                   // Play from here: ChessConnector.PlayFromHere; {} = a new game
    readonly property bool _fromHere: from.gameId !== undefined

    signal levelsHelpRequested()

    // Open with the last game's choices; open() keeps the current ones (back from the levels help).
    // chess960: the variant to offer (the result card's New game: the game's own); else the last one chosen.
    function openNew(chess960) {
        from = ({})
        level = ChessConnector.LastLevel
        // The side chosen here last time, not the last game's: a rematch may have swapped that (#193).
        side.checkedIndex = AppSettings.computerSide >= 0 ? AppSettings.computerSide : ChessConnector.PlayerPlaysWhite ? 0 : 1
        variant.choose(chess960 ?? AppSettings.chess960)
        clock.restore()
        open()
    }
    // From the position `position` (PlayFromHere): the player takes the side to move.
    function openFrom(position) {
        from = position
        level = ChessConnector.LastLevel
        side.checkedIndex = position.whiteToMove ? 0 : 1
        clock.restore()
        open()
    }

    readonly property var _info: ChessConnector.Levels[Math.round(levelSlider.value) - 1]

    title: _fromHere ? "Play from here vs computer" : "Play vs computer"
    width: 420

    onAccepted: {
        clock.remember()
        if (_fromHere) {
            ChessConnector.startComputerGameFrom(from.gameId, from.ply, Math.round(levelSlider.value), side.checkedIndex, clock.limitSec, clock.incSec)
        } else {
            variant.remember()
            AppSettings.computerSide = side.checkedIndex   // not from Play from here: the side to move chose it
            ChessConnector.startComputerGame(Math.round(levelSlider.value), side.checkedIndex, clock.limitSec, clock.incSec,
                                             variant.startPosition)
        }
    }

    ColumnLayout {
        anchors.fill: parent
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
            objectName: "computerSide"
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

        VariantPicker {
            id: variant
            objectName: "computerVariant"
            visible: !dialog._fromHere
            rateable: true
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

    footer: DialogButtonBox {
        defaultButton: primaryButton // highlighted (the box sets it, Qt 6.11+)
        Button {
            id: primaryButton
            objectName: "startComputerGameButton"
            text: "Start game"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
