import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus

// Settings → Data → Restore…: pick one of the backups of users.json. Restore emits restoreRequested;
// ChessGame.qml calls UserManager.restoreBackup() and reports the result.
AppDialog {
    id: dialog

    signal restoreRequested(int number)

    property var _backups: []
    property int _selected: 0   // the backup's number; 0 = none picked

    title: "Restore profiles from a backup"
    width: 460
    acceptEnabled: _selected > 0

    // Read when opened: users.json changes with every game, so "the same as now" may have changed.
    onAboutToShow: {
        _backups = UserManager.backups
        var first = _backups.find(b => !b.current)
        _selected = first ? first.number : 0
    }
    onAccepted: if (_selected > 0) restoreRequested(_selected)

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.s2

        Label {
            text: "The profiles and statistics go back to the backup. Your profiles now are backed up first, so you can undo this."
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        ButtonGroup { id: choices }

        Repeater {
            model: dialog._backups
            delegate: RadioButton {
                required property var modelData
                ButtonGroup.group: choices
                checked: modelData.number === dialog._selected
                enabled: !modelData.current
                text: modelData.date + " · " + modelData.profiles.join(", ")
                      + (modelData.current ? " (the same as now)" : "")
                onClicked: dialog._selected = modelData.number
                Layout.fillWidth: true
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: "Restore"
            highlighted: true
            enabled: dialog._selected > 0
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
