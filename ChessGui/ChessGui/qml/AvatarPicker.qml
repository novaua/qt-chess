import QtQuick
import ChessPlusPlus

// The avatars to choose from (AvatarProvider.names); `selected` is the chosen one's name.
// One Tab stop, like a group of radio buttons: the arrow keys pick the next avatar.
Grid {
    id: picker

    property string selected
    property int    cellSize: 52

    signal picked(string name)

    columns: 5
    spacing: Theme.s2
    activeFocusOnTab: true
    Accessible.role: Accessible.List
    Accessible.name: "Avatar"

    function pick(name) {
        selected = name
        picked(name)
    }
    function _step(delta) {
        var names = AvatarProvider.names
        var i = names.indexOf(selected)
        pick(names[i < 0 ? 0 : Math.max(0, Math.min(names.length - 1, i + delta))])
    }
    Keys.onLeftPressed:  _step(-1)
    Keys.onRightPressed: _step(1)
    Keys.onUpPressed:    _step(-columns)
    Keys.onDownPressed:  _step(columns)

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

            FocusRing { visible: picker.activeFocus && isSelected }

            MouseArea {
                id: avatarMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: picker.pick(modelData)
            }
        }
    }
}
