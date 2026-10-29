import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus

// First run (no profiles yet): sign in with Lichess, which makes a profile named after the
// account, or create a local profile. ChessGame.qml owns the dialogs and answers the signals.
Page {
    id: page
    objectName: "welcome"

    signal createProfileRequested()
    signal importRequested()

    readonly property bool _linking: UserManager.lichessLinking
    property string _error

    Connections {
        target: UserManager
        function onLichessLinkFailed(error) { page._error = error }
    }

    background: Rectangle {
        color: Theme.windowBg
        Image {
            anchors.fill: parent
            source: "qrc:/app/pics/ChessBackground.jpg"
            fillMode: Image.PreserveAspectCrop
            opacity: Theme.homeBackdropOpacity
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * Theme.s6, 420)
        spacing: Theme.s3

        Image {
            source: "qrc:/app/pics/chess_icon.svg"
            sourceSize.width: 72; sourceSize.height: 72
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: "Chess++"
            font: Theme.titleLarge
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: "Play against the computer, with a friend on this PC, or online on Lichess."
            color: Theme.textSecondary
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.bottomMargin: Theme.s4
        }

        Button {
            text: page._linking ? "Waiting for the browser…" : "Continue with Lichess"
            icon.source: Theme.icon("globe")
            highlighted: true
            enabled: !page._linking
            Layout.fillWidth: true
            onClicked: {
                page._error = ""
                UserManager.createUserWithLichess()
            }
        }

        // BL-08: the browser may not have opened, or the user changed their mind.
        RowLayout {
            visible: page._linking
            spacing: Theme.s2
            Layout.fillWidth: true

            BusyIndicator {
                running: page._linking
                Layout.preferredWidth: 20; Layout.preferredHeight: 20
            }
            Label {
                text: "Sign in on lichess.org and allow Chess++ to play for you."
                font: Theme.caption
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Button {
                text: "Cancel"
                onClicked: UserManager.cancelLichessLink()
            }
        }

        Label {
            visible: page._error !== "" && !page._linking
            text: "Couldn't sign in with Lichess: " + page._error
            color: Theme.critical
            font: Theme.caption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Button {
            text: "Create local profile"
            icon.source: Theme.icon("person-add")
            enabled: !page._linking
            Layout.fillWidth: true
            onClicked: page.createProfileRequested()
        }

        Label {
            text: "Local profiles work offline. You can link a Lichess account later in your profile."
            font: Theme.caption
            color: Theme.textSecondary
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.topMargin: Theme.s2
        }

        Button {
            text: "Import a profile…"
            icon.source: Theme.icon("import")
            flat: true
            enabled: !page._linking
            Layout.alignment: Qt.AlignHCenter
            onClicked: page.importRequested()
        }
    }
}
