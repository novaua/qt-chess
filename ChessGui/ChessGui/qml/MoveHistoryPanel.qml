import QtQuick
import QtQuick.Window

Rectangle {
    id: panel

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

    ListView {
        id: listView
        anchors {
            top: header.bottom; topMargin: 2
            left: parent.left; leftMargin: 3
            right: parent.right; rightMargin: 3
            bottom: resultBar.visible ? resultBar.top : parent.bottom
            bottomMargin: 3
        }
        clip: true
        model: chessConnector.MoveHistory
        spacing: 1
        onCountChanged: Qt.callLater(function() { listView.positionViewAtEnd() })

        delegate: Rectangle {
            required property var modelData
            required property int index
            width: listView.width; height: 26
            color: index % 2 === 0
                ? (isDarkMode ? "#252525" : "#f8f8f8")
                : (isDarkMode ? "#2d2d2d" : "#eeeeee")
            radius: 2

            Row {
                anchors.fill: parent; anchors.leftMargin: 3; anchors.rightMargin: 3; spacing: 0
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
