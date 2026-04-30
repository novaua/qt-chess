import QtQuick

Rectangle {
    id: container

    SystemPalette { id: activePalette }

    readonly property bool isDarkMode: (0.299 * activePalette.window.r
                                      + 0.587 * activePalette.window.g
                                      + 0.114 * activePalette.window.b) <= 0.5

    property string text: "Button"

    signal clicked

    width: buttonLabel.width + 20; height: buttonLabel.height + 5
    border { width: 1; color: isDarkMode ? "#606060" : "#aaaaaa" }
    antialiasing: true
    radius: 8

    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: mouseArea.pressed
                ? (isDarkMode ? "#282828" : "#888888")
                : (isDarkMode ? "#505050" : "#f0f0f0")
        }
        GradientStop {
            position: 1.0
            color: isDarkMode ? "#3c3c3c" : "#d6d6d6"
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: container.clicked();
    }

    Text {
        id: buttonLabel
        anchors.centerIn: container
        color: isDarkMode ? "#eeeeee" : "#111111"
        text: container.text
    }
}
