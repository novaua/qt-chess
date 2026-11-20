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
    property bool rematchSwapsColours: true // a robot or two-player rematch gives the player the other colour (ChessConnector.RematchSwapsColours)
    // The side last chosen in each setup dialog (0 White, 1 Black, 2 Random: vs computer only), apart from the
    // side of the last game, which a rematch swaps (#193); -1: not chosen yet, the last game's side.
    property int  computerSide:       -1
    property int  twoPlayerSide:      -1
    property int  clockLimitSec:      0     // the last new game's time control; 0 and 0: no clock
    property int  clockIncSec:        0
    property bool chess960:           false // the last new game's variant: Chess960 (LI-01A), else standard
    property bool onlineChess960:     false // the Online page's last challenge: Chess960 (LI-01A stage 2), apart from the local games'
    property bool quickRated:         true  // the Online page's quick pairing: rated seeks, as lichess.org's default (LI-09)
    property bool quickChess960:      false // its variant, apart from the challenge card's: two pickers on one page

    // Computer
    property int  analysisDepth:      14    // the engine's search depth per position of an analysis (ChessConnector.AnalysisDepth, BL-19)

    // Data
    property int  gamesKept:          100   // finished games kept per profile (GameLibrary.keepCount, BL-19)

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
