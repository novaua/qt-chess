import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Explains the five computer levels. Opened from the start menu's "?" button, the ⚙ menu,
// F1, or by clicking the robot's avatar during a game.
AppDialog {
    id: dialog

    signal levelPicked(int level)

    property int  currentLevel: 0      // highlighted row; 0 = none
    property bool selectable:   false  // rows can be clicked to pick a level (start menu)

    title: "Computer levels"
    width: 420

    ColumnLayout {
        id: helpCol
        anchors.fill: parent
        spacing: Theme.s2

        Label {
            Layout.fillWidth: true
            text: dialog.selectable
                ? "Pick a level to play against the robot."
                : "How strong each robot level plays."
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }

        Repeater {
            model: chessConnector.Levels

            Rectangle {
                id: row
                required property var modelData
                readonly property bool isCurrent: modelData.level === dialog.currentLevel

                Layout.fillWidth: true
                implicitHeight: rowContent.implicitHeight + 2 * Theme.s2
                radius: Theme.radiusCard
                color: isCurrent ? Theme.selectedBg
                     : rowMouse.containsMouse && dialog.selectable ? Theme.cardBgHover
                     : Theme.cardBg
                border.color: isCurrent ? Theme.accent : Theme.cardStroke
                border.width: 1

                RowLayout {
                    id: rowContent
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter
                              leftMargin: Theme.s3; rightMargin: Theme.s3 }
                    spacing: Theme.s3

                    Rectangle {
                        Layout.preferredWidth: 28; Layout.preferredHeight: 28
                        radius: 14
                        color: root.levelColor(row.modelData.level)
                        Text {
                            anchors.centerIn: parent
                            text: row.modelData.level
                            font: Theme.bodyStrong
                            color: Theme.textOnColor
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: row.modelData.name
                                font: Theme.bodyStrong
                                Layout.fillWidth: true
                            }
                            Label {
                                text: root.levelRatingText(row.modelData)
                                color: Theme.textSecondary
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            text: row.modelData.hint
                            font: Theme.caption
                            color: Theme.textSecondary
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

        Label {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s1
            textFormat: Text.StyledText
            text: "<b>Levels 1–2</b> pick at random among the engine's best moves, so the robot "
                + "makes human-like mistakes. Their ratings (≈) are rough estimates.<br>"
                + "<b>Levels 3–5</b> use Stockfish's built-in strength limit. Its Elo is measured "
                + "against other engines, so it plays stronger than a person with the same rating."
            font: Theme.caption
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }
    }

    footer: DialogButtonBox {
        Button {
            text: "Done"
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
