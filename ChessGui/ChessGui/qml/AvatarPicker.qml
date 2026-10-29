import QtQuick
import ChessPlusPlus

// The avatars to choose from (AvatarProvider.names); `selected` is the chosen one's name.
Grid {
    id: picker

    property string selected
    property int    cellSize: 52

    signal picked(string name)

    columns: 5
    spacing: Theme.s2

    Repeater {
        model: AvatarProvider.names
        delegate: Rectangle {
            required property string modelData
            readonly property bool isSelected: picker.selected === modelData

            width: picker.cellSize; height: picker.cellSize
            radius: Theme.radiusCard
            color: isSelected ? Theme.selectedBg
                 : avatarMouse.containsMouse ? Theme.cardBgHover : "transparent"
            border.color: isSelected ? Theme.accent : "transparent"
            border.width: 2

            Accessible.role: Accessible.RadioButton
            Accessible.name: modelData
            Accessible.checked: isSelected

            Image {
                anchors.centerIn: parent
                width: picker.cellSize - 12; height: width
                source: "qrc:/app/pics/avatars/" + modelData + ".png"
                fillMode: Image.PreserveAspectFit
            }

            MouseArea {
                id: avatarMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    picker.selected = modelData
                    picker.picked(modelData)
                }
            }
        }
    }
}
