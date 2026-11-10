import QtQuick
import QtQuick.Layouts

// The strip at the top of a page (a Page's `header`). Its children are laid out in a row.
Rectangle {
    default property alias content: row.data
    // The item that fills the free middle (a title or a filler): the window's short messages show over
    // it (BL-13 step 3, CommandBarMessage), in this bar's coordinates.
    property Item messageArea: null
    readonly property real messageX: messageArea ? row.x + messageArea.x : 0
    readonly property real messageWidth: messageArea ? messageArea.width : 0
    // How much of it the title's text takes, from its left edge (0 for a filler).
    readonly property real messageTextWidth: messageArea ? Math.min(messageArea.implicitWidth, messageArea.width) : 0

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
