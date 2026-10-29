import QtQuick
import QtQuick.Controls

// An imported profile has the id or name of one that is already here: add it as a new profile
// ("Bob (2)") or replace the existing one.
AppDialog {
    id: dialog

    property string importName     // the profile in the file
    property string existingName   // the profile already here

    signal importAsNew()
    signal replaceExisting()

    title: "“" + existingName + "” is already here"
    width: 440
    onAccepted: importAsNew()

    Label {
        anchors.left: parent.left
        anchors.right: parent.right
        wrapMode: Text.WordWrap
        text: "Import “" + importName + "” as a new profile, or replace “" + existingName
              + "” with it? Replacing overwrites " + existingName + "'s statistics and saved games."
    }

    footer: DialogButtonBox {
        Button {
            text: "Import as new"
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Replace"
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                dialog.close()
                dialog.replaceExisting()
            }
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
