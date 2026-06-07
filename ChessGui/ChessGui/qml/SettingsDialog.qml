import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Window
import QtMultimedia

Rectangle {
    id: dialog

    signal closeRequested()

    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    // Same perceptual curve as ChessGame.qml's _sfxVolume, so the preview
    // matches what the player will actually hear in-game.
    readonly property real _previewVolume: Math.pow(appSettings.soundVolume / 5.0, 2)

    SoundEffect { id: sndPreviewMove;     source: "qrc:/sounds/move.wav";          volume: dialog._previewVolume }
    SoundEffect { id: sndPreviewOpponent; source: "qrc:/sounds/move_opponent.wav"; volume: dialog._previewVolume }

    width: 300
    height: settingsCol.implicitHeight + 48
    color:        isDarkMode ? "#252525" : "#f4f4f4"
    border.color: isDarkMode ? "#555555" : "#cccccc"
    border.width: 1
    radius: 12

    Column {
        id: settingsCol
        anchors.centerIn: parent
        width: parent.width - 40
        spacing: 16

        Text {
            text: "Settings"
            font.pixelSize: 20
            font.bold: true
            color: isDarkMode ? "#ffffff" : "#000000"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Flickable {
            width: parent.width
            height: flickContent.implicitHeight
            contentHeight: flickContent.implicitHeight
            clip: true

            Column {
                id: flickContent
                width: parent.width
                spacing: 4

                Row {
                    width: parent.width
                    Text {
                        text: "Dark Mode"
                        color: isDarkMode ? "#dddddd" : "#222222"
                        font.pixelSize: 15
                        width: parent.width - darkSwitch.width
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Controls.Switch {
                        id: darkSwitch
                        checked: appSettings.darkMode
                        onToggled: appSettings.darkMode = checked
                    }
                }

                Row {
                    width: parent.width
                    Text {
                        text: "Sound"
                        color: isDarkMode ? "#dddddd" : "#222222"
                        font.pixelSize: 15
                        width: parent.width - musicSwitch.width
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Controls.Switch {
                        id: musicSwitch
                        checked: appSettings.musicEnabled
                        onToggled: {
                            appSettings.musicEnabled = checked
                            if (checked && appSettings.soundVolume === 0)
                                appSettings.soundVolume = appSettings.lastSoundVolume > 0
                                    ? appSettings.lastSoundVolume : 3
                        }
                    }
                }

                Row {
                    width: parent.width
                    Text {
                        text: "Loudness"
                        color: isDarkMode ? "#dddddd" : "#222222"
                        font.pixelSize: 15
                        width: parent.width - volumeSlider.width
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    CustomSlider {
                        id: volumeSlider
                        width: 150
                        from: 0; to: 5; stepSize: 1
                        value: appSettings.soundVolume
                        enabled: appSettings.musicEnabled
                        anchors.verticalCenter: parent.verticalCenter
                        onMoved: {
                            appSettings.soundVolume = value
                            if (value > 0)
                                appSettings.lastSoundVolume = value
                            else
                                appSettings.musicEnabled = false

                            var snd = (value % 2 === 0) ? sndPreviewOpponent : sndPreviewMove
                            if (value > 0 && snd.status === SoundEffect.Ready)
                                snd.play()
                        }
                    }
                }

                Row {
                    width: parent.width
                    Text {
                        text: "Use FEN (engine)"
                        color: isDarkMode ? "#dddddd" : "#222222"
                        font.pixelSize: 15
                        width: parent.width - fenSwitch.width
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Controls.Switch {
                        id: fenSwitch
                        checked: chessConnector.UseFen
                        onToggled: chessConnector.UseFen = checked
                    }
                }
            }
        }

        Button {
            text: "Done"
            implicitWidth: 100
            implicitHeight: 32
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: dialog.closeRequested()
        }
    }
}
