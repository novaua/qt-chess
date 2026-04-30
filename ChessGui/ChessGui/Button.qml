import QtQuick

Rectangle {
    id: container

    SystemPalette { id: activePalette }

    property string text: "Button"

    signal clicked

    width: buttonLabel.width + 20; height: buttonLabel.height + 5
    border { width: 1; color: Qt.darker(activePalette.button) }
    antialiasing: true
    radius: 8

    // color the button with a gradient
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: {
                if (mouseArea.pressed)
                    return activePalette.dark
                else
                    return activePalette.light
            }
        }
        GradientStop { position: 1.0; color: activePalette.button }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: container.clicked();
    }

    Text {
        id: buttonLabel
        anchors.centerIn: container
        color: {
            var c = activePalette.button
            var luminance = 0.299 * c.r + 0.587 * c.g + 0.114 * c.b
            return luminance > 0.5 ? "#111111" : "#eeeeee"
        }
        text: container.text
    }
}
