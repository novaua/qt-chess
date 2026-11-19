import QtQuick
import QtQuick.Controls

// A row of mutually exclusive option buttons (side, time control): `options` is [{ label, ... }].
Row {
    id: choiceRow

    property var options: []
    property int checkedIndex: 0
    // Each button's width; 0: the style's own (FluentWinUI3 makes even "1" about 96 wide). For short
    // labels in a long row, e.g. the days per move (BL-23).
    property real buttonWidth: 0

    spacing: Theme.s1

    Repeater {
        model: choiceRow.options
        delegate: Button {
            required property var modelData
            required property int index
            objectName: choiceRow.objectName !== "" ? choiceRow.objectName + "_" + index : ""   // for the UI tests
            text: modelData.label
            width: choiceRow.buttonWidth > 0 ? choiceRow.buttonWidth : implicitWidth
            checked: index === choiceRow.checkedIndex   // not checkable: a click can't uncheck it
            onClicked: choiceRow.checkedIndex = index
        }
    }
}
