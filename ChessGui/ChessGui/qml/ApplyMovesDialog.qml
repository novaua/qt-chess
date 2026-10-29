import QtQuick
import ChessPlusPlus
import QtQuick.Controls

AppDialog {
    id: dialog

    title: "Enter moves"
    width: 440

    onOpened: movesField.forceActiveFocus()
    onAccepted: ChessConnector.applyMoves(movesField.text)
    onClosed: movesField.text = ""

    TextField {
        id: movesField
        anchors.left: parent.left
        anchors.right: parent.right
        placeholderText: "e2e4 d7d5 g1f3 ..."
    }

    footer: DialogButtonBox {
        Button {
            text: "Apply"
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
