import QtQuick
import ChessPlusPlus
import QtQuick.Controls
import QtQuick.Layouts

// A new local profile: name and avatar. Editing a profile is on the Profile page.
AppDialog {
    id: dialog

    signal confirmed(string name, string avatarName)

    title: "Create profile"
    width: 380
    acceptEnabled: nameField.text.trim().length > 0

    onOpened: {
        nameField.text = ""
        avatars.selected = AvatarProvider.names[0]
        nameField.forceActiveFocus()
    }
    onAccepted: confirmed(nameField.text.trim(), avatars.selected)

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.s3

        TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: "Your name"
            maximumLength: 40
        }

        Label {
            text: "Choose your avatar"
            color: Theme.textSecondary
        }

        AvatarPicker {
            id: avatars
            Layout.alignment: Qt.AlignHCenter
        }
    }

    footer: DialogButtonBox {
        Button {
            text: "Create profile"
            highlighted: true
            enabled: dialog.acceptEnabled
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
