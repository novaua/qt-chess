import QtQuick

// Lichess "opponentGone": the opponent left the game. Counts down until Lichess allows
// claiming the win, then enables "Claim victory".
Rectangle {
    id: banner

    signal claimVictoryRequested()

    property bool _active:      false
    property int  _secondsLeft: -1   // <= 0: the win can be claimed now

    // claimWinInSeconds as sent by Lichess; -1 if it didn't say (button enabled right away).
    function show(claimWinInSeconds) {
        _secondsLeft = claimWinInSeconds
        _active = true
    }
    function hide() {
        _active = false
        _secondsLeft = -1
    }

    visible: _active
    width: 420
    height: 44
    radius: 8
    color: "#7a3b00"

    Timer {
        interval: 1000
        repeat: true
        running: banner._active && banner._secondsLeft > 0
        onTriggered: banner._secondsLeft--
    }

    Row {
        anchors.centerIn: parent
        spacing: 10
        Text {
            text: banner._secondsLeft > 0
                ? "Opponent left the game. Claim victory in " + banner._secondsLeft + " s"
                : "Opponent left the game"
            color: "#ffffff"; font.pixelSize: 13
            anchors.verticalCenter: parent.verticalCenter
        }
        Button {
            text: "Claim victory"; implicitWidth: 100; implicitHeight: 28
            enabled: banner._secondsLeft <= 0
            onClicked: banner.claimVictoryRequested()
        }
    }
}
