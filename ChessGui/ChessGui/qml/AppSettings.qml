pragma Singleton
import QtQuick
import QtCore

// The app's own settings (Settings page), kept by QtCore Settings like before P5: the same keys,
// so nothing is lost. Engine settings that C++ needs live in config.json (ChessConnector.UseFen).
Settings {
    id: settings

    // Appearance
    property int  theme:              -1    // 0 system, 1 light, 2 dark; -1 = not set yet, see darkMode
    property bool darkMode:           false // before the theme setting: only read, to migrate it
    property bool coordinatesInside:  false // the files and ranks on the squares, not around the board
    property bool reduceMotion:       false // no board turn, piece fades or pulsing highlights

    // Sound ("musicEnabled" is the old name of the sound effects switch)
    property bool musicEnabled:       true
    property int  soundVolume:        3     // 1-5; 0 before P5 meant muted
    property int  lastSoundVolume:    3     // before P5: the level to go back to after muting

    // Gameplay
    property bool confirmResign:      true
    property bool showLegalMoves:     true
    property bool highlightLastMove:  true

    // Perceptual loudness: linear 1-5 feels far too loud at level 1, since hearing is roughly logarithmic.
    // A function, not a property: Settings would store every property.
    function effectsVolume() { return Math.pow(soundVolume / 5.0, 2) }

    // Settings are read before Component.onCompleted runs.
    Component.onCompleted: {
        if (theme < 0)
            theme = darkMode ? 2 : 0
        // Muting is its own switch now; the volume is 1-5.
        if (soundVolume <= 0) {
            soundVolume = lastSoundVolume > 0 ? lastSoundVolume : 3
            musicEnabled = false
        }
    }
}
