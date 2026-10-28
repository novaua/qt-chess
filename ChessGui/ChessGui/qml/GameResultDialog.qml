import QtQuick

Rectangle {
    id: dialog

    property string winner: ""

    width: 160
    height: dialogContent.implicitHeight + 2 * Theme.s3
    color: Theme.cardBg
    border.color: Theme.cardStroke
    border.width: 1
    radius: Theme.radiusCard

    Column {
        id: dialogContent
        anchors.centerIn: parent
        spacing: Theme.s2
        width: parent.width - 2 * Theme.s3

        Text {
            text: dialog.winner
            font: Theme.bodyStrong
            color: Theme.textPrimary
            anchors.horizontalCenter: parent.horizontalCenter
            wrapMode: Text.WordWrap
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
        }

        Rectangle {
            width: parent.width
            height: 1
            color: Theme.divider
        }

        Repeater {
            model: [
                "Games played: " + userManager.gamesPlayed,
                "Human won: "    + userManager.humanWins,
                "Robot won: "    + userManager.computerWins
            ]
            Text {
                text: modelData
                color: Theme.textPrimary
                font: Theme.body
                wrapMode: Text.WordWrap
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Text {
            text: "Since: " + userManager.statsCreatedDate
            color: Theme.textSecondary
            font: Theme.caption
            wrapMode: Text.WordWrap
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
