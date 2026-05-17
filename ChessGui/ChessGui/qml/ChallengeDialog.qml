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
            text: "Challenge"
            font.pixelSize: 20
            font.bold: true
            color: isDarkMode ? "#ffffff" : "#000000"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "To invite someone to play, give them this link:"
            font.pixelSize: 13
            color: isDarkMode ? "#dddddd" : "#444444"
            wrapMode: Text.WordWrap
            width: parent.width
        }

        Row {
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
            text: "Or let your opponent scan this QR code:"
            font.pixelSize: 13
            color: isDarkMode ? "#dddddd" : "#444444"
            wrapMode: Text.WordWrap
            width: parent.width
        }

        Image {
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
                text: "Waiting for opponent…"
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
