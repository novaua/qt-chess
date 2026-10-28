import QtQuick
import QtQuick.Controls

// Base for every modal dialog: centred over the window with a scrim behind it, Enter runs
// accept() and Esc runs reject(). Give it a `footer: DialogButtonBox { … }`; the box's
// Accept/Reject role buttons are wired to accept()/reject() by Dialog itself.
//
// Opened with open(). The dialogs live in ChessGame.qml; pages ask for them through signals.
Dialog {
    id: dialog

    property bool acceptEnabled: true   // false: Enter does nothing (e.g. a required field is empty)
    property bool rejectEnabled: true   // false: Esc does nothing (a dialog that can't be cancelled)

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose      // Esc goes through the shortcut below, so it emits rejected()

    Overlay.modal: Rectangle { color: Theme.scrim }

    Shortcut {
        sequences: ["Return", "Enter"]
        enabled: dialog.opened && dialog.acceptEnabled
        onActivated: dialog.accept()
    }
    Shortcut {
        sequence: "Escape"
        enabled: dialog.opened && dialog.rejectEnabled
        onActivated: dialog.reject()
    }
}
