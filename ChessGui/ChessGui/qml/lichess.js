.pragma library

// Lichess's words for a game, shared by the Online page and the Continue list (BL-17).

function speedName(speed) {
    return ({ ultraBullet: "UltraBullet", bullet: "Bullet", blitz: "Blitz", rapid: "Rapid",
              classical: "Classical", correspondence: "Correspondence" })[speed] ?? speed
}

// An unfinished game's card line (LichessClient.ongoingGames): the board shows our side, this the rest.
function gameDetails(game) {
    var parts = [speedName(game.speed)]
    if (game.variant !== "standard")
        parts.push(game.variant)
    if (game.timeLeftText !== "")
        parts.push(game.timeLeftText + " left")
    return parts.join(" · ")
}

// An unfinished game's card title: the opponent, with the rating.
function gameTitle(game) {
    return game.opponentName + (game.opponentRating > 0 ? " (" + game.opponentRating + ")" : "")
}
