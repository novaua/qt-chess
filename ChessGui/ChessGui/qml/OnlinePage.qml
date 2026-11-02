import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus

// The ways to play on Lichess (LI-04, docs/LI-04-RejoinPlan.md): the player's unfinished games
// (Continue rejoins one), a new challenge, joining by link. Pushed over Home by "Play online"; Back
// pops it. A game that starts replaces it by the game page (ChessGame.qml route()); the challenge
// waiting for the friend is ChessGame.qml's ChallengeDialog, over this page.
Page {
    id: page
    objectName: "online"

    signal backRequested()
    signal profileRequested()   // link or reconnect the Lichess account there

    property bool _busy: false   // a challenge, a join or a rejoin is under way

    readonly property var _games: LichessClient.ongoingGames
    readonly property int _myTurnCount: _games.filter(g => g.isMyTurn).length

    // Not while one is running: a new token (main.cpp) has started one already.
    function _refresh() {
        if (UserManager.lichessConnected && !LichessClient.ongoingGamesLoading)
            LichessClient.refreshOngoingGames()
    }
    // A join, rejoin or challenge that is starting is dropped, so no game starts unexpectedly. Not
    // when a game did start: route() replaces this page by the game page then.
    function _dropStarting() {
        if (_busy && !ChessConnector.InGame)
            LichessClient.stopStream()
        _busy = false
    }
    // Pushed, or shown again (back from the Profile page). Nothing started from here is under way
    // any more: a game replaces the page, a failure says so (networkError).
    StackView.onActivated: {
        _busy = false
        _refresh()
    }
    // Linked, reconnected or signed out (expired) while the page is open, e.g. the browser sign-in
    // ended after Back from the Profile page: what was starting belonged to the other account. The
    // list is refreshed by main.cpp, which sees the new token. (UserManager.lichessChanged also
    // fires for a ratings update: not that.)
    readonly property bool _connected: UserManager.lichessConnected
    on_ConnectedChanged: _dropStarting()
    // Back while something is starting: drop it.
    StackView.onRemoved: _dropStarting()

    function _speedName(speed) {
        return ({ ultraBullet: "UltraBullet", bullet: "Bullet", blitz: "Blitz", rapid: "Rapid",
                  classical: "Classical", correspondence: "Correspondence" })[speed] ?? speed
    }
    // The card's board shows our side; this line the rest.
    function _gameDetails(game) {
        var parts = [_speedName(game.speed)]
        if (game.variant !== "standard")
            parts.push(game.variant)
        if (game.timeLeftText !== "")
            parts.push(game.timeLeftText + " left")
        return parts.join(" · ")
    }
    // Continue (rejoin), or lichess.org for a game the Board API can't play here.
    function _openGame(game) {
        if (game.canPlayHere)
            _start(() => LichessClient.rejoinGame(game.gameId))
        else
            Qt.openUrlExternally("https://lichess.org/" + game.gameId)
    }
    function _start(action) {
        errorBar.visible = false
        page._busy = true
        action()
    }

    Connections {
        target: LichessClient
        function onChallengeCreated(gameId, joinUrl) { page._busy = false }
        function onGameStarted(gameId, playingAsWhite, opponentName, opponentAvatarUrl) { page._busy = false }
        function onNetworkError(message) {
            page._busy = false
            errorBar.show(message, "critical")
        }
    }

    background: Rectangle { color: Theme.windowBg }

    header: CommandBar {
        IconButton {
            icon.source: Theme.icon("back")
            tip: "Back (Esc)"
            onClicked: page.backRequested()
        }
        Label {
            text: "Play online"
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
            width: Math.min(flick.width - 2 * Theme.s5, 640)
            x: (flick.width - width) / 2
            y: Theme.s5
            spacing: Theme.s3

            // ── Not connected: link the account first ──
            SectionCard {
                visible: !UserManager.lichessConnected
                title: "Lichess account"
                subtitle: UserManager.lichessNeedsReconnect
                          ? "The Lichess sign-in of " + UserManager.lichessUsername + " expired. Reconnect it in your profile to play online."
                          : "Link your Lichess account in your profile to play online."
                Layout.fillWidth: true

                Button {
                    text: "Open profile"
                    icon.source: Theme.icon("person")
                    highlighted: true
                    onClicked: page.profileRequested()
                }
            }

            // ── Your games: the unfinished ones, most urgent first ──
            SectionCard {
                id: gamesCard
                visible: UserManager.lichessConnected
                title: "Your games"
                subtitle: LichessClient.ongoingGamesError !== "" ? "Couldn't load your games: " + LichessClient.ongoingGamesError
                        : page._games.length === 0 ? (LichessClient.ongoingGamesLoading ? "Loading…"
                              : "No unfinished games. A game you leave before it ends waits here, and so does a correspondence game.")
                        : page._games.length + (page._games.length === 1 ? " game" : " games")
                          + (page._myTurnCount > 0 ? " · your turn in " + page._myTurnCount : "")
                Layout.fillWidth: true

                // One card per game, like Lichess's "current games": the position from our side,
                // then who, whose turn and Continue. A click on the board continues too.
                GridLayout {
                    id: gamesGrid
                    // From the card's inside, not from the layout's own width, which isn't known yet
                    // when the cards ask.
                    readonly property real innerWidth: gamesCard.contentWidth
                    readonly property real cardWidth: (innerWidth - (columns - 1) * columnSpacing) / columns
                    visible: page._games.length > 0
                    columns: innerWidth >= 440 ? 2 : 1
                    columnSpacing: Theme.s3
                    rowSpacing: Theme.s3
                    Layout.fillWidth: true

                    Repeater {
                        model: page._games
                        delegate: Rectangle {
                            id: gameCard
                            required property var modelData
                            Layout.preferredWidth: gamesGrid.cardWidth
                            implicitHeight: cardColumn.implicitHeight + 2 * Theme.s3
                            radius: Theme.radiusCard
                            color: Theme.layerBg
                            border.color: Theme.cardStroke
                            Accessible.role: Accessible.ListItem
                            Accessible.name: modelData.opponentName + ", " + page._gameDetails(modelData)
                                             + ", you play " + (modelData.color === "white" ? "White" : "Black")
                                             + (modelData.isMyTurn ? ", your turn" : ", waiting")

                            ColumnLayout {
                                id: cardColumn
                                anchors { left: parent.left; right: parent.right; top: parent.top; margins: Theme.s3 }
                                spacing: Theme.s2

                                BoardPreview {
                                    fen: gameCard.modelData.fen
                                    flipped: gameCard.modelData.color === "black"
                                    lastMove: gameCard.modelData.lastMove
                                    Layout.preferredWidth: gamesGrid.cardWidth - 2 * Theme.s3
                                    Layout.preferredHeight: gamesGrid.cardWidth - 2 * Theme.s3

                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: !page._busy
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: page._openGame(gameCard.modelData)
                                    }
                                }
                                RowLayout {
                                    spacing: Theme.s2
                                    Layout.fillWidth: true
                                    Label {
                                        text: gameCard.modelData.opponentName
                                              + (gameCard.modelData.opponentRating > 0 ? " (" + gameCard.modelData.opponentRating + ")" : "")
                                        font: Theme.bodyStrong
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: gameCard.modelData.isMyTurn ? "Your turn" : "Waiting"
                                        font: gameCard.modelData.isMyTurn ? Theme.bodyStrong : Theme.caption
                                        color: gameCard.modelData.isMyTurn ? Theme.accent : Theme.textSecondary
                                    }
                                }
                                Label {
                                    text: page._gameDetails(gameCard.modelData)
                                    font: Theme.caption
                                    color: Theme.textSecondary
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Button {
                                    text: gameCard.modelData.canPlayHere ? "Continue" : "Open on lichess.org"
                                    icon.source: Theme.icon(gameCard.modelData.canPlayHere ? "play" : "link")
                                    highlighted: gameCard.modelData.canPlayHere && gameCard.modelData.isMyTurn
                                    enabled: !page._busy
                                    Layout.fillWidth: true
                                    ToolTip.visible: hovered && !gameCard.modelData.canPlayHere
                                    ToolTip.text: "This kind of game can't be played in Chess++ yet"
                                    ToolTip.delay: 400
                                    onClicked: page._openGame(gameCard.modelData)
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: Theme.s2
                    Button {
                        text: "Refresh"
                        flat: true
                        enabled: !LichessClient.ongoingGamesLoading
                        onClicked: page._refresh()
                    }
                    BusyIndicator {
                        visible: LichessClient.ongoingGamesLoading
                        running: visible
                        Layout.preferredWidth: 20; Layout.preferredHeight: 20
                    }
                }
            }

            // ── New game: a challenge with a link for the friend ──
            SectionCard {
                visible: UserManager.lichessConnected
                title: "Challenge a friend"
                subtitle: "You get a link to send. The game starts when your friend opens it."
                Layout.fillWidth: true

                Label { text: "Time control"; color: Theme.textSecondary; font: Theme.caption }
                ChoiceRow {
                    id: tcType
                    options: [{ label: "Unlimited" }, { label: "Real time" }, { label: "Days per move" }]
                }
                // Real time: minutes + increment. Rapid and Classical only: an open challenge (a
                // link) can't be Bullet or Blitz for a Board API player such as this app (LI-02).
                ChoiceRow {
                    id: tcPreset
                    visible: tcType.checkedIndex === 1
                    checkedIndex: 2
                    options: [{ label: "10+0",  min: 10, inc: 0 },
                              { label: "10+5",  min: 10, inc: 5 },
                              { label: "15+10", min: 15, inc: 10 },
                              { label: "30+0",  min: 30, inc: 0 },
                              { label: "30+20", min: 30, inc: 20 }]
                }
                // Correspondence: the days each player has for a move (Lichess's choices). Short
                // labels, so the seven fit a narrow window.
                RowLayout {
                    visible: tcType.checkedIndex === 2
                    spacing: Theme.s2
                    ChoiceRow {
                        id: tcDays
                        checkedIndex: 2
                        options: [1, 2, 3, 5, 7, 10, 14].map(d => ({ label: String(d), days: d }))
                    }
                    Label {
                        text: tcDays.checkedIndex === 0 ? "day per move" : "days per move"
                        color: Theme.textSecondary
                    }
                }

                Label { text: "Side"; color: Theme.textSecondary; font: Theme.caption }
                ChoiceRow {
                    id: side
                    options: [{ label: "Random", value: "random" },
                              { label: "White",  value: "white"  },
                              { label: "Black",  value: "black"  }]
                }

                Button {
                    text: "Create challenge"
                    icon.source: Theme.icon("link")
                    highlighted: true
                    enabled: !page._busy
                    Layout.topMargin: Theme.s1
                    onClicked: {
                        var preset = tcType.checkedIndex === 1 ? tcPreset.options[tcPreset.checkedIndex] : { min: 0, inc: 0 }
                        var days = tcType.checkedIndex === 2 ? tcDays.options[tcDays.checkedIndex].days : 0
                        // Standard only: Chess960 isn't playable here yet (LI-01).
                        page._start(() => LichessClient.createOpenChallenge(preset.min, preset.inc,
                                                                            side.options[side.checkedIndex].value,
                                                                            "standard", days))
                    }
                }
            }

            // ── Join by link or id: the fallback ──
            SectionCard {
                visible: UserManager.lichessConnected
                title: "Join a game"
                subtitle: "Paste the link a friend sent you, or a game's id."
                Layout.fillWidth: true

                RowLayout {
                    spacing: Theme.s2
                    Layout.fillWidth: true

                    TextField {
                        id: joinUrlField
                        placeholderText: "Lichess link or game ID"
                        Layout.fillWidth: true
                        onAccepted: joinButton.clicked()
                    }
                    Button {
                        id: joinButton
                        text: "Join"
                        enabled: joinUrlField.text.trim() !== "" && !page._busy
                        onClicked: {
                            if (!enabled)
                                return
                            errorBar.visible = false
                            if (LichessClient.joinFromUrl(joinUrlField.text.trim()))
                                page._busy = true
                            else
                                errorBar.show("That isn't a Lichess link or game ID", "critical")
                        }
                    }
                }
            }
        }
    }

    // Over the content, under the header: seen wherever the page is scrolled to.
    InfoBar {
        id: errorBar
        anchors { top: parent.top; topMargin: Theme.s2; horizontalCenter: parent.horizontalCenter }
        width: content.width
        z: 1
    }
}
