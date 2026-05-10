import QtQuick
import QtQuick.Window
import QtQuick.Controls as Controls

Controls.Button {
    id: self

    // Window.window.color is a C++ Q_PROPERTY (colorChanged signal), so bindings
    // update reliably when the ApplicationWindow background switches dark/light.
    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    contentItem: Text {
        text: self.text
        color: self.isDarkMode ? "#ffffff" : "#000000"
        font: self.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitHeight: 24
        color: self.down
            ? (self.isDarkMode ? "#282828" : "#c0c0c0")
            : (self.isDarkMode ? "#3c3c3c" : "#e0e0e0")
        border.color: self.isDarkMode ? "#555555" : "#bbbbbb"
        radius: 4
    }
}
