pragma Singleton
import QtQuick

// Colours, type, spacing and motion that the FluentWinUI3 style doesn't cover.
// Spec: docs/design/ThemeTokens.md. No other QML file hard-codes a colour.
QtObject {
    // 0 System, 1 Light, 2 Dark (AppSettings.theme). Forcing a scheme also restyles the Fluent controls.
    readonly property int scheme: AppSettings.theme
    onSchemeChanged: _applyScheme()
    Component.onCompleted: _applyScheme()   // the saved scheme: no change signal for a first value
    function _applyScheme() {
        Application.styleHints.colorScheme =
            [Qt.ColorScheme.Unknown, Qt.ColorScheme.Light, Qt.ColorScheme.Dark][scheme] ?? Qt.ColorScheme.Unknown
    }
    // -platform offscreen can't set the colour scheme and Fluent then draws its dark palette:
    // match it (screenshots on a locked screen, see the run-chess-app skill).
    readonly property bool _offscreen: Qt.platform.pluginName === "offscreen"
    readonly property bool dark: _offscreen || Application.styleHints.colorScheme === Qt.ColorScheme.Dark

    // The window's palette.accent (system accent colour); set by ChessGame.qml.
    property color systemAccent: "transparent"
    readonly property color accent: systemAccent.a > 0 ? systemAccent : (dark ? "#60CDFF" : "#0078D4")

    // ── Colour roles ──
    readonly property color windowBg:      dark ? "#202020" : "#F3F3F3"
    readonly property color layerBg:       dark ? "#272727" : "#F9F9F9"
    readonly property color cardBg:        dark ? "#2B2B2B" : "#FFFFFF"
    readonly property color cardBgHover:   dark ? "#323232" : "#F6F6F6"
    readonly property color cardStroke:    dark ? "#1D1D1D" : "#E5E5E5"
    readonly property color divider:       dark ? "#3A3A3A" : "#EAEAEA"
    readonly property color textPrimary:   dark ? "#FFFFFF" : "#1B1B1B"
    readonly property color textSecondary: dark ? "#CFCFCF" : "#5F5F5F"
    readonly property color textDisabled:  dark ? "#787878" : "#A0A0A0"
    readonly property color scrim:         dark ? "#66000000" : "#4D000000"
    readonly property color success:       dark ? "#6CCB5F" : "#0F7B0F"
    readonly property color caution:       dark ? "#FCE100" : "#9D5D00"
    readonly property color critical:      dark ? "#FF99A4" : "#C42B1C"
    readonly property color criticalBg:    dark ? "#442726" : "#FDE7E9"
    readonly property color successBg:     dark ? "#393D1B" : "#DFF6DD"   // InfoBar backgrounds (WinUI)
    readonly property color cautionBg:     dark ? "#433519" : "#FFF4CE"
    readonly property color textOnColor:   "#FFFFFF"   // on a saturated fill, e.g. the level badges
    readonly property real  homeBackdropOpacity: dark ? 0.10 : 0.18
    // Selected card / list row (avatar pickers, current level).
    readonly property color selectedBg: Qt.alpha(accent, dark ? 0.25 : 0.15)

    // ── Board (same in both modes) ──
    readonly property color squareLight:  "#ECF0F1"
    readonly property color squareDark:   "#2F4F4F"
    readonly property color lastMove:     Qt.alpha("#A8D8EA", 0.7)
    readonly property color selected:     Qt.alpha(accent, 0.45)
    readonly property color legalMoveDot: Qt.alpha("#000000", 0.22)
    readonly property color checkGlowCenter: "#FFD54A"   // the king in check: yellow in the middle …
    readonly property color checkGlowEdge:   "#E5341F"   // … red towards the edges
    // Pieces off the board (move list): a glow of the other colour, so both show on any panel.
    readonly property color pieceGlowOnBlack: "#FFFFFF"
    readonly property color pieceGlowOnWhite: "#000000"
    readonly property color flaggedKing:  "#C42B1C"   // light-mode critical; dark's pink fades on the board

    // ── Type ramp (family: the app font, Segoe UI) ──
    readonly property font caption:    Qt.font({ pixelSize: 12 })
    readonly property font body:       Qt.font({ pixelSize: 14 })
    readonly property font bodyStrong: Qt.font({ pixelSize: 14, weight: Font.DemiBold })
    readonly property font subtitle:   Qt.font({ pixelSize: 20, weight: Font.DemiBold })
    readonly property font title:      Qt.font({ pixelSize: 28, weight: Font.DemiBold })
    readonly property font titleLarge: Qt.font({ pixelSize: 40, weight: Font.DemiBold })

    // ── Spacing (4 px grid), radius, sizes ──
    readonly property int s1: 4
    readonly property int s2: 8
    readonly property int s3: 12
    readonly property int s4: 16
    readonly property int s5: 24
    readonly property int s6: 32
    readonly property int s7: 48
    readonly property int radiusCard:   8
    readonly property int radiusAvatar: 6
    readonly property int iconSize:      20
    readonly property int iconSizeDense: 16
    readonly property int iconSizeCard:  24   // Home cards
    readonly property int commandBarHeight: 48
    // Game page
    readonly property int playerCardHeight: 60
    readonly property int sidePanelWidth:   280   // move list beside the board (wide windows)
    readonly property int sheetHandleHeight: 44   // the collapsed move sheet (narrow windows)

    // The squares' share of the board item; the rest is the edge for the coordinates outside.
    readonly property real boardSquares: AppSettings.coordinatesInside ? 1.0 : 0.95

    // ── Motion ──
    // Settings → Reduce animations: the board turns at once, pieces don't fade in, nothing pulses.
    readonly property bool reduceMotion: AppSettings.reduceMotion
    readonly property int fast:   83
    readonly property int normal: 167
    readonly property int slow:   250

    function icon(name) { return "qrc:/icons/" + name + ".svg" }
    // Piece image by key: "N" white knight, "n" black knight (the board's own images).
    function piece(key) {
        return key === key.toLowerCase() ? "qrc:/piece/pics/black/" + key.toUpperCase() + ".png"
                                         : "qrc:/piece/pics/white/" + key.toLowerCase() + ".png"
    }
}
