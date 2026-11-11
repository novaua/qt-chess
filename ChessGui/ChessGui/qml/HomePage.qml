import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus

// Home: who is playing (account menu), Continue, and the ways to start a game. Each card opens a
// setup dialog or starts right away; ChessGame.qml owns the dialogs and answers the signals.
Page {
    id: page
    objectName: "home"

    signal computerGameRequested()
    signal twoPlayerGameRequested()
    signal onlineGameRequested()
    signal continueRequested()      // BL-17: the one unfinished game, or the list of them all (ChessGame.continueGames)
    signal settingsRequested()
    signal profileRequested()
    signal switchUserRequested()
    signal recentMessagesRequested(Item anchor)
    property int unseenMessages: 0   // the dot on the account button (BL-13 step 3)
    property bool continueBusy: false // Continue is rejoining the one Lichess game

    readonly property var _levels: ChessConnector.Levels

    // BL-17: every unfinished game, the robot's autosaved one and the Lichess ones (LI-04). Continue
    // opens the only one directly, else the list of them all.
    readonly property var _onlineGames: UserManager.lichessConnected ? LichessClient.ongoingGames : []
    readonly property int _myTurnCount: _onlineGames.filter(g => g.isMyTurn).length
    readonly property int _pendingCount: (ChessConnector.CanContinue ? 1 : 0) + _onlineGames.length
    readonly property string _continueText: {
        const n = _onlineGames.length
        const games = n + (n === 1 ? " Lichess game" : " Lichess games")
        const turn = _myTurnCount === 0 ? "waiting for " + (n === 1 ? "your opponent" : "your opponents")
                   : n === 1 ? "your turn" : "your turn in " + _myTurnCount
        if (ChessConnector.CanContinue)
            return ChessConnector.ContinueDescription + (n > 0 ? " · " + games + (_myTurnCount > 0 ? ", " + turn : "") : "")
        return n === 0 ? "" : games + " · " + turn
    }

    // A fresh list each time Home shows (back from a game, Settings, the profile): an opponent may
    // have moved or ended a game meanwhile.
    StackView.onActivated: {
        if (UserManager.lichessConnected && !LichessClient.ongoingGamesLoading)
            LichessClient.refreshOngoingGames()
    }

    // Transparent: the window's photo shows through (ChessGame.qml, BL-11).
    background: Item {}

    header: CommandBar {
        messageArea: barFill   // the window's short messages show over it
        ToolButton {
            id: accountButton
            text: UserManager.activeUserName
            Accessible.name: "Account: " + text + (page.unseenMessages > 0 ? ", " + page.unseenMessages + " new messages" : "")
            // Own content: the style tints the icon and draws the text in icon.color, and the
            // avatar must keep its colours.
            contentItem: Row {
                spacing: Theme.s2
                Image {
                    source: AvatarProvider.playerUrl
                    width: Theme.iconSizeCard; height: Theme.iconSizeCard
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                }
                Label {
                    text: accountButton.text
                    font: Theme.bodyStrong
                    anchors.verticalCenter: parent.verticalCenter
                }
                Label {   // the Lichess blitz rating, once linked
                    visible: text !== ""
                    text: UserManager.lichessConnected && UserManager.lichessRatings.blitz
                          ? "Lichess " + UserManager.lichessRatings.blitz.rating : ""
                    font: Theme.caption
                    color: Theme.textSecondary
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            ToolTip.visible: hovered
            ToolTip.text: "Profile and switch user"
            ToolTip.delay: 400
            onClicked: accountMenu.popup(accountButton, 0, accountButton.height)

            NewMessagesDot { count: page.unseenMessages }

            Menu {
                id: accountMenu
                MenuItem {
                    text: "Profile"
                    icon.source: Theme.icon("person")
                    onTriggered: page.profileRequested()
                }
                MenuItem {
                    text: "Switch user"
                    icon.source: Theme.icon("sign-out")
                    onTriggered: page.switchUserRequested()
                }
                MenuSeparator {}
                RecentMessagesMenuItem {
                    count: page.unseenMessages
                    onTriggered: page.recentMessagesRequested(accountButton)
                }
            }
        }

        Item { id: barFill; Layout.fillWidth: true }

        IconButton {
            icon.source: Theme.icon("settings")
            tip: "Settings (Ctrl+,)"
            onClicked: page.settingsRequested()
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: Math.max(height, content.implicitHeight + 2 * Theme.s5)
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: content
            width: Math.min(flick.width - 2 * Theme.s5, 600)
            x: (flick.width - width) / 2
            y: Math.max(Theme.s5, (flick.height - implicitHeight) / 2)   // centred while it fits
            spacing: Theme.s2

            Label {
                text: "Chess++"
                font: Theme.title
            }
            Label {
                text: "Hi " + UserManager.activeUserName + ", what would you like to play?"
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.s3
            }

            HomeCard {
                visible: page._pendingCount > 0
                enabled: !page.continueBusy
                text: page._pendingCount > 1 ? "Continue a game"
                    : ChessConnector.CanContinue ? "Continue game" : "Continue online game"
                description: page._continueText
                icon.source: Theme.icon("play")
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.s2
                onClicked: page.continueRequested()
            }

            GridLayout {
                columns: content.width >= 440 ? 2 : 1
                columnSpacing: Theme.s2
                rowSpacing: Theme.s2
                Layout.fillWidth: true

                HomeCard {
                    text: "Play vs computer"
                    shortcut: "Ctrl+N"
                    description: page._levels.length + " levels, from " + page._levels[0].name.toLowerCase()
                                 + " to " + page._levels[page._levels.length - 1].name.toLowerCase()
                    icon.source: Theme.icon("robot")
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                    onClicked: page.computerGameRequested()
                }
                HomeCard {
                    text: "Two players"
                    description: "Take turns on this computer"
                    icon.source: Theme.icon("two-players")
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                    onClicked: page.twoPlayerGameRequested()
                }
                HomeCard {
                    text: "Play online"
                    // BL-10: the challenges waiting for an answer are on the Online page.
                    description: UserManager.lichessConnected
                                 ? (LichessClient.incomingChallenges.length === 1 ? "1 challenge waiting for you"
                                    : LichessClient.incomingChallenges.length > 1 ? LichessClient.incomingChallenges.length + " challenges waiting for you"
                                    : "Challenge a friend on Lichess")
                               : UserManager.lichessNeedsReconnect ? "Lichess sign-in expired: reconnect in your profile"
                               : "Link your Lichess account in your profile first"
                    dimmed: !UserManager.lichessConnected
                    icon.source: Theme.icon("globe")
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                    onClicked: page.onlineGameRequested()
                }
                HomeCard {
                    text: "Open saved game"
                    shortcut: "Ctrl+O"
                    description: ChessConnector.CanLoad ? "Replay the game you saved"
                                                        : "No saved game yet: use Save during a game"
                    dimmed: !ChessConnector.CanLoad
                    icon.source: Theme.icon("folder-open")
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                    onClicked: ChessConnector.loadGame()
                }
            }
        }
    }
}
