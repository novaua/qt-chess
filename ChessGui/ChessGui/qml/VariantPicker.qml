import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The setup dialogs' Variant row (LI-01A): Standard or Chess960, a random one of the 960 start positions
// (Rematch keeps it, New game draws another). remember() keeps the choice for the next new game.
ColumnLayout {
    id: picker

    readonly property bool chess960: choice.checkedIndex === 1
    property bool rateable: false   // vs the robot: says a Chess960 game isn't rated
    // ChessConnector.startComputerGame / startLocalGame's last argument: RandomChess960 (-1) or StandardGame (-2).
    readonly property int startPosition: chess960 ? -1 : -2

    function choose(on) { choice.checkedIndex = on ? 1 : 0 }
    function remember() { AppSettings.chess960 = chess960 }

    spacing: Theme.s2

    Label { text: "Variant"; color: Theme.textSecondary; font: Theme.caption }
    ChoiceRow {
        id: choice
        objectName: "variantChoice"   // its buttons: variantChoice_0, _1 (the UI tests, in the picker's dialog)
        options: [{ label: "Standard" }, { label: "Chess960" }]
    }
    Label {
        visible: picker.chess960
        text: "The pieces behind the pawns are shuffled: one of 960 start positions, at random. "
              + "To castle, move the king onto its rook." + (picker.rateable ? " Not rated." : "")
        font: Theme.caption
        color: Theme.textSecondary
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
