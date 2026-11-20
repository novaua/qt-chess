import QtQuick
import QtQuick.Controls
import ChessPlusPlus
import "lichess.js" as Lichess

// An unfinished Lichess game (a row of LichessClient.ongoingGames) as a GameCard: Continue rejoins it
// (ChessConnector.continueOnlineGame), or lichess.org opens a game the Board API can't play here.
// On the Online page and in the Continue list (BL-17). A right click on it opens a menu (LI-10): Continue,
// Open on lichess.org, Copy link.
GameCard {
    id: card

    property var game
    readonly property string url: "https://lichess.org/" + (game?.gameId ?? "")
    property bool lichessLabel: false   // "Lichess · " before the details, beside other kinds of games

    fen: game.fen
    flipped: game.color === "black"
    lastMove: game.lastMove
    title: Lichess.gameTitle(game)
    playerToMove: game.isMyTurn
    details: (lichessLabel ? "Lichess · " : "") + Lichess.gameDetails(game)
    playable: game.canPlayHere
    actionText: playable ? "Continue" : "Open on lichess.org"
    actionIcon: playable ? "play" : "link"
    actionTip: playable ? "" : "This kind of game can't be played in Chess++ yet"
    busy: ChessConnector.StartingOnlineGame
    starting: ChessConnector.StartingGameId === game.gameId
    objectName: "ongoingGame_" + (game?.gameId ?? "")
    hasMenu: true
    onMenuRequested: openMenu()

    onActivated: {
        if (playable)
            ChessConnector.continueOnlineGame(game.gameId)
        else
            Qt.openUrlExternally(card.url)
    }

    // The menu where the card was right-clicked (also on its board and its button: they take only the
    // left button); at the action button for openMenu(): the Menu key or Shift+F10 on it (and the scene).
    function openMenu() { gameMenu.popup(card, Theme.s3, card.height - Theme.s3) }
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (eventPoint) => gameMenu.popup(card, eventPoint.position)
    }
    Menu {
        id: gameMenu
        objectName: "gameMenu"
        MenuItem {
            objectName: "gameMenuContinue"
            text: "Continue"
            icon.source: Theme.icon("play")
            visible: card.playable
            height: visible ? implicitHeight : 0
            enabled: !card.busy && !card.starting
            onTriggered: card.activated()
        }
        MenuItem {
            objectName: "gameMenuOpen"
            text: "Open on lichess.org"
            icon.source: Theme.icon("link")
            onTriggered: Qt.openUrlExternally(card.url)
        }
        MenuItem {
            objectName: "gameMenuCopyLink"
            text: "Copy link"
            icon.source: Theme.icon("copy")
            onTriggered: AppInfo.copyToClipboard(card.url, "Link copied")
        }
    }
}
