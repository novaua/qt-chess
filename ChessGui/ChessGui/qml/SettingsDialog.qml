import QtQuick
import ChessPlusPlus
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

AppDialog {
    id: dialog

    title: "Settings"
    width: 420

    // Same perceptual curve as ChessGame.qml's _sfxVolume, so the preview
    // matches what the player will actually hear in-game.
    readonly property real _previewVolume: Math.pow(appSettings.soundVolume / 5.0, 2)

    SoundEffect { id: sndPreviewMove;     source: "qrc:/sounds/move.wav";          volume: dialog._previewVolume }
    SoundEffect { id: sndPreviewOpponent; source: "qrc:/sounds/move_opponent.wav"; volume: dialog._previewVolume }

    GridLayout {
        anchors.fill: parent
        columns: 2
        columnSpacing: Theme.s4
        rowSpacing: Theme.s2

        Label { text: "App theme"; Layout.fillWidth: true }
        ComboBox {
            model: ["Use system setting", "Light", "Dark"]
            currentIndex: Theme.scheme
            onActivated: (index) => appSettings.theme = index
            Layout.preferredWidth: 180
        }

        Label { text: "Sound"; Layout.fillWidth: true }
        Switch {
            id: musicSwitch
            checked: appSettings.musicEnabled
            onToggled: {
                appSettings.musicEnabled = checked
                if (checked && appSettings.soundVolume === 0)
                    appSettings.soundVolume = appSettings.lastSoundVolume > 0
                        ? appSettings.lastSoundVolume : 3
            }
        }

        Label { text: "Volume"; Layout.fillWidth: true }
        Slider {
            id: volumeSlider
            Layout.preferredWidth: 180
            from: 0; to: 5; stepSize: 1
            snapMode: Slider.SnapAlways
            value: appSettings.soundVolume
            enabled: appSettings.musicEnabled
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

        Label { text: "Send positions to the engine as FEN"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        Switch {
            checked: ChessConnector.UseFen
            onToggled: ChessConnector.UseFen = checked
        }
    }

    footer: DialogButtonBox {
        Button {
            text: "Done"
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
