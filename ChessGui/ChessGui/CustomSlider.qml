// CustomSlider.qml
import QtQuick
import QtQuick.Window
import QtQuick.Controls
pragma ComponentBehavior: Bound

Slider {
    id: control
    from: 1
    to: 5
    stepSize: 1
    snapMode: Slider.SnapAlways

    leftPadding: handle.width / 2
    rightPadding: handle.width / 2

    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * control.availableWidth - width / 2
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 16
        implicitHeight: 16
        radius: 8
        color: control.pressed
            ? (control.isDarkMode ? "#282828" : "#c0c0c0")
            : (control.isDarkMode ? "#3c3c3c" : "#f6f6f6")
        border.color: control.isDarkMode ? "#777777" : "#bdbebf"
    }

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: 4
        radius: 2
        color: control.isDarkMode ? "#555555" : "#bdbebf"

        Repeater {
            id: repeater
            model: control.stepSize > 0 ? (control.to - control.from) / control.stepSize + 1 : 0

            delegate: Rectangle {
                required property int index

                x: (repeater.count > 1)
                    ? index * (parent.width / (repeater.count - 1)) - width / 2
                    : parent.width / 2 - width / 2

                y: parent.height + 5
                width: 1
                height: 6
                color: control.isDarkMode ? "#aaaaaa" : "#888888"
            }
        }
    }
}
