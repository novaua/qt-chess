import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus
import "lichess.js" as Lichess

// The ways to play on Lichess (LI-04, docs/LI-04-RejoinPlan.md): the challenges to the player
// (BL-10), a new challenge, joining by link, and last the player's unfinished games (Continue rejoins
// one; BL-17: also in Home's Continue list). Pushed over Home by "Play online"; Back
// pops it. A game that starts replaces it by the game page (ChessGame.qml route()); the challenge
// waiting for the friend is ChessGame.qml's ChallengeDialog, over this page.
Page {
    id: page
    objectName: "online"

    signal backRequested()
    signal profileRequested()   // link or reconnect the Lichess account there

    // What is under way (BL-16: its button says so): "challenge" or "join"; "" when nothing is. An
    // Accept: ChessConnector.AcceptingChallengeId (shared with the challenge bar).
    property string _busyAction: ""
    readonly property bool _busy: _busyAction !== ""
    // That, or a rejoin (ChessConnector.continueOnlineGame): no second start meanwhile.
    readonly property bool _starting: _busy || ChessConnector.StartingOnlineGame || ChessConnector.AcceptingChallengeId !== ""

    readonly property string _opponent: opponentField.text.trim()   // empty: a challenge with a link
    readonly property bool _opponentValid: _opponent === "" || LichessClient.isUsername(_opponent)
    readonly property bool _named: _opponent !== "" && _opponentValid
    readonly property bool _realTime: tcType.checkedIndex === 1
    // What the chosen time control is on Lichess, and the player's rating there.
    readonly property string _mode: _realTime ? tcClock.speed : tcType.checkedIndex === 2 ? "Correspondence" : ""
    readonly property var _perf: UserManager.lichessRatings[_mode.toLowerCase()]

    readonly property var _games: LichessClient.ongoingGames
    readonly property int _myTurnCount: _games.filter(g => g.isMyTurn).length
    readonly property var _challenges: LichessClient.incomingChallenges

    // BL-10: one line under the challenger's name.
    function _challengeDetails(c) {
        const parts = [c.timeControlText, Lichess.speedName(c.speed), c.rated ? "Rated" : "Casual", c.sideText]
        if (c.variant !== "standard")
            parts.push(c.variant)
        if (c.rematchOf !== "")
            parts.push("Rematch")
        return parts.filter(p => p !== "").join(" · ")
    }
    // The reasons Lichess shows the challenger (LichessClient::declineChallenge).
    readonly property var _declineReasons: [
        { text: "Decline",                     reason: "generic" },
        { text: "Not now",                     reason: "later" },
        { text: "Too fast for me",             reason: "tooFast" },
        { text: "Too slow for me",             reason: "tooSlow" },
        { text: "Not this time control",       reason: "timeControl" },
        { text: "Casual games only",           reason: "casual" },
        { text: "Rated games only",            reason: "rated" },
        { text: "Standard chess only",         reason: "standard" }
    ]

    // A join or challenge that is starting is dropped, so no game starts unexpectedly. Not when a
    // game did start: route() replaces this page by the game page then. A rejoin: ChessConnector.
    function _dropStarting() {
        if (_busy && !ChessConnector.InGame)
            LichessClient.stopStream()
        _busyAction = ""
        ChessConnector.cancelOnlineStart()
    }
    // Pushed, or shown again (back from the Profile page). Nothing started from here is under way
    // any more: a game replaces the page, a failure says so (networkError).
    StackView.onActivated: {
        _busyAction = ""
        gamesRefresh.refresh()   // not while one is running: a new token (main.cpp) has started one already
    }
    // Linked, reconnected or signed out (expired) while the page is open, e.g. the browser sign-in
    // ended after Back from the Profile page: what was starting belonged to the other account. The
    // list is refreshed by main.cpp, which sees the new token. (UserManager.lichessChanged also
    // fires for a ratings update: not that.)
    readonly property bool _connected: UserManager.lichessConnected
    on_ConnectedChanged: _dropStarting()
    // Back while something is starting: drop it.
    StackView.onRemoved: _dropStarting()

    function _start(action) {
        errorBar.visible = false
        page._busyAction = "challenge"
        action()
    }
    function _challenge() {
        const limitSec = _realTime ? tcClock.limitSec : 0
        const incSec = _realTime ? tcClock.incSec : 0
        const days = tcType.checkedIndex === 2 ? tcDays.options[tcDays.checkedIndex].days : 0
        const color = side.options[side.checkedIndex].value
        // Standard only: Chess960 isn't playable here yet (LI-01).
        if (_opponent !== "")
            _start(() => LichessClient.challengePlayer(_opponent, limitSec, incSec, days, color))
        else
            _start(() => LichessClient.createOpenChallenge(limitSec / 60, incSec, color, "standard", days))
    }

    Connections {
        target: LichessClient
        function onChallengeCreated(gameId, joinUrl) { page._busyAction = "" }
        function onDirectChallengeSent(challengeId, opponentName) { page._busyAction = "" }
        function onDirectChallengeLeft(opponentName) { page._busyAction = "" }
        function onChallengeDeclined(opponentName) { page._busyAction = "" }
        function onChallengeCanceled() { page._busyAction = "" }
        function onGameStarted(gameId, playingAsWhite, opponentName, opponentAvatarUrl) { page._busyAction = "" }
        function onNetworkError(message) {
            page._busyAction = ""
            errorBar.show(message, "critical")
        }
        function onChallengeAcceptFailed(challengeId, message) {
            page._busyAction = ""
            errorBar.show("Couldn't accept the challenge: " + message, "critical")
        }
    }

    background: Rectangle { color: Theme.windowBg }

    header: CommandBar {
        messageArea: barFill   // the window's short messages show over it
        IconButton {
            icon.source: Theme.icon("back")
            tip: "Back (Esc)"
            onClicked: page.backRequested()
        }
        Label {
            text: "Play online"
            font: Theme.bodyStrong
            id: barFill
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

            // ── BL-10: the challenges to the player, oldest first (the window's bar shows only one,
            // and not over this page) ──
            SectionCard {
                id: challengesCard
                objectName: "challengesCard"
                visible: UserManager.lichessConnected && page._challenges.length > 0
                title: "Challenges to you"
                subtitle: page._challenges.length === 1 ? "1 player is waiting for your answer."
                                                        : page._challenges.length + " players are waiting for your answer."
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: Theme.s2
                    Layout.fillWidth: true

                    Repeater {
                        model: page._challenges
                        delegate: Rectangle {
                            id: challengeRow
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: challengeColumn.implicitHeight + 2 * Theme.s3
                            radius: Theme.radiusCard
                            color: Theme.layerBg
                            border.color: Theme.cardStroke
                            Accessible.role: Accessible.ListItem
                            Accessible.name: modelData.challengerName + ", " + page._challengeDetails(modelData)

                            ColumnLayout {
                                id: challengeColumn
                                anchors { left: parent.left; right: parent.right; top: parent.top; margins: Theme.s3 }
                                spacing: Theme.s1

                                Label {
                                    text: challengeRow.modelData.challengerText
                                    font: Theme.bodyStrong
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: page._challengeDetails(challengeRow.modelData)
                                          + (challengeRow.modelData.canPlayHere ? "" : " · Can't be played in Chess++ yet")
                                    font: Theme.caption
                                    color: Theme.textSecondary
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                                RowLayout {
                                    spacing: Theme.s2
                                    Layout.topMargin: Theme.s1
                                    BusyButton {
                                        idleText: challengeRow.modelData.canPlayHere ? "Accept" : "Open on lichess.org"
                                        idleIcon: Theme.icon(challengeRow.modelData.canPlayHere ? "play" : "link")
                                        busy: ChessConnector.AcceptingChallengeId === challengeRow.modelData.id
                                        busyText: "Accepting…"
                                        highlighted: challengeRow.modelData.canPlayHere
                                        enabled: !page._starting
                                        onClicked: {
                                            if (!challengeRow.modelData.canPlayHere) {
                                                Qt.openUrlExternally(challengeRow.modelData.url)
                                                return
                                            }
                                            errorBar.visible = false
                                            ChessConnector.acceptChallenge(challengeRow.modelData.id)
                                        }
                                    }
                                    Button {
                                        id: declineButton
                                        objectName: "declineButton"   // the online-challenges-decline scene
                                        text: "Decline…"
                                        enabled: !page._starting
                                        onClicked: declineMenu.open()
                                        Menu {
                                            id: declineMenu
                                            y: declineButton.height
                                            Repeater {
                                                model: page._declineReasons
                                                delegate: MenuItem {
                                                    required property var modelData
                                                    text: modelData.text
                                                    onTriggered: LichessClient.declineChallenge(challengeRow.modelData.id, modelData.reason)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── New game: a challenge with a link for the friend, or to a named player ──
            SectionCard {
                visible: UserManager.lichessConnected
                title: "Challenge a friend"
                subtitle: "You get a link to send, or name a Lichess player to challenge them directly."
                Layout.fillWidth: true

                Label { text: "Time control"; color: Theme.textSecondary; font: Theme.caption }
                ChoiceRow {
                    id: tcType
                    checkedIndex: 1
                    options: [{ label: "Unlimited" }, { label: "Real time" }, { label: "Days per move" }]
                }
                // Real time: an open challenge (a link) can't be Blitz for a Board API player such as
                // this app, a challenge to a named player can. Bullet is never possible (LI-02).
                TimeControlPicker {
                    id: tcClock
                    objectName: "challengeClock"
                    visible: page._realTime
                    online: true
                    blitzAllowed: page._opponent !== ""
                    limitSec: 900
                    incSec: 10
                    Layout.fillWidth: true
                }
                Label {
                    visible: page._realTime && !tcClock.blitzAllowed
                    text: "Blitz needs a Lichess username: Lichess allows it only in a direct challenge."
                    color: Theme.textSecondary
                    font: Theme.caption
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
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
                Label {
                    visible: page._perf !== undefined
                    text: page._perf ? "Your " + page._mode + " rating: " + page._perf.rating + (page._perf.prov ? "?" : "") : ""
                    font: Theme.bodyStrong
                }

                Label { text: "Lichess username (optional)"; color: Theme.textSecondary; font: Theme.caption }
                TextField {
                    id: opponentField
                    objectName: "challengeOpponent"
                    placeholderText: "Leave empty to get a link"
                    maximumLength: 30
                    Layout.fillWidth: true
                    onAccepted: if (challengeButton.enabled) challengeButton.clicked()
                }
                Label {
                    visible: !page._opponentValid
                    text: "That isn't a Lichess username: 2 to 30 letters, digits, - or _."
                    color: Theme.critical
                    font: Theme.caption
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Label { text: "Side"; color: Theme.textSecondary; font: Theme.caption }
                ChoiceRow {
                    id: side
                    options: [{ label: "Random", value: "random" },
                              { label: "White",  value: "white"  },
                              { label: "Black",  value: "black"  }]
                }

                BusyButton {
                    id: challengeButton
                    idleText: page._named ? "Challenge " + page._opponent : "Create challenge"
                    idleIcon: page._named ? "" : Theme.icon("link")
                    busy: page._busyAction === "challenge"
                    busyText: page._named ? "Sending…" : "Creating…"
                    highlighted: true
                    enabled: !page._starting && page._opponentValid && (!page._realTime || tcClock.ready)
                    Layout.topMargin: Theme.s1
                    onClicked: page._challenge()
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
                    BusyButton {
                        id: joinButton
                        idleText: "Join"
                        busy: page._busyAction === "join"
                        busyText: "Joining…"
                        enabled: joinUrlField.text.trim() !== "" && !page._starting
                        onClicked: {
                            if (!enabled)
                                return
                            errorBar.visible = false
                            if (LichessClient.joinFromUrl(joinUrlField.text.trim()))
                                page._busyAction = "join"
                            else
                                errorBar.show("That isn't a Lichess link or game ID", "critical")
                        }
                    }
                }
            }

            // ── Your games: the unfinished ones, last (BL-17: the player came for a new game; Home's
            // Continue lists them too) ──
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

                // One card per game (GameCard), most urgent first.
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
                        delegate: OngoingGameCard {
                            required property var modelData
                            game: modelData
                            busy: page._starting
                            Layout.preferredWidth: gamesGrid.cardWidth
                        }
                    }
                }

                OngoingGamesRefresh { id: gamesRefresh }   // the error is in the subtitle
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
