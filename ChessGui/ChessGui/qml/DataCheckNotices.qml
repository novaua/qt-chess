import QtQuick
import ChessPlusPlus

// BL-20's notices (BL-35): kept games that failed their check (edited by hand, damaged) were set aside when
// the profile loaded, or the rating (rating.json) or the statistics (users.json) failed theirs and were
// restored. Said once per load, as short messages that count as new (the dot). One instance in the window,
// `dataCheckNotices`: the first profile loads before the window, so the window asks tellAll() once it is
// up; later loads come through the Connections.
Item {
    id: notices
    visible: false

    signal message(string text, string severity, bool external)

    function tellAll() {
        notices._tellSetAside(GameLibrary.setAsideCount)
        notices._tellRatingRestored()
        notices._tellStatisticsRestored()
    }

    function _tellSetAside(count) {
        if (count > 0)
            notices.message((count === 1 ? "1 kept game couldn't be verified and was"
                                         : count + " kept games couldn't be verified and were")
                            + " set aside.", "caution", true)
    }
    function _tellRatingRestored() {
        if (GameLibrary.ratingRestored)
            notices.message("Your rating couldn't be verified and was "
                            + (GameLibrary.ratedGames > 0 ? "restored from your kept games." : "started again."),
                            "caution", true)
    }
    function _tellStatisticsRestored() {
        for (const s of UserManager.takeRestoredStatistics())
            notices.message(s.name + "'s statistics couldn't be verified and were "
                            + (s.rebuilt ? "rebuilt from the kept games (Lichess and older games aren't in them)."
                                         : "restored from a backup."), "caution", true)
    }

    Connections {
        target: GameLibrary
        function onGamesSetAside(count) { notices._tellSetAside(count) }
        function onRatingRestoredOnLoad() { notices._tellRatingRestored() }
    }
    Connections {
        target: UserManager
        function onStatisticsRestored() { notices._tellStatisticsRestored() }
    }
}
