import QtQuick
import QtQuick.Window
import QtQuick.Controls as Controls

Controls.Button {
    id: control

    readonly property bool isDarkMode: Window.window ? !!Window.window.isDarkMode : false

    contentItem: Text {
        text: control.text
        color: control.isDarkMode ? "#ffffff" : "#000000"
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 40
        color: control.down
            ? (control.isDarkMode ? "#282828" : "#c0c0c0")
            : (control.isDarkMode ? "#3c3c3c" : "#e0e0e0")
        border.color: control.isDarkMode ? "#555555" : "#bbbbbb"
        radius: 4
    }
}
