import QtQuick
import QtQuick.Controls

// "Are you sure?" before a destructive action: `title`, an explaining `text`, and the action's
// verb on the accept button (`acceptText`, e.g. "Resign"). An optional second action between
// them (`actionText`, e.g. "Save and leave") emits actionRequested().
AppDialog {
    id: dialog

    property alias text: message.text
    property string acceptText: "OK"
    property string actionText: ""      // empty: no second action
    property string cancelText: "Cancel"

    signal actionRequested()

    width: 400

    Label {
        id: message
        anchors.left: parent.left
        anchors.right: parent.right
        wrapMode: Text.WordWrap
        visible: text !== ""
    }

    footer: DialogButtonBox {
        id: buttons
        defaultButton: primaryButton // highlighted (the box sets it, Qt 6.11+)
        Button {
            id: primaryButton
            objectName: "acceptButton"   // the UI tests find it in its dialog (AppDriver.clickIn)
            text: dialog.acceptText
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            id: actionButton
            objectName: "actionButton"
            text: dialog.actionText
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                dialog.close()
                dialog.actionRequested()
            }
        }
        Button {
            text: dialog.cancelText
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }

    // Without a second action its button goes: a hidden one keeps its place in the box.
    Component.onCompleted: {
        if (actionText === "") {
            buttons.removeItem(actionButton)
            actionButton.destroy()
        }
    }
}
