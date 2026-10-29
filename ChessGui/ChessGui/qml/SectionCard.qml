import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A titled card on a full page (Profile, later Settings). Children go in a column under the title.
Rectangle {
    id: card

    property string title
    property string subtitle
    default property alias content: column.data

    implicitHeight: column.implicitHeight + 2 * Theme.s4
    radius: Theme.radiusCard
    color: Theme.cardBg
    border.color: Theme.cardStroke

    ColumnLayout {
        id: column
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: Theme.s4 }
        spacing: Theme.s3

        ColumnLayout {
            spacing: 2
            visible: card.title !== ""
            Layout.fillWidth: true

            Label {
                text: card.title
                font: Theme.bodyStrong
                Layout.fillWidth: true
            }
            Label {
                text: card.subtitle
                visible: text !== ""
                font: Theme.caption
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}
