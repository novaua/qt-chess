import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One setting in a SectionCard on the Settings page: its name and an optional description on the
// left, the control (the children) on the right.
RowLayout {
    id: row

    property string title
    property string description
    default property alias control: controls.data

    spacing: Theme.s4
    Layout.fillWidth: true

    ColumnLayout {
        spacing: 2
        Layout.fillWidth: true

        Label {
            text: row.title
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Label {
            text: row.description
            visible: text !== ""
            font: Theme.caption
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    RowLayout {
        id: controls
        spacing: Theme.s2
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
    }
}
