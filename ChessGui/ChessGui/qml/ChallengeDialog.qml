import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Window

Rectangle {
    id: dialog

    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    property string challengeId:  ""
    property string challengeUrl: ""
    property string opponentName: ""    // set: challenge sent to this user directly, no link to share
    property bool   isRematch:    false

    readonly property bool _direct: opponentName !== ""

    width: 380
    height: dialogContent.implicitHeight + 48
    color:        isDarkMode ? "#252525" : "#f4f4f4"
    border.color: isDarkMode ? "#555555" : "#cccccc"
    border.width: 1
    radius: 12

    Column {
        id: dialogContent
        anchors.centerIn: parent
        width: parent.width - 40
        spacing: 16

        Text {
            text: dialog.isRematch ? "Rematch" : "Challenge"
            font.pixelSize: 20
            font.bold: true
            color: isDarkMode ? "#ffffff" : "#000000"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            visible: dialog._direct
            textFormat: Text.StyledText
            text: "Rematch challenge sent to <b>" + dialog.opponentName + "</b> on Lichess. "
                + "They can accept it from their Lichess notifications."
            font.pixelSize: 13
            color: isDarkMode ? "#dddddd" : "#444444"
            wrapMode: Text.WordWrap
            width: parent.width
        }

        Text {
            visible: !dialog._direct
            text: dialog.isRematch
                ? "Your opponent played anonymously, so send them this new link to play again:"
                : "To invite someone to play, give them this link:"
            font.pixelSize: 13
            color: isDarkMode ? "#dddddd" : "#444444"
            wrapMode: Text.WordWrap
            width: parent.width
        }

        Row {
            visible: !dialog._direct
            width: parent.width
            spacing: 6

            QQC.TextField {
                id: urlField
                text: dialog.challengeUrl
                readOnly: true
                width: parent.width - copyBtn.implicitWidth - 6
                color: isDarkMode ? "#ffffff" : "#000000"
                background: Rectangle {
                    radius: 4
                    color:        isDarkMode ? "#3c3c3c" : "#ffffff"
                    border.color: isDarkMode ? "#555555" : "#cccccc"
                    border.width: 1
                }
            }

            Button {
                id: copyBtn
                text: "Copy"
                implicitWidth: 60
                implicitHeight: urlField.height
                onClicked: {
                    urlField.selectAll()
                    urlField.copy()
                }
            }
        }

        Text {
            visible: !dialog._direct
            text: "Or let your opponent scan this QR code:"
            font.pixelSize: 13
            color: isDarkMode ? "#dddddd" : "#444444"
            wrapMode: Text.WordWrap
            width: parent.width
        }

        Image {
            visible: !dialog._direct
            anchors.horizontalCenter: parent.horizontalCenter
            width: 180; height: 180
            source: dialog.challengeUrl !== ""
                ? ("https://api.qrserver.com/v1/create-qr-code/?size=180x180&data="
                   + encodeURIComponent(dialog.challengeUrl))
                : ""
            fillMode: Image.PreserveAspectFit
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8

            QQC.BusyIndicator {
                width: 20; height: 20
                running: true
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: dialog._direct ? "Waiting for " + dialog.opponentName + " to accept…"
                                     : "Waiting for opponent…"
                font.pixelSize: 13
                color: isDarkMode ? "#dddddd" : "#444444"
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Button {
            text: "Cancel"
            implicitWidth: 90
            implicitHeight: 32
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: lichessClient.cancelChallenge(dialog.challengeId)
        }
    }
}
