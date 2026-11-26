import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus

// The move list with its tools: Copy / Export PGN, the review buttons and the game status. Beside
// the board in a wide window, inside the move sheet in a narrow one.
ColumnLayout {
    id: panel

    property string status        // e.g. "Bob to move", the result
    // The review bar: "Viewing 12. Nf3" while reviewing (with Back to live: canGoLive), a replay's
    // "Move 5 of 40"; with Play from here when the position shown can be played from (BL-19 step 4b).
    property string reviewText
    property bool   canGoLive: reviewText !== ""
    property bool   canPlayFromHere: false
    property bool   showTitle: true
    property bool   resultActions: false   // the game is over and its result card closed
    property bool   canRematch: false
    property bool   canSeekAgain: false   // LI-16: New opponent in New game's place
    property bool   seeking: false        // its seek waits

    signal exportRequested()
    signal rematchRequested()
    signal newGameRequested()
    signal newOpponentRequested()
    signal playFromHereClicked(Item button)   // opens the vs computer / two players menu under it
    signal analyseRequested()

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
            enabled: ChessConnector.PlyCount > 0
            onClicked: ChessConnector.copyPgn()
        }
        IconButton {
            dense: true
            icon.source: Theme.icon("export")
            tip: "Export PGN…"
            enabled: ChessConnector.PlyCount > 0
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

    // The engine's analysis of the game (BL-19 step 7): Analyse game, its progress, then the summary.
    AnalysisSummary {
        onAnalyseRequested: panel.analyseRequested()
        Layout.fillWidth: true
        Layout.leftMargin: Theme.s3
        Layout.rightMargin: Theme.s3
        Layout.topMargin: Theme.s2
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
                objectName: "review_" + modelData.icon   // review_first … review_last (the UI tests)
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
            visible: !panel.canSeekAgain
            text: "New game"
            icon.source: Theme.icon("play")
            highlighted: !panel.canRematch
            onClicked: panel.newGameRequested()
        }
        BusyButton {
            objectName: "panelNewOpponentButton"
            visible: panel.canSeekAgain
            idleText: "New opponent"
            idleIcon: Theme.icon("play")
            busy: panel.seeking
            busyText: "Looking for an opponent…"
            enabled: !panel.seeking
            highlighted: !panel.canRematch
            onClicked: panel.newOpponentRequested()
        }
    }

    // Reviewing: a slim InfoBar with the way back, and Play from here.
    InfoBar {
        objectName: "reviewBar"
        Layout.fillWidth: true
        Layout.margins: Theme.s2
        Layout.topMargin: Theme.s1
        visible: panel.reviewText !== ""
        closable: false
        actionsBelow: panel.canPlayFromHere   // two buttons don't fit beside the text
        text: panel.reviewText
        Button {
            id: fromHereButton
            objectName: "playFromHereButton"
            visible: panel.canPlayFromHere
            text: "Play from here"
            icon.source: Theme.icon("play")
            onClicked: panel.playFromHereClicked(fromHereButton)
        }
        Button {
            visible: panel.canGoLive
            text: "Back to live"
            onClicked: ChessConnector.reviewLast()
        }
    }
}
