.pragma library
.import "lichess.js" as Lichess

// Words for a kept game (GameLibrary.games, BL-19; a finished Lichess game too, LI-14), shared by Home's
// Continue card, the Continue list and the Games page. `game` is a GameRecord as QML sees it
// (GameRecord::toVariant).

// Who the player played: "Robot · Level 3 · Club Player", "GM Magnus (2850)" on Lichess, or the other
// player's name.
function title(game, levels) {
    if (game.kind === "lichess")
        return (game.opponentTitle ? game.opponentTitle + " " : "") + game.opponentName
               + (game.opponentRating ? " (" + game.opponentRating + ")" : "")
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

// A clock's minutes as lichess.org writes them: 10, ½, 1.5.
function minutesText(seconds) {
    if (seconds % 60 === 0)
        return String(seconds / 60)
    return ({ 15: "\u00bc", 30: "\u00bd", 45: "\u00be", 90: "1.5" })[seconds] ?? (seconds / 60).toFixed(1)
}

// A kept Lichess game's speed and clock: "Rapid 10+5", "Correspondence · 3 days" (LI-14).
function lichessClock(game) {
    const speed = Lichess.speedName(game.lichessSpeed)
    if (game.lichessDays > 0)
        return speed + " · " + game.lichessDays + (game.lichessDays === 1 ? " day" : " days")
    if (game.clockLimitSec > 0 || game.clockIncSec > 0)
        return (speed ? speed + " " : "") + minutesText(game.clockLimitSec) + "+" + game.clockIncSec
    return speed
}

// The card's line under the title: a Chess960 game's position, a Lichess game's clock and Rated / Casual,
// the sides, the day, the moves.
function details(game) {
    var parts = []
    if (game.chess960)   // LI-01A: first, so a narrow card still shows it
        parts.push(game.startPosition >= 0 ? "Chess960 #" + game.startPosition : "Chess960")   // -1: its start can't be read
    if (game.kind === "lichess") {
        const clock = lichessClock(game)
        if (clock !== "")
            parts.push(clock)
        parts.push(game.lichessRated ? "Rated" : "Casual")
    }
    parts.push(game.kind === "computer" || game.kind === "lichess" ? "you play " + (game.playerPlaysWhite ? "White" : "Black")
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
    if (game.kind === "computer" || game.kind === "lichess")
        return whiteWon === game.playerPlaysWhite ? "You won" : "You lost"
    return whiteWon ? "White won" : "Black won"
}

// A rated game's change of the player's rating (step 5): "+12", "−8", "±0"; "" when not rated.
function ratingChangeText(game) {
    if (!game.rated)
        return ""
    return game.ratingChange > 0 ? "+" + game.ratingChange
         : game.ratingChange < 0 ? "\u2212" + (-game.ratingChange) : "\u00b10"
}

// A finished game's result with its rating change: "You won · +12".
function resultWithRating(game) {
    const change = ratingChangeText(game)
    return resultWord(game) + (change !== "" ? " · " + change : "")
}

// The player's rating vs the robot as Home and the Profile page show it: "1212", "1212?" while provisional.
function ratingText(rating, provisional) {
    return rating + (provisional ? "?" : "")
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
