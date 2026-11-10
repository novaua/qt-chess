import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus
import "lichess.js" as Lichess

// Every unfinished game (BL-17), from Home's Continue when there are two or more: the robot's
// autosaved game first (upper left), then the Lichess games, most urgent first (as on the Online
// page). Pushed over Home; Back pops it. A game that starts replaces it by the game page
// (ChessGame.qml route()).
Page {
    id: page
    objectName: "games"

    signal backRequested()

    property bool _busy: false   // a Lichess game is being rejoined

    readonly property var _robot: ChessConnector.ContinueGame   // {} without one
    readonly property bool _hasRobot: _robot.fen !== undefined
    readonly property var _games: UserManager.lichessConnected ? LichessClient.ongoingGames : []
    readonly property int _count: (_hasRobot ? 1 : 0) + _games.length
    readonly property int _myTurnCount: (_hasRobot && _robot.playerToMove ? 1 : 0) + _games.filter(g => g.isMyTurn).length
    readonly property var _levels: ChessConnector.Levels

    function _refresh() {
        if (UserManager.lichessConnected && !LichessClient.ongoingGamesLoading)
            LichessClient.refreshOngoingGames()
    }
    // A rejoin that is starting is dropped, so no game starts unexpectedly. Not when a game did
    // start: route() replaces this page by the game page then.
    function _dropStarting() {
        if (_busy && !ChessConnector.InGame)
            LichessClient.stopStream()
        _busy = false
    }
    // Pushed, or shown again: a fresh list (an opponent may have moved or ended a game).
    StackView.onActivated: {
        _busy = false
        _refresh()
    }
    StackView.onRemoved: _dropStarting()
    // Signed out (expired) or linked anew while the page is open: the rejoin belonged to the other account.
    readonly property bool _connected: UserManager.lichessConnected
    on_ConnectedChanged: _dropStarting()

    // Continue (rejoin), or lichess.org for a game the Board API can't play here.
    function _openGame(game) {
        if (!game.canPlayHere) {
            Qt.openUrlExternally("https://lichess.org/" + game.gameId)
            return
        }
        errorBar.visible = false
        _busy = true
        LichessClient.rejoinGame(game.gameId)
    }
    function _robotDetails() {
        const level = _levels.find(l => l.level === _robot.level)
        return "Level " + _robot.level + (level ? " · " + level.name : "")
               + " · you play " + (_robot.playerPlaysWhite ? "White" : "Black")
    }

    Connections {
        target: LichessClient
        function onGameStarted(gameId, playingAsWhite, opponentName, opponentAvatarUrl) { page._busy = false }
        function onNetworkError(message) {
            if (!page._busy)
                return
            page._busy = false
            errorBar.show("Couldn't continue the game: " + message, "critical")
        }
    }

    background: Rectangle { color: Theme.windowBg }

    header: CommandBar {
        messageArea: barFill   // the window's short messages show over it
        IconButton {
            icon.source: Theme.icon("back")
            tip: "Back (Esc)"
            onClicked: page.backRequested()
        }
        Label {
            id: barFill
            text: "Continue a game"
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
            width: Math.min(flick.width - 2 * Theme.s5, 960)
            x: (flick.width - width) / 2
            y: Theme.s5
            spacing: Theme.s3

            Label {
                text: page._count === 0
                      ? (LichessClient.ongoingGamesLoading ? "Loading…" : "No unfinished games.")
                      : page._count + (page._count === 1 ? " unfinished game" : " unfinished games")
                        + (page._myTurnCount === 0 ? "" : page._count === 1 ? " · your turn" : " · your turn in " + page._myTurnCount)
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            GridLayout {
                id: grid
                readonly property real cardWidth: (content.width - (columns - 1) * columnSpacing) / columns
                visible: page._count > 0
                // Cards about 220 wide (a board big enough to read), up to four in a row.
                columns: Math.max(1, Math.min(4, Math.floor((content.width + columnSpacing) / (220 + columnSpacing))))
                columnSpacing: Theme.s3
                rowSpacing: Theme.s3
                Layout.fillWidth: true

                // The robot's game: the board as it was left, from the player's side.
                GameCard {
                    objectName: "robotGameCard"
                    visible: page._hasRobot
                    Layout.preferredWidth: grid.cardWidth
                    fen: page._robot.fen ?? ""
                    flipped: page._hasRobot && !page._robot.playerPlaysWhite
                    lastMove: page._robot.lastMove ?? ""
                    title: "Robot"
                    playerToMove: page._hasRobot && page._robot.playerToMove
                    waitingText: "Robot's turn"
                    details: page._hasRobot ? page._robotDetails() : ""
                    busy: page._busy
                    onActivated: ChessConnector.continueGame()
                }

                Repeater {
                    model: page._games
                    delegate: GameCard {
                        required property var modelData
                        Layout.preferredWidth: grid.cardWidth
                        fen: modelData.fen
                        flipped: modelData.color === "black"
                        lastMove: modelData.lastMove
                        title: Lichess.gameTitle(modelData)
                        playerToMove: modelData.isMyTurn
                        details: "Lichess · " + Lichess.gameDetails(modelData)
                        actionText: modelData.canPlayHere ? "Continue" : "Open on lichess.org"
                        actionIcon: modelData.canPlayHere ? "play" : "link"
                        actionTip: modelData.canPlayHere ? "" : "This kind of game can't be played in Chess++ yet"
                        busy: page._busy
                        onActivated: page._openGame(modelData)
                    }
                }
            }

            // The Lichess games come from lichess.org: refresh them, or say why they're missing.
            RowLayout {
                visible: UserManager.lichessConnected
                spacing: Theme.s2
                Button {
                    text: "Refresh"
                    flat: true
                    enabled: !LichessClient.ongoingGamesLoading
                    onClicked: page._refresh()
                }
                BusyIndicator {
                    visible: LichessClient.ongoingGamesLoading
                    running: visible
                    Layout.preferredWidth: 20; Layout.preferredHeight: 20
                }
                Label {
                    visible: LichessClient.ongoingGamesError !== ""
                    text: "Couldn't load your Lichess games: " + LichessClient.ongoingGamesError
                    color: Theme.critical
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }
    }

    // Over the content, under the header: seen wherever the page is scrolled to.
    InfoBar {
        id: errorBar
        anchors { top: parent.top; topMargin: Theme.s2; horizontalCenter: parent.horizontalCenter }
        width: content.width
        z: 1
    }
}
