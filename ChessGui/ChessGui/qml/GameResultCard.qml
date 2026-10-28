import QtQuick
import QtQuick.Controls

// Result of a finished game, with Rematch, shown on the board itself when the side panels
// (GameResultDialog / MoveHistoryPanel) are hidden, e.g. in the default portrait window.
Rectangle {
    id: card

    property string resultText: ""
    property bool   canRematch: false

    signal rematchRequested()
    signal closeRequested()

    height: resultCol.implicitHeight + 2 * Theme.s4
    radius: Theme.radiusCard
    color:        Theme.cardBg
    border.color: Theme.cardStroke

    MouseArea { anchors.fill: parent } // keep clicks off the board

    Column {
        id: resultCol
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.s4
        spacing: Theme.s3

        Text {
            width: parent.width
            text: card.resultText
            font: Theme.subtitle
            color: Theme.textPrimary
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        Row {
            spacing: Theme.s2
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: "Rematch"
                icon.source: Theme.icon("rematch")
                highlighted: true
                visible: card.canRematch
                onClicked: card.rematchRequested()
            }
            Button {
                text: "Close"
                onClicked: card.closeRequested()
            }
        }
    }
}
