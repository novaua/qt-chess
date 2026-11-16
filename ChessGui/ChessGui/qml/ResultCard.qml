import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

// The end of a game, over the board: who won and how (with the players' names), then Rematch,
// New game and Review moves.
Rectangle {
    id: card

    property string resultText
    property bool   canRematch: false
    property bool   canAnalyse: false   // BL-19 step 7: a kept game, the engine there

    signal analyseRequested()

    signal rematchRequested()
    signal newGameRequested()
    signal reviewRequested()
    signal closeRequested()

    implicitHeight: content.implicitHeight + 2 * Theme.s5
    radius: Theme.radiusCard
    color: Theme.cardBg
    border.color: Theme.cardStroke

    Accessible.role: Accessible.Dialog
    Accessible.name: resultText

    MouseArea { anchors.fill: parent }   // keep clicks off the board

    IconButton {
        dense: true
        anchors { top: parent.top; right: parent.right; margins: Theme.s1 }
        icon.source: Theme.icon("close")
        tip: "Close"
        onClicked: card.closeRequested()
    }

    ColumnLayout {
        id: content
        anchors {
            left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter
            leftMargin: Theme.s5; rightMargin: Theme.s5
        }
        spacing: Theme.s4

        IconImage {
            Layout.alignment: Qt.AlignHCenter
            source: Theme.icon("trophy")
            sourceSize.width: 32; sourceSize.height: 32
            color: Theme.accent
        }
        Label {
            Layout.fillWidth: true
            text: card.resultText
            font: Theme.subtitle
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        // A row when there's room, else stacked.
        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: columns === 1
            // Three in a row; four in two rows of two.
            readonly property int buttons: (card.canRematch ? 1 : 0) + 2 + (card.canAnalyse ? 1 : 0)
            columns: card.width >= 400 ? (buttons === 4 ? 2 : buttons) : 1
            columnSpacing: Theme.s2
            rowSpacing: Theme.s2

            Button {
                objectName: "rematchButton"
                visible: card.canRematch
                text: "Rematch"
                icon.source: Theme.icon("rematch")
                highlighted: true
                Layout.fillWidth: true
                onClicked: card.rematchRequested()
            }
            Button {
                objectName: "newGameButton"
                text: "New game"
                icon.source: Theme.icon("play")
                highlighted: !card.canRematch
                Layout.fillWidth: true
                onClicked: card.newGameRequested()
            }
            Button {
                objectName: "reviewButton"
                text: "Review moves"
                icon.source: Theme.icon("first")
                Layout.fillWidth: true
                onClicked: card.reviewRequested()
            }
            Button {
                objectName: "analyseResultButton"
                visible: card.canAnalyse
                text: "Analyse"
                icon.source: Theme.icon("analyse")
                Layout.fillWidth: true
                onClicked: card.analyseRequested()
            }
        }
    }
}
