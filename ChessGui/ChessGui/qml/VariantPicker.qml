import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus

// The setup dialogs' Variant row (LI-01A): Standard or Chess960, a random one of the 960 start positions
// (Rematch keeps it, New game draws another). remember() keeps the choice for the next new game.
// online: the Online page's challenge (LI-01A stage 2): Lichess picks the position; remembered on its own.
ColumnLayout {
    id: picker

    readonly property bool chess960: choice.checkedIndex === 1
    property bool rateable: false   // vs the robot: says a Chess960 game isn't rated
    property bool online: false     // a Lichess challenge
    property bool quick: false      // a Lichess seek (LI-09): remembered apart from the challenge's
    // ChessConnector.startComputerGame / startLocalGame's last argument.
    readonly property int startPosition: chess960 ? ChessConnector.RandomChess960 : ChessConnector.StandardGame

    function choose(on) { choice.checkedIndex = on ? 1 : 0 }
    function remember() {
        if (quick)
            AppSettings.quickChess960 = chess960
        else if (online)
            AppSettings.onlineChess960 = chess960
        else
            AppSettings.chess960 = chess960
    }

    spacing: Theme.s2

    Label { text: "Variant"; color: Theme.textSecondary; font: Theme.caption }
    ChoiceRow {
        id: choice
        // Its buttons: variantChoice_0, _1 (the UI tests, in the picker's dialog); quickVariantChoice_ on
        // the Online page, which has two pickers.
        objectName: picker.quick ? "quickVariantChoice" : "variantChoice"
        options: [{ label: "Standard" }, { label: "Chess960" }]
    }
    Label {
        visible: picker.chess960
        text: "The pieces behind the pawns are shuffled: one of 960 start positions, "
              + (picker.online || picker.quick ? "picked by Lichess. " : "at random. ")
              + "To castle, move the king onto its rook." + (picker.rateable ? " Not rated." : "")
        font: Theme.caption
        color: Theme.textSecondary
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
