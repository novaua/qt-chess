.pragma library

// Words for a kept local game (GameLibrary.games, BL-19), shared by Home's Continue card, the Continue
// list and the Games page. `game` is a GameRecord as QML sees it (GameRecord::toVariant).

// Who the player played: "Robot · Level 3 · Club Player", or the other player's name.
function title(game, levels) {
    if (game.kind !== "computer")
        return game.opponentName === "" ? "Guest" : game.opponentName
    const level = levels ? levels.find(l => l.level === game.level) : undefined
    return "Robot · Level " + game.level + (level ? " · " + level.name : "")
}

// The move number a position after `ply` moves goes on with: after 23 plies, Black's 12th move.
function moveNumberAfter(ply) {
    return Math.floor(ply / 2) + 1
}

// The setup dialogs' line for Play from here: "From the position after 12. Nf3: Black to move."
function fromHereText(from) {
    return "From the position after " + from.move + ": " + (from.whiteToMove ? "White" : "Black") + " to move."
}

// The card's line under the title: the result or whose turn, the day, the moves.
function details(game) {
    var parts = []
    parts.push(game.kind === "computer" ? "you play " + (game.playerPlaysWhite ? "White" : "Black")
                                        : (game.playerPlaysWhite ? "White" : "Black") + " vs " + (game.playerPlaysWhite ? "Black" : "White"))
    if (game.forkOfId)   // Play from here: where it began
        parts.push("from move " + moveNumberAfter(game.forkPly))
    const when = game.finished ? game.finishedAt : game.startedAt
    if (when && !isNaN(when.getTime()))
        parts.push(Qt.formatDate(when, "d MMM yyyy"))
    const moves = Math.ceil(game.plyCount / 2)
    parts.push(moves + (moves === 1 ? " move" : " moves"))
    return parts.join(" · ")
}

// The player's view of the result: "You won", "You lost", "Draw"; "" while unfinished.
function resultWord(game) {
    if (!game.finished || game.result === "")
        return ""
    if (game.result === "1/2-1/2")
        return "Draw"
    const whiteWon = game.result === "1-0"
    if (game.kind === "computer")
        return whiteWon === game.playerPlaysWhite ? "You won" : "You lost"
    return whiteWon ? "White won" : "Black won"
}

// Whose turn it is in an unfinished game, from the player's side.
function turnWord(game) {
    if (game.finished)
        return ""
    if (game.kind === "computer")
        return game.playerToMove ? "Your turn" : "Robot's turn"
    return (game.playerToMove === game.playerPlaysWhite ? "White" : "Black") + " to move"
}

// Home's one-line description of the unfinished local games.
function continueText(games, levels) {
    if (games.length === 0)
        return ""
    if (games.length === 1)
        return (games[0].kind === "computer" ? "vs " : "") + title(games[0], levels)
    return games.length + " local games"
}
