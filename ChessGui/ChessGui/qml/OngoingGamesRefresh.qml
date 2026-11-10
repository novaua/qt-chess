import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus

// Refresh for the unfinished Lichess games (LichessClient.ongoingGames), with a ring while they load:
// on the Online page and in the Continue list (BL-17). `showError`: the last refresh's error beside it
// (where nothing else says it).
RowLayout {
    property bool showError: false

    spacing: Theme.s2

    function refresh() {
        if (UserManager.lichessConnected && !LichessClient.ongoingGamesLoading)
            LichessClient.refreshOngoingGames()
    }

    Button {
        text: "Refresh"
        flat: true
        enabled: !LichessClient.ongoingGamesLoading
        onClicked: parent.refresh()
    }
    BusyIndicator {
        visible: LichessClient.ongoingGamesLoading
        running: visible
        Layout.preferredWidth: 20; Layout.preferredHeight: 20
    }
    Label {
        visible: parent.showError && LichessClient.ongoingGamesError !== ""
        text: "Couldn't load your Lichess games: " + LichessClient.ongoingGamesError
        color: Theme.critical
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
