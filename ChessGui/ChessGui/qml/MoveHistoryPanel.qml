import QtQuick
import ChessPlusPlus
import QtQuick.Controls

Rectangle {
    id: panel

    property bool rematchPending: false // a rematch challenge is waiting for the opponent

    color: Theme.layerBg
    border.color: Theme.cardStroke
    border.width: 1
    radius: Theme.radiusCard

    Text {
        id: header
        anchors { top: parent.top; topMargin: Theme.s2; horizontalCenter: parent.horizontalCenter }
        text: "Moves"
        font: Theme.bodyStrong
        color: Theme.textPrimary
    }

    Timer {
        id: blinkTimer
        interval: 1000; repeat: true
        running: ChessConnector.ReviewMode
        property bool blinkOn: true
        onTriggered: blinkOn = !blinkOn
        onRunningChanged: if (!running) blinkOn = true
    }

    Row {
        id: navButtons
        anchors {
            top: header.bottom; topMargin: Theme.s1
            horizontalCenter: parent.horizontalCenter
        }

        Repeater {
            model: ["first", "prev", "next", "last"]
            delegate: IconButton { dense: true
                required property string modelData
                required property int index

                icon.source: Theme.icon(modelData)
                tip: ["First move", "Previous move", "Next move", "Last move"][index]
                enabled: index < 2 ? ChessConnector.CanReviewPrev : ChessConnector.CanReviewNext
                // While reviewing, "Last move" blinks to show the way back to the game.
                opacity: index === 3 && ChessConnector.ReviewMode
                         && !blinkTimer.blinkOn ? 0.25 : 1.0
                onClicked: {
                    if      (index === 0) ChessConnector.reviewFirst()
                    else if (index === 1) ChessConnector.reviewPrev()
                    else if (index === 2) ChessConnector.reviewNext()
                    else                  ChessConnector.reviewLast()
                }
            }
        }
    }

    Text {
        id: reviewIndicator
        visible: ChessConnector.ReviewMode
        anchors {
            top: navButtons.bottom
            horizontalCenter: parent.horizontalCenter
        }
        text: "Reviewing"
        font: Theme.caption
        color: Theme.textSecondary
        height: visible ? implicitHeight : 0
    }


    ListView {
        id: listView
        anchors {
            top: reviewIndicator.bottom; topMargin: Theme.s1
            left: parent.left; leftMargin: Theme.s1
            right: parent.right; rightMargin: Theme.s1
            bottom: rematchButton.visible ? rematchButton.top
                  : (resultBar.visible    ? resultBar.top : parent.bottom)
            bottomMargin: Theme.s1
        }
        clip: true
        model: ChessConnector.MoveHistory
        onCountChanged: Qt.callLater(function() { listView.positionViewAtEnd() })

        Connections {
            target: ChessConnector
            function onReviewStateChanged() {
                var ri = ChessConnector.ReviewIndex
                if (ri > 0)
                    listView.positionViewAtIndex((ri - 1) >> 1, ListView.Contain)
                else if (!ChessConnector.ReviewMode)
                    Qt.callLater(function() { listView.positionViewAtEnd() })
            }
        }

        delegate: Rectangle {
            required property var modelData
            required property int index
            width: listView.width; height: 26
            color: "transparent"

            readonly property int ri: ChessConnector.ReviewIndex
            readonly property int reviewRow: ri > 0 ? Math.floor((ri - 1) / 2) : -1
            readonly property bool hlW: reviewRow === index && ri % 2 === 1
            readonly property bool hlB: reviewRow === index && ri % 2 === 0

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1
                color: Theme.divider
            }

            Rectangle {
                visible: hlW
                x: whiteMove.x; y: 2; width: whiteMove.width; height: parent.height - 4
                color: Theme.selectedBg
                radius: 4
            }
            Rectangle {
                visible: hlB
                x: blackMove.x; y: 2; width: blackMove.width; height: parent.height - 4
                color: Theme.selectedBg
                radius: 4
            }

            Row {
                anchors.fill: parent; anchors.leftMargin: Theme.s1; anchors.rightMargin: Theme.s1
                Text {
                    width: 30; height: parent.height
                    text: modelData.n + "."
                    font: Theme.caption
                    color: Theme.textSecondary
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    id: whiteMove
                    width: (parent.width - 30) / 2; height: parent.height
                    leftPadding: Theme.s1
                    text: modelData.w
                    font.pixelSize: Theme.body.pixelSize; font.features: { "tnum": 1 }
                    color: Theme.textPrimary
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    id: blackMove
                    width: (parent.width - 30) / 2; height: parent.height
                    leftPadding: Theme.s1
                    text: modelData.b
                    font.pixelSize: Theme.body.pixelSize; font.features: { "tnum": 1 }
                    color: Theme.textPrimary
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    // Rematch — shown when the game is over. For a Lichess game it sends the opponent a new
    // challenge (ChessConnector::rematch()).
    Button {
        id: rematchButton
        visible: ChessConnector.GameResult !== "" && ChessConnector.CanRematch && !panel.rematchPending
        anchors {
            bottom: resultBar.visible ? resultBar.top : parent.bottom
            left: parent.left; leftMargin: Theme.s1
            right: parent.right; rightMargin: Theme.s1
        }
        text: "Rematch"
        icon.source: Theme.icon("rematch")
        highlighted: true
        onClicked: ChessConnector.rematch()
    }

    Text {
        id: resultBar
        visible: ChessConnector.GameResult !== ""
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        padding: Theme.s2
        text: ChessConnector.ResultText
        font: Theme.bodyStrong
        color: Theme.textPrimary
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
    }
}
