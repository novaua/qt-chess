import QtQuick
import QtQuick.Window
import QtQuick.Controls

Rectangle {
    id: panel

    property bool gameInProgress: false
    property bool isOnlineGame: false

    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    color: isDarkMode ? "#1a1a1a" : "#f2f2f2"
    border.color: isDarkMode ? "#444" : "#ccc"
    border.width: 5
    radius: 6

    Rectangle {
        id: header
        width: parent.width; height: 24
        color: isDarkMode ? "#2d2d2d" : "#e0e0e0"
        radius: parent.radius
        Rectangle { width: parent.width; height: parent.radius; anchors.bottom: parent.bottom; color: parent.color }
        Text {
            anchors.centerIn: parent
            text: "Move History"
            font.pixelSize: 13; font.bold: true
            color: isDarkMode ? "#cccccc" : "#555555"
        }
    }

    Timer {
        id: blinkTimer
        interval: 1000; repeat: true
        running: chessConnector.ReviewMode && panel.gameInProgress
        property bool blinkOn: true
        onTriggered: blinkOn = !blinkOn
        onRunningChanged: if (!running) blinkOn = true
    }

    Row {
        id: navButtons
        anchors {
            top: header.bottom; topMargin: 3
            horizontalCenter: parent.horizontalCenter
        }
        spacing: 3

        Repeater {
            model: ["|<<", "<", ">", ">>|"]
            delegate: Rectangle {
                required property string modelData
                required property int index

                readonly property bool canDo: index < 2 ? chessConnector.CanReviewPrev
                                                        : chessConnector.CanReviewNext
                readonly property string tipText: [
                    "First move", "Previous move", "Next move", "Last move"][index]
                readonly property bool shouldBlink: index === 3
                                                 && chessConnector.ReviewMode
                                                 && panel.gameInProgress

                width: 30; height: 20
                radius: 3
                opacity: shouldBlink ? (blinkTimer.blinkOn ? 1.0 : 0.25) : 1.0
                color: canDo
                    ? (navMa.containsMouse
                        ? (isDarkMode ? "#555" : "#ccc")
                        : (isDarkMode ? "#333" : "#e8e8e8"))
                    : (isDarkMode ? "#222" : "#f5f5f5")
                border.color: isDarkMode ? "#555" : "#ccc"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: 12; font.family: "Helvetica"
                    color: canDo ? (isDarkMode ? "#ddd" : "#333")
                                 : (isDarkMode ? "#444" : "#bbb")
                }

                MouseArea {
                    id: navMa
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: canDo
                    onClicked: {
                        if      (index === 0) chessConnector.reviewFirst()
                        else if (index === 1) chessConnector.reviewPrev()
                        else if (index === 2) chessConnector.reviewNext()
                        else                  chessConnector.reviewLast()
                    }
                    ToolTip.visible: containsMouse
                    ToolTip.text: tipText
                    ToolTip.delay: 400
                }
            }
        }
    }

    Text {
        id: reviewIndicator
        visible: chessConnector.ReviewMode
        anchors {
            top: navButtons.bottom; topMargin: 2
            horizontalCenter: parent.horizontalCenter
        }
        text: "— reviewing —"
        font.pixelSize: 9; font.italic: true
        color: isDarkMode ? "#888" : "#aaa"
        height: visible ? implicitHeight : 0
    }


    ListView {
        id: listView
        anchors {
            top: reviewIndicator.bottom; topMargin: 2
            left: parent.left; leftMargin: 3
            right: parent.right; rightMargin: 3
            bottom: actionButtons.visible ? actionButtons.top
                  : (resultBar.visible ? resultBar.top : parent.bottom)
            bottomMargin: 3
        }
        clip: true
        model: chessConnector.MoveHistory
        spacing: 1
        onCountChanged: Qt.callLater(function() { listView.positionViewAtEnd() })

        Connections {
            target: chessConnector
            function onReviewStateChanged() {
                var ri = chessConnector.ReviewIndex
                if (ri > 0)
                    listView.positionViewAtIndex((ri - 1) >> 1, ListView.Contain)
                else if (!chessConnector.ReviewMode)
                    Qt.callLater(function() { listView.positionViewAtEnd() })
            }
        }

        delegate: Rectangle {
            required property var modelData
            required property int index
            width: listView.width; height: 26
            color: index % 2 === 0
                ? (isDarkMode ? "#252525" : "#f8f8f8")
                : (isDarkMode ? "#2d2d2d" : "#eeeeee")
            radius: 2

            readonly property int ri: chessConnector.ReviewIndex
            readonly property int reviewRow: ri > 0 ? Math.floor((ri - 1) / 2) : -1
            readonly property bool hlW: reviewRow === index && ri % 2 === 1
            readonly property bool hlB: reviewRow === index && ri % 2 === 0

            Rectangle {
                visible: hlW
                x: 29; y: 1; width: 60; height: parent.height - 2
                color: isDarkMode ? "#2a4a2a" : "#c8ebc8"
                radius: 2
            }
            Rectangle {
                visible: hlB
                x: 101; y: 1; width: 60; height: parent.height - 2
                color: isDarkMode ? "#2a4a2a" : "#c8ebc8"
                radius: 2
            }

            Row {
                anchors.fill: parent; anchors.leftMargin: 3; anchors.rightMargin: 3; spacing: 0
                z: 1
                Text {
                    width: 26; height: parent.height
                    text: modelData.n + "."
                    font.pixelSize: 13
                    color: isDarkMode ? "#888" : "#999"
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    width: 60; height: parent.height
                    text: modelData.w
                    font.pixelSize: 13; font.family: "Courier New"
                    color: isDarkMode ? "#ffffff" : "#111111"
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    width: 12; height: parent.height
                    text: "|"
                    font.pixelSize: 13
                    color: isDarkMode ? "#555" : "#bbb"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    visible: modelData.b !== ""
                }
                Text {
                    width: 60; height: parent.height
                    text: modelData.b
                    font.pixelSize: 13; font.family: "Courier New"
                    color: isDarkMode ? "#ffffff" : "#111111"
                    verticalAlignment: Text.AlignVCenter
                    visible: modelData.b !== ""
                }
            }
        }
    }

    // Action buttons: Undo / Draw / Resign
    Row {
        id: actionButtons
        visible: panel.gameInProgress && chessConnector.GameResult === "" && !resignConfirm.visible
        anchors {
            bottom: resultBar.visible ? resultBar.top : parent.bottom
            bottomMargin: 4
            horizontalCenter: parent.horizontalCenter
        }
        spacing: 6

        ActionButton {
            label: "↩"
            tip: panel.isOnlineGame ? "Propose Takeback" : "Undo Move"
            onAction: chessConnector.requestTakeback()
        }
        ActionButton {
            label: "½"
            tip: "Offer Draw"
            visible: panel.isOnlineGame
            onAction: chessConnector.offerDraw()
        }
        ActionButton {
            label: "⚑"
            tip: "Resign"
            onAction: resignConfirm.visible = true
        }
    }

    // Resign confirmation strip
    Row {
        id: resignConfirm
        visible: false
        anchors {
            bottom: resultBar.visible ? resultBar.top : parent.bottom
            bottomMargin: 4
            horizontalCenter: parent.horizontalCenter
        }
        spacing: 6

        Text {
            text: "Resign?"
            color: isDarkMode ? "#ccc" : "#555"
            font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
        }
        ActionButton {
            label: "Yes"; danger: true
            onAction: { resignConfirm.visible = false; chessConnector.resignGame() }
        }
        ActionButton {
            label: "No"
            onAction: resignConfirm.visible = false
        }
    }

    Rectangle {
        id: resultBar
        visible: chessConnector.GameResult !== ""
        width: parent.width; height: 22
        anchors.bottom: parent.bottom
        color: isDarkMode ? "#2a2a2a" : "#e8e8e8"
        radius: 4
        Text {
            anchors.centerIn: parent
            text: chessConnector.GameResult
            font.pixelSize: 12; font.bold: true
            color: isDarkMode ? "#f0c040" : "#8B6914"
        }
    }
}
