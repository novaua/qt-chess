import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus

// The move list with its tools: Copy / Export PGN, the review buttons and the game status. Beside
// the board in a wide window, inside the move sheet in a narrow one.
ColumnLayout {
    id: panel

    property string status        // e.g. "Bob to move", the result
    property string reviewText    // "Viewing 12. Nf3" while reviewing: shown with Back to live
    property bool   showTitle: true
    property bool   resultActions: false   // the game is over and its result card closed
    property bool   canRematch: false

    signal exportRequested()
    signal rematchRequested()
    signal newGameRequested()

    spacing: 0

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.s3
        Layout.rightMargin: Theme.s1
        Layout.preferredHeight: 40
        spacing: 0

        Label {
            text: panel.showTitle ? "Moves" : ""
            font: Theme.bodyStrong
            Layout.fillWidth: true
        }
        IconButton {
            dense: true
            icon.source: Theme.icon("copy")
            tip: "Copy PGN"
            enabled: ChessConnector.MoveHistory.length > 0
            onClicked: ChessConnector.copyPgn()
        }
        IconButton {
            dense: true
            icon.source: Theme.icon("export")
            tip: "Export PGN…"
            enabled: ChessConnector.MoveHistory.length > 0
            onClicked: panel.exportRequested()
        }
    }

    MoveList {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: Theme.s1
        Layout.rightMargin: Theme.s1
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: Theme.divider
    }

    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Theme.s1
        spacing: Theme.s1

        Repeater {
            model: [
                { icon: "first", tip: "First move (Home)", back: true  },
                { icon: "prev",  tip: "Previous move (←)", back: true  },
                { icon: "next",  tip: "Next move (→)",     back: false },
                { icon: "last",  tip: "Last move (End)",   back: false },
            ]
            IconButton {
                required property var modelData
                required property int index
                dense: true
                icon.source: Theme.icon(modelData.icon)
                tip: modelData.tip
                enabled: modelData.back ? ChessConnector.CanReviewPrev : ChessConnector.CanReviewNext
                onClicked: {
                    if      (index === 0) ChessConnector.reviewFirst()
                    else if (index === 1) ChessConnector.reviewPrev()
                    else if (index === 2) ChessConnector.reviewNext()
                    else                  ChessConnector.reviewLast()
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        Layout.margins: Theme.s3
        Layout.topMargin: Theme.s1
        visible: panel.status !== "" && panel.reviewText === ""
        text: panel.status
        font: Theme.body
        color: Theme.textSecondary
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
    }

    // What the closed result card offered.
    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.bottomMargin: Theme.s3
        visible: panel.resultActions && panel.reviewText === ""
        spacing: Theme.s2

        Button {
            visible: panel.canRematch
            text: "Rematch"
            icon.source: Theme.icon("rematch")
            highlighted: true
            onClicked: panel.rematchRequested()
        }
        Button {
            text: "New game"
            icon.source: Theme.icon("play")
            highlighted: !panel.canRematch
            onClicked: panel.newGameRequested()
        }
    }

    // Reviewing: a slim InfoBar with the way back.
    InfoBar {
        Layout.fillWidth: true
        Layout.margins: Theme.s2
        Layout.topMargin: Theme.s1
        visible: panel.reviewText !== ""
        closable: false
        text: panel.reviewText
        Button {
            text: "Back to live"
            onClicked: ChessConnector.reviewLast()
        }
    }
}
