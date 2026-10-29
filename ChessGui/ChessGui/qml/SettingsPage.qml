import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtMultimedia
import ChessPlusPlus

// Settings: appearance, sound, gameplay, the computer, data, advanced and About. Pushed over Home or
// the game like the Profile page; Back or Esc pops it. Changes apply at once (AppSettings, or
// config.json through ChessConnector for the engine protocol).
Page {
    id: page
    objectName: "settings"

    signal backRequested()
    signal levelsHelpRequested()
    signal exportRequested(string userId)
    signal importRequested()
    signal restoreRequested()

    property string _backupMessage

    readonly property var _backups: UserManager.backups
    readonly property string _repository: "https://github.com/novaua/qt-chess"

    // The About card at the top of the view (the "about" screenshot scene).
    function showAbout() {
        flick.contentY = Math.max(0, Math.min(aboutCard.y + content.y - Theme.s5,
                                              flick.contentHeight - flick.height))
    }

    Component.onCompleted: AppInfo.probeEngine()

    SoundEffect { id: previewMove;     source: "qrc:/sounds/move.wav";          volume: AppSettings.effectsVolume() }
    SoundEffect { id: previewOpponent; source: "qrc:/sounds/move_opponent.wav"; volume: AppSettings.effectsVolume() }

    // A text with links in it (opened in the browser).
    component LinkLabel: Label {
        textFormat: Text.StyledText
        linkColor: Theme.accent
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        onLinkActivated: link => Qt.openUrlExternally(link)
        HoverHandler { cursorShape: parent.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor }
    }

    background: Rectangle { color: Theme.windowBg }

    header: CommandBar {
        IconButton {
            icon.source: Theme.icon("back")
            tip: "Back (Esc)"
            onClicked: page.backRequested()
        }
        Label {
            text: "Settings"
            font: Theme.bodyStrong
            Layout.fillWidth: true
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: content.implicitHeight + 2 * Theme.s5
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: content
            width: Math.min(flick.width - 2 * Theme.s5, 680)
            x: (flick.width - width) / 2
            y: Theme.s5
            spacing: Theme.s3

            // ── Appearance ──
            SectionCard {
                title: "Appearance"
                Layout.fillWidth: true

                SettingRow {
                    title: "App theme"
                    ComboBox {
                        model: ["Use system setting", "Light", "Dark"]
                        currentIndex: AppSettings.theme
                        onActivated: index => AppSettings.theme = index
                        Accessible.name: "App theme"
                        Layout.preferredWidth: 200
                    }
                }
                SettingRow {
                    title: "Board coordinates"
                    description: "The files (a–h) and ranks (1–8): around the board, or in the corners of the squares for a bigger board."
                    ComboBox {
                        model: ["Around the board", "On the squares"]
                        currentIndex: AppSettings.coordinatesInside ? 1 : 0
                        onActivated: index => AppSettings.coordinatesInside = index === 1
                        Accessible.name: "Board coordinates"
                        Layout.preferredWidth: 200
                    }
                }
                SettingRow {
                    title: "Reduce animations"
                    description: AppInfo.windowsAnimationsOff
                                 ? "On, because animation effects are off in Windows (Accessibility → Visual effects)."
                                 : "The board turns at once, pages and pieces appear without sliding or fading in, and nothing pulses."
                    Switch {
                        checked: AppSettings.reduceMotion || AppInfo.windowsAnimationsOff
                        enabled: !AppInfo.windowsAnimationsOff
                        onToggled: AppSettings.reduceMotion = checked
                        Accessible.name: "Reduce animations"
                    }
                }
            }

            // ── Sound ──
            SectionCard {
                title: "Sound"
                Layout.fillWidth: true

                SettingRow {
                    title: "Sound effects"
                    description: "Moves, check, checkmate and a win."
                    Switch {
                        checked: AppSettings.musicEnabled
                        onToggled: AppSettings.musicEnabled = checked
                        Accessible.name: "Sound effects"
                    }
                }
                SettingRow {
                    title: "Volume"
                    enabled: AppSettings.musicEnabled
                    IconImageLabel { icon: "volume-mute" }
                    Slider {
                        from: 1; to: 5; stepSize: 1
                        snapMode: Slider.SnapAlways
                        value: AppSettings.soundVolume
                        Accessible.name: "Volume"
                        Layout.preferredWidth: 160
                        onMoved: {
                            AppSettings.soundVolume = value
                            var sound = value % 2 === 0 ? previewOpponent : previewMove
                            if (sound.status === SoundEffect.Ready)
                                sound.play()
                        }
                    }
                    IconImageLabel { icon: "volume" }
                }
            }

            // ── Gameplay ──
            SectionCard {
                title: "Gameplay"
                Layout.fillWidth: true

                SettingRow {
                    title: "Confirm before resigning"
                    description: "Ask before the game is given up."
                    Switch {
                        checked: AppSettings.confirmResign
                        onToggled: AppSettings.confirmResign = checked
                        Accessible.name: "Confirm before resigning"
                    }
                }
                SettingRow {
                    title: "Show legal moves"
                    description: "Dots on the squares the selected piece can move to."
                    Switch {
                        checked: AppSettings.showLegalMoves
                        onToggled: AppSettings.showLegalMoves = checked
                        Accessible.name: "Show legal moves"
                    }
                }
                SettingRow {
                    title: "Highlight the last move"
                    description: "Tint the squares the last move came from and went to."
                    Switch {
                        checked: AppSettings.highlightLastMove
                        onToggled: AppSettings.highlightLastMove = checked
                        Accessible.name: "Highlight the last move"
                    }
                }
            }

            // ── Computer ──
            SectionCard {
                title: "Computer"
                Layout.fillWidth: true

                // BL-06 adds a "Change…" button here once other UCI engines can be chosen.
                SettingRow {
                    title: AppInfo.engineState === "ok" ? AppInfo.engineName
                         : AppInfo.engineState === "checking" ? "Looking for the chess engine…"
                         : "Chess engine not found"
                    description: AppInfo.engineState === "missing"
                        ? "The robot needs Stockfish. Reinstall Chess++, or put stockfish.exe on the PATH."
                          + (AppInfo.enginePath !== "" ? " " + AppInfo.enginePath + " didn't answer." : "")
                        : AppInfo.enginePath
                }
                SettingRow {
                    title: "Computer levels"
                    description: "How each level of the robot plays, from beginner to full strength."
                    Button {
                        text: "Show levels"
                        onClicked: page.levelsHelpRequested()
                    }
                }
            }

            // ── Data ──
            SectionCard {
                title: "Data"
                subtitle: "Backups hold your profiles and statistics. One is made when the app starts after a change, and the last 3 are kept. "
                          + "Saved games aren't in backups: export a profile to keep them too."
                Layout.fillWidth: true

                SettingRow {
                    title: "Back up profiles"
                    description: page._backupMessage !== "" ? page._backupMessage
                               : page._backups.length > 0 ? "Latest backup: " + page._backups[0].date
                               : "No backup yet."
                    Button {
                        text: "Back up now"
                        enabled: !UserManager.dataDamaged
                        onClicked: page._backupMessage = UserManager.backupNow()
                            ? "Backed up: " + UserManager.backups[0].date
                            : "Couldn't back up the profiles. See the log for details."
                    }
                }
                SettingRow {
                    title: "Restore from a backup"
                    description: ChessConnector.InGame ? "Leave the game to restore a backup."
                               : "Your profiles now are backed up first, so a restore can be undone."
                    Button {
                        text: "Restore…"
                        enabled: page._backups.length > 0 && !ChessConnector.InGame
                        onClicked: page.restoreRequested()
                    }
                }
                SettingRow {
                    title: "Profiles"
                    description: "One file per profile with its statistics and saved games, for another PC or as a copy. Never your Lichess sign-in."
                    Button {
                        text: "Export…"
                        icon.source: Theme.icon("export")
                        enabled: UserManager.isLoggedIn
                        onClicked: page.exportRequested(UserManager.activeUserId)
                    }
                    Button {
                        text: "Import…"
                        icon.source: Theme.icon("import")
                        onClicked: page.importRequested()
                    }
                }
                SettingRow {
                    title: "Data folder"
                    description: "Profiles, settings, saves and backups."
                    Button {
                        text: "Open"
                        icon.source: Theme.icon("folder-open")
                        onClicked: Qt.openUrlExternally(AppInfo.dataFolder)
                    }
                }
            }

            // ── Advanced ──
            SectionCard {
                title: "Advanced"
                Layout.fillWidth: true

                SettingRow {
                    title: "Send the game to the engine as"
                    description: "A position (FEN) is shortest. Try the move list only if another engine plays strange moves."
                    ComboBox {
                        model: ["Position (FEN)", "Start position and moves"]
                        currentIndex: ChessConnector.UseFen ? 0 : 1
                        onActivated: index => ChessConnector.UseFen = index === 0
                        Accessible.name: "Send the game to the engine as"
                        Layout.preferredWidth: 200
                    }
                }
                SettingRow {
                    title: "Log files"
                    description: "Attach them to a problem report."
                    Button {
                        text: "Open"
                        icon.source: Theme.icon("folder-open")
                        onClicked: Qt.openUrlExternally(AppInfo.logFolder)
                    }
                }
            }

            // ── About ──
            SectionCard {
                id: aboutCard
                Layout.fillWidth: true

                RowLayout {
                    spacing: Theme.s3
                    Layout.fillWidth: true
                    Image {
                        source: "qrc:/app/pics/app-icon.png"
                        mipmap: true
                        Layout.preferredWidth: 48; Layout.preferredHeight: 48
                    }
                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Label { text: "Chess++"; font: Theme.subtitle }
                        Label {
                            text: "Version " + AppInfo.version + " · " + AppInfo.buildInfo
                            font: Theme.caption
                            color: Theme.textSecondary
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
                RowLayout {
                    spacing: Theme.s2
                    Button {
                        text: "Source code on GitHub"
                        icon.source: Theme.icon("link")
                        flat: true
                        onClicked: Qt.openUrlExternally(page._repository)
                    }
                    Button {
                        text: "Report a problem"
                        icon.source: Theme.icon("link")
                        flat: true
                        onClicked: Qt.openUrlExternally(page._repository + "/issues")
                    }
                }

                Label { text: "Licences"; font: Theme.bodyStrong }
                LinkLabel {
                    text: "Chess++ is free software under the "
                          + "<a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">GNU General Public License v3</a>."
                }
                LinkLabel {
                    text: "Built with Qt " + AppInfo.qtVersion + " by The Qt Company, used under the "
                          + "<a href=\"https://www.gnu.org/licenses/lgpl-3.0.html\">GNU Lesser General Public License v3</a>. "
                          + "<a href=\"https://download.qt.io/official_releases/qt/\">Qt source code</a>."
                }
                LinkLabel {
                    text: "The robot is Stockfish by the Stockfish developers, under the "
                          + "<a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">GNU General Public License v3</a>. "
                          + "<a href=\"https://github.com/official-stockfish/Stockfish\">Stockfish source code</a>."
                }

                Label { text: "Keyboard shortcuts"; font: Theme.bodyStrong }
                GridLayout {
                    columns: 2
                    columnSpacing: Theme.s4
                    rowSpacing: Theme.s1
                    Layout.fillWidth: true

                    Repeater {
                        model: [
                            ["Ctrl+N", "New game vs the computer; after a game, a new one like it"],
                            ["Ctrl+O", "Open the saved game (Home)"],
                            ["Tab  Space", "Move between buttons and cards, and press one"],
                            ["Esc", "Back; in a game, close the move sheet or go Home"],
                            ["← →", "Previous or next move"],
                            ["Home  End", "First move or the live position"],
                            ["F", "Turn the board around"],
                            ["Ctrl+Z", "Take back a move"],
                            ["Ctrl+S", "Save the game"],
                            ["F1", "Computer levels"],
                            ["Ctrl+,", "Settings"],
                            ["Alt+S", "Type in moves (two-player game)"],
                            ["Enter  Esc", "In a dialog: the main button, or Cancel"],
                        ]
                        delegate: Repeater {
                            required property var modelData
                            model: modelData
                            Label {
                                required property string modelData
                                required property int index
                                text: modelData
                                font: index === 0 ? Theme.bodyStrong : Theme.body
                                color: index === 0 ? Theme.textPrimary : Theme.textSecondary
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: index === 1
                                Layout.preferredWidth: index === 0 ? 96 : -1
                            }
                        }
                    }
                }
            }
        }
    }

    // A small icon beside a control (the volume slider's ends).
    component IconImageLabel: IconImage {
        property string icon
        source: Theme.icon(icon)
        sourceSize.width: Theme.iconSizeDense; sourceSize.height: Theme.iconSizeDense
        color: enabled ? Theme.textSecondary : Theme.textDisabled
    }
}
