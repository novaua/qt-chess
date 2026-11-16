import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus
import "games.js" as Games

// The games library (BL-19): every local game of the profile, vs the robot or between two players,
// kept from its first move, newest first. A finished game opens as a replay (Open); an unfinished one
// goes on (Continue); Delete removes it for good. Pushed over Home (Home's Games card, Ctrl+O); Back
// pops it. A game that opens replaces it by the game page (ChessGame.qml route()). The filters show
// one kind of game; Export PGN saves one game as a file (the window's file dialog).
Page {
    id: page
    objectName: "library"

    signal backRequested()
    signal exportRequested(string gameId)

    // The filter buttons, in their order: which kept games each one shows.
    readonly property var _filters: [
        { label: "All",         shows: g => true },
        { label: "vs Robot",    shows: g => g.kind === "computer" },
        { label: "Two players", shows: g => g.kind === "local" },
        { label: "Unfinished",  shows: g => !g.finished },
    ]
    property int filter: 0
    readonly property var _allGames: GameLibrary.games
    readonly property var _games: _allGames.filter(_filters[filter].shows)
    readonly property var _levels: ChessConnector.Levels
    readonly property int _unfinished: GameLibrary.unfinishedCount

    background: Rectangle { color: Theme.windowBg }

    header: CommandBar {
        messageArea: barFill   // the window's short messages show over it
        IconButton {
            objectName: "backButton"
            icon.source: Theme.icon("back")
            tip: "Back (Esc)"
            onClicked: page.backRequested()
        }
        Label {
            id: barFill
            text: "Games"
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
                objectName: "gamesSummary"
                text: page._allGames.length === 0
                      ? "No games yet. Every game you play against the robot or with another player at this computer is kept here from its first move."
                      : page._allGames.length + (page._allGames.length === 1 ? " game" : " games")
                        + (page._unfinished === 0 ? "" : " · " + page._unfinished + " unfinished")
                        + " · the last " + GameLibrary.keepCount + " finished games are kept"
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            ChoiceRow {
                objectName: "gamesFilter"
                visible: page._allGames.length > 0
                options: page._filters
                checkedIndex: page.filter
                onCheckedIndexChanged: page.filter = checkedIndex
                Layout.fillWidth: true
            }

            Label {
                objectName: "gamesFilterEmpty"
                visible: page._allGames.length > 0 && page._games.length === 0
                text: ["", "No games against the robot.", "No two-player games.", "No unfinished games."][page.filter]
                color: Theme.textSecondary
                Layout.fillWidth: true
            }

            GridLayout {
                id: grid
                readonly property real cardWidth: (content.width - (columns - 1) * columnSpacing) / columns
                visible: page._games.length > 0
                // Cards about 220 wide (a board big enough to read), up to four in a row.
                columns: Math.max(1, Math.min(4, Math.floor((content.width + columnSpacing) / (220 + columnSpacing))))
                columnSpacing: Theme.s3
                rowSpacing: Theme.s3
                Layout.fillWidth: true

                Repeater {
                    model: page._games
                    delegate: GameCard {
                        required property var modelData
                        required property int index
                        objectName: "gameCard_" + index
                        Layout.preferredWidth: grid.cardWidth
                        fen: modelData.fen
                        flipped: !modelData.playerPlaysWhite
                        lastMove: modelData.lastMove
                        title: Games.title(modelData, page._levels)
                        playerToMove: !modelData.finished && modelData.kind === "computer" && modelData.playerToMove
                        waitingText: modelData.finished ? Games.resultWithRating(modelData) : Games.turnWord(modelData)
                        details: Games.details(modelData)
                        actionText: modelData.finished ? "Open" : "Continue"
                        actionIcon: modelData.finished ? "folder-open" : "play"
                        deletable: true
                        exportable: true
                        onExportRequested: page.exportRequested(modelData.id)
                        onActivated: {
                            if (modelData.finished) ChessConnector.openGame(modelData.id)
                            else                    ChessConnector.continueGame(modelData.id)
                        }
                        onDeleteRequested: {
                            deleteConfirm.gameId = modelData.id
                            deleteConfirm.gameTitle = Games.title(modelData, page._levels)
                            deleteConfirm.open()
                        }
                    }
                }
            }
        }
    }

    ConfirmDialog {
        id: deleteConfirm
        objectName: "deleteGameConfirm"
        property string gameId
        property string gameTitle
        title: "Delete this game?"
        text: "The game " + (gameTitle !== "" ? "vs " + gameTitle + " " : "") + "and its moves are removed for good."
        acceptText: "Delete"
        onAccepted: GameLibrary.remove(gameId)
    }
}
