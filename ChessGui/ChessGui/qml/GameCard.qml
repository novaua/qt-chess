import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A game as a card, like Lichess's "current games" (LI-04, BL-17): the position from the player's
// side, who, whose turn (or the result), a line of details and Continue (or Open). A click on the board
// does the same. On the Online page (the Lichess games), in the Continue list (the kept local games
// and the Lichess ones) and on the Games page (every kept game, BL-19). Its width comes from the layout
// (Layout.preferredWidth); the board fills it.
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
    property bool   deletable: false    // a Delete button beside the action (the Games page, BL-19)
    property bool   exportable: false   // an Export PGN button beside the action (the Games page)
    property bool   hasMenu: false      // the Menu key / Shift+F10 on its button asks for its menu (LI-10)

    signal activated()
    signal menuRequested()
    signal deleteRequested()
    signal exportRequested()

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
        RowLayout {
            spacing: Theme.s2
            Layout.fillWidth: true
            BusyButton {
                objectName: "actionButton"
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
                Keys.onPressed: (event) => {
                    if (card.hasMenu && (event.key === Qt.Key_Menu
                                         || (event.key === Qt.Key_F10 && (event.modifiers & Qt.ShiftModifier)))) {
                        card.menuRequested()
                        event.accepted = true
                    }
                }
            }
            IconButton {
                objectName: "exportButton"
                visible: card.exportable
                icon.source: Theme.icon("export")
                tip: "Export PGN"
                enabled: !card.busy
                onClicked: card.exportRequested()
            }
            IconButton {
                objectName: "deleteButton"
                visible: card.deletable
                icon.source: Theme.icon("delete")
                tip: "Delete"
                enabled: !card.busy
                onClicked: card.deleteRequested()
            }
        }
    }
}
