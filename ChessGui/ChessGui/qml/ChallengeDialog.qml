import QtQuick
import ChessPlusPlus
import QtQuick.Controls
import QtQuick.Layouts

// Waiting for the opponent to accept a Lichess challenge. Cancel (or Esc) withdraws it.
AppDialog {
    id: dialog

    property string challengeId:  ""
    property string challengeUrl: ""
    property string opponentName: ""    // set: challenge sent to this user directly, no link to share
    property bool   isRematch:    false

    readonly property bool _direct: opponentName !== ""

    title: isRematch ? "Rematch" : "Challenge"
    width: 400
    acceptEnabled: false

    onRejected: LichessClient.cancelChallenge(dialog.challengeId)

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.s3

        Label {
            visible: dialog._direct
            textFormat: Text.StyledText
            text: "Rematch challenge sent to <b>" + dialog.opponentName + "</b> on Lichess. "
                + "They can accept it from their Lichess notifications."
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            visible: !dialog._direct
            text: dialog.isRematch
                ? "Your opponent played anonymously, so send them this new link to play again:"
                : "To invite someone to play, give them this link:"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout {
            visible: !dialog._direct
            spacing: Theme.s2
            Layout.fillWidth: true

            TextField {
                id: urlField
                text: dialog.challengeUrl
                readOnly: true
                Layout.fillWidth: true
            }

            Button {
                text: "Copy"
                icon.source: Theme.icon("copy")
                onClicked: {
                    urlField.selectAll()
                    urlField.copy()
                }
            }
        }

        Label {
            visible: !dialog._direct
            text: "Or let your opponent scan this QR code:"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Image {
            visible: !dialog._direct
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 180; Layout.preferredHeight: 180
            source: dialog.challengeUrl !== ""
                ? ("https://api.qrserver.com/v1/create-qr-code/?size=180x180&data="
                   + encodeURIComponent(dialog.challengeUrl))
                : ""
            fillMode: Image.PreserveAspectFit
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.s2

            BusyIndicator {
                Layout.preferredWidth: 20; Layout.preferredHeight: 20
                running: dialog.visible
            }

            Label {
                text: dialog._direct ? "Waiting for " + dialog.opponentName + " to accept…"
                                     : "Waiting for opponent…"
                color: Theme.textSecondary
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: dialog.isRematch ? "Cancel rematch" : "Cancel challenge"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
