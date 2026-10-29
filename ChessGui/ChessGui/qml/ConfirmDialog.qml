import QtQuick
import QtQuick.Controls

// "Are you sure?" before a destructive action: `title`, an explaining `text`, and the action's
// verb on the accept button (`acceptText`, e.g. "Resign").
AppDialog {
    id: dialog

    property alias text: message.text
    property string acceptText: "OK"

    width: 400

    Label {
        id: message
        anchors.left: parent.left
        anchors.right: parent.right
        wrapMode: Text.WordWrap
        visible: text !== ""
    }

    footer: DialogButtonBox {
        defaultButton: primaryButton // highlighted (the box sets it, Qt 6.11+)
        Button {
            id: primaryButton
            text: dialog.acceptText
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
