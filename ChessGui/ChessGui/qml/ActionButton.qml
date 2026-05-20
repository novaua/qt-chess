import QtQuick
import QtQuick.Window
import QtQuick.Controls

Rectangle {
    property string label: ""
    property string tip: ""
    property bool   danger: false
    signal action()

    readonly property bool _dark: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    height: 22; radius: 3
    width: label.length === 1 ? 36 : Math.max(36, _lbl.implicitWidth + 16)

    color: danger
        ? (_ma.containsMouse ? (_dark ? "#6a1a1a" : "#f0cccc") : (_dark ? "#3a1010" : "#fde8e8"))
        : (_ma.containsMouse ? (_dark ? "#555"    : "#ccc")    : (_dark ? "#333"    : "#e8e8e8"))
    border.color: danger ? (_dark ? "#883333" : "#cc8888") : (_dark ? "#555" : "#ccc")
    border.width: 1

    Text {
        id: _lbl
        anchors.centerIn: parent
        text: parent.label
        font.pixelSize: 13
        color: danger ? (_dark ? "#ffaaaa" : "#880000") : (_dark ? "#ddd" : "#333")
    }

    MouseArea {
        id: _ma
        anchors.fill: parent
        hoverEnabled: true
        onClicked: parent.action()
        ToolTip.visible: containsMouse && parent.tip !== ""
        ToolTip.text: parent.tip
        ToolTip.delay: 400
    }
}
