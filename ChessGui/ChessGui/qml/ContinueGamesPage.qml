import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus
import "games.js" as Games

// Every unfinished game (BL-17), from Home's Continue when there are two or more: the local games
// first (the games library, BL-19: vs the robot and two players, newest first), then the Lichess
// games, most urgent first (as on the Online page). Pushed over Home; Back pops it. A game that
// starts replaces it by the game page (ChessGame.qml route()); a rejoin that fails says so over the
// command bar (gameStartFailed).
Page {
    id: page
    objectName: "games"

    signal backRequested()

    readonly property var _local: GameLibrary.unfinishedGames
    readonly property var _games: UserManager.lichessConnected ? LichessClient.ongoingGames : []
    readonly property int _count: _local.length + _games.length
    readonly property int _myTurnCount: _local.filter(g => g.playerToMove).length + _games.filter(g => g.isMyTurn).length
    readonly property var _levels: ChessConnector.Levels

    // Pushed, or shown again: a fresh list (an opponent may have moved or ended a game).
    StackView.onActivated: refreshRow.refresh()
    // Back while a rejoin is under way: dropped, so no game starts unexpectedly.
    StackView.onRemoved: ChessConnector.cancelOnlineStart()

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

                // The local games: the board as it was left, from the player's side.
                Repeater {
                    model: page._local
                    delegate: GameCard {
                        required property var modelData
                        required property int index
                        objectName: "localGameCard_" + index
                        Layout.preferredWidth: grid.cardWidth
                        fen: modelData.fen
                        flipped: !modelData.playerPlaysWhite
                        lastMove: modelData.lastMove
                        title: Games.title(modelData, page._levels)
                        playerToMove: modelData.kind === "computer" && modelData.playerToMove
                        waitingText: Games.turnWord(modelData)
                        details: Games.details(modelData)
                        busy: ChessConnector.StartingOnlineGame
                        onActivated: ChessConnector.continueGame(modelData.id)
                    }
                }

                Repeater {
                    model: page._games
                    delegate: OngoingGameCard {
                        required property var modelData
                        game: modelData
                        lichessLabel: true
                        Layout.preferredWidth: grid.cardWidth
                    }
                }
            }

            // The Lichess games come from lichess.org: refresh them, or say why they're missing.
            OngoingGamesRefresh {
                id: refreshRow
                visible: UserManager.lichessConnected
                showError: true
                Layout.fillWidth: true
            }
        }
    }
}
