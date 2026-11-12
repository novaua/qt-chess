import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// An unfinished game as a card, like Lichess's "current games" (LI-04, BL-17): the position from the
// player's side, who, whose turn, a line of details and Continue. A click on the board continues too.
// On the Online page (the Lichess games) and in the Continue list (the robot's game and the Lichess
// ones). Its width comes from the layout (Layout.preferredWidth); the board fills it.
Rectangle {
    id: card

    property string fen
    property bool   flipped: false      // the player plays Black
    property string lastMove            // UCI
    property string title               // the opponent
    property bool   playerToMove: false // "Your turn", else `waitingText`
    property string waitingText: "Waiting"
    property string details
    property bool   playable: true      // Continue plays it here (highlighted on the player's turn); else the action opens it elsewhere
    property string actionText: "Continue"
    property string actionIcon: "play"
    property string actionTip: ""       // e.g. why it opens lichess.org
    property bool   busy: false         // something is starting: no second click
    property bool   starting: false     // this card's game is the one starting: "Starting…" (BL-16)

    signal activated()

    implicitHeight: column.implicitHeight + 2 * Theme.s3
    radius: Theme.radiusCard
    color: Theme.layerBg
    border.color: Theme.cardStroke
    Accessible.role: Accessible.ListItem
    Accessible.name: title + ", " + details + ", you play " + (flipped ? "Black" : "White")
                     + ", " + (playerToMove ? "your turn" : waitingText.toLowerCase())

    ColumnLayout {
        id: column
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: Theme.s3 }
        spacing: Theme.s2

        BoardPreview {
            fen: card.fen
            flipped: card.flipped
            lastMove: card.lastMove
            Layout.preferredWidth: card.width - 2 * Theme.s3
            Layout.preferredHeight: card.width - 2 * Theme.s3

            MouseArea {
                anchors.fill: parent
                enabled: !card.busy
                cursorShape: Qt.PointingHandCursor
                onClicked: card.activated()
            }
        }
        RowLayout {
            spacing: Theme.s2
            Layout.fillWidth: true
            Label {
                text: card.title
                font: Theme.bodyStrong
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Label {
                text: card.playerToMove ? "Your turn" : card.waitingText
                font: card.playerToMove ? Theme.bodyStrong : Theme.caption
                color: card.playerToMove ? Theme.accent : Theme.textSecondary
            }
        }
        Label {
            text: card.details
            font: Theme.caption
            color: Theme.textSecondary
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        BusyButton {
            idleText: card.actionText
            idleIcon: Theme.icon(card.actionIcon)
            busy: card.starting
            highlighted: card.playable && card.playerToMove
            enabled: !card.busy && !card.starting
            Layout.fillWidth: true
            ToolTip.visible: hovered && card.actionTip !== ""
            ToolTip.text: card.actionTip
            ToolTip.delay: 400
            onClicked: card.activated()
        }
    }
}
