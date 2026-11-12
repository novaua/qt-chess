import QtQuick
import QtQuick.Controls

// Leaving a Lichess game that isn't over (Home, Esc, closing the window): the game goes on at
// lichess.org. Leave (or Quit, when closing the window) is accepted(), Resign is resignRequested(),
// Stay is rejected().
AppDialog {
    id: dialog

    property bool quitting: false   // opened by closing the window: Leave quits the app

    signal resignRequested()

    title: quitting ? "Quit during the game?" : "Leave the game?"
    width: 440

    Label {
        anchors.left: parent.left
        anchors.right: parent.right
        wrapMode: Text.WordWrap
        text: "The game goes on at lichess.org and your clock is running. You can come back to it "
              + "from Continue or Play online."
    }

    footer: DialogButtonBox {
        defaultButton: primaryButton // highlighted (the box sets it, Qt 6.11+)
        Button {
            id: primaryButton
            text: dialog.quitting ? "Quit" : "Leave"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            objectName: "resignButton"
            text: "Resign"
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                dialog.close()
                dialog.resignRequested()
            }
        }
        Button {
            text: "Stay"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
