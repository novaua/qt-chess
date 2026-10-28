import QtQuick
import QtQuick.Layouts

// The strip at the top of a page (a Page's `header`). Its children are laid out in a row.
Rectangle {
    default property alias content: row.data

    implicitHeight: Theme.commandBarHeight
    color: Theme.layerBg

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width; height: 1
        color: Theme.divider
    }

    RowLayout {
        id: row
        anchors { fill: parent; leftMargin: Theme.s1; rightMargin: Theme.s1 }
        spacing: Theme.s1
    }
}
