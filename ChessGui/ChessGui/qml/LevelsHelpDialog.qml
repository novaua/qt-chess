import QtQuick
import QtQuick.Window

// Explains the five computer levels. Opened from the start menu's "?" button, the ⚙ menu,
// F1, or by clicking the robot's avatar during a game.
Rectangle {
    id: dialog

    signal closeRequested()
    signal levelPicked(int level)

    property int  currentLevel: 0      // highlighted row; 0 = none
    property bool selectable:   false  // rows can be clicked to pick a level (start menu)

    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    width: 400
    height: helpCol.implicitHeight + 40
    color:        isDarkMode ? "#252525" : "#f4f4f4"
    border.color: isDarkMode ? "#555555" : "#cccccc"
    border.width: 1
    radius: 12

    // Swallow clicks so they don't reach the board underneath.
    MouseArea { anchors.fill: parent }

    Column {
        id: helpCol
        anchors.centerIn: parent
        width: parent.width - 40
        spacing: 12

        Text {
            text: "Computer Levels"
            font.pixelSize: 20
            font.bold: true
            color: isDarkMode ? "#ffffff" : "#000000"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            width: parent.width
            text: dialog.selectable
                ? "Pick a level to play against the robot."
                : "How strong each robot level plays."
            font.pixelSize: 13
            color: isDarkMode ? "#aaaaaa" : "#555555"
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Repeater {
            model: chessConnector.Levels

            Rectangle {
                id: row
                required property var modelData
                readonly property bool isCurrent: modelData.level === dialog.currentLevel

                width: helpCol.width
                height: rowContent.implicitHeight + 16
                radius: 8
                color: isCurrent
                    ? (isDarkMode ? "#1d3a52" : "#dcebf8")
                    : rowMouse.containsMouse && dialog.selectable
                        ? (isDarkMode ? "#333333" : "#e6e6e6")
                        : (isDarkMode ? "#2d2d2d" : "#ffffff")
                border.color: isCurrent ? "#0078d4" : (isDarkMode ? "#444444" : "#dddddd")
                border.width: 1

                Row {
                    id: rowContent
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter
                              leftMargin: 10; rightMargin: 10 }
                    spacing: 10

                    Rectangle {
                        width: 28; height: 28; radius: 14
                        color: root.levelColor(row.modelData.level)
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            anchors.centerIn: parent
                            text: row.modelData.level
                            font.pixelSize: 14
                            font.bold: true
                            color: "#ffffff"
                        }
                    }

                    Column {
                        width: parent.width - 38
                        spacing: 2

                        Item {
                            width: parent.width
                            height: nameText.implicitHeight
                            Text {
                                id: nameText
                                text: row.modelData.name
                                font.pixelSize: 15
                                font.bold: true
                                color: isDarkMode ? "#ffffff" : "#000000"
                            }
                            Text {
                                anchors.right: parent.right
                                anchors.baseline: nameText.baseline
                                text: root.levelRatingText(row.modelData)
                                font.pixelSize: 13
                                color: isDarkMode ? "#bbbbbb" : "#444444"
                            }
                        }
                        Text {
                            width: parent.width
                            text: row.modelData.hint
                            font.pixelSize: 12
                            color: isDarkMode ? "#aaaaaa" : "#555555"
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: dialog.selectable
                    enabled: dialog.selectable
                    cursorShape: dialog.selectable ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: dialog.levelPicked(row.modelData.level)
                }
            }
        }

        Text {
            width: parent.width
            textFormat: Text.StyledText
            text: "<b>Levels 1–2</b> pick at random among the engine's best moves, so the robot "
                + "makes human-like mistakes. Their ratings (≈) are rough estimates.<br>"
                + "<b>Levels 3–5</b> use Stockfish's built-in strength limit. Its Elo is measured "
                + "against other engines, so it plays stronger than a person with the same rating."
            font.pixelSize: 11
            color: isDarkMode ? "#999999" : "#666666"
            wrapMode: Text.WordWrap
        }

        Button {
            text: "Done"
            implicitWidth: 100
            implicitHeight: 32
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: dialog.closeRequested()
        }
    }
}
