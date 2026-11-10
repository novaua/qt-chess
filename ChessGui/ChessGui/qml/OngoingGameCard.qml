import QtQuick
import ChessPlusPlus
import "lichess.js" as Lichess

// An unfinished Lichess game (a row of LichessClient.ongoingGames) as a GameCard: Continue rejoins it
// (ChessConnector.continueOnlineGame), or lichess.org opens a game the Board API can't play here.
// On the Online page and in the Continue list (BL-17).
GameCard {
    id: card

    property var game
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

    onActivated: {
        if (playable)
            ChessConnector.continueOnlineGame(game.gameId)
        else
            Qt.openUrlExternally("https://lichess.org/" + game.gameId)
    }
}
