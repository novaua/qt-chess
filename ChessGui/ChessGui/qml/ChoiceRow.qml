import QtQuick
import QtQuick.Controls

// A row of mutually exclusive option buttons (side, time control): `options` is [{ label, ... }].
Row {
    id: choiceRow

    property var options: []
    property int checkedIndex: 0

    spacing: Theme.s1

    Repeater {
        model: choiceRow.options
        delegate: Button {
            required property var modelData
            required property int index
            text: modelData.label
            checked: index === choiceRow.checkedIndex   // not checkable: a click can't uncheck it
            onClicked: choiceRow.checkedIndex = index
        }
    }
}
