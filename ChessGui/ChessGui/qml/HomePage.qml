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
    signal settingsRequested()
    signal profileRequested()
    signal switchUserRequested()

    readonly property var _levels: ChessConnector.Levels

    background: Rectangle {
        color: Theme.windowBg

        // The glass-pieces photo, faint, on Home only.
        Image {
            anchors.fill: parent
            source: "qrc:/app/pics/ChessBackground.jpg"
            fillMode: Image.PreserveAspectCrop
            opacity: Theme.homeBackdropOpacity
        }
    }

    header: CommandBar {
        ToolButton {
            id: accountButton
            text: UserManager.activeUserName
            Accessible.name: "Account: " + text
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
            }
        }

        Item { Layout.fillWidth: true }

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
                visible: ChessConnector.CanContinue
                text: "Continue game"
                description: ChessConnector.ContinueDescription
                icon.source: Theme.icon("play")
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.s2
                onClicked: ChessConnector.continueGame()
            }

            GridLayout {
                columns: content.width >= 440 ? 2 : 1
                columnSpacing: Theme.s2
                rowSpacing: Theme.s2
                Layout.fillWidth: true

                HomeCard {
                    text: "Play vs computer"
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
                    description: UserManager.lichessConnected ? "Challenge a friend on Lichess"
                               : UserManager.lichessNeedsReconnect ? "Lichess sign-in expired: reconnect in your profile"
                               : "Link your Lichess account in your profile first"
                    dimmed: !UserManager.lichessConnected
                    icon.source: Theme.icon("globe")
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                    onClicked: page.onlineGameRequested()
                }
                HomeCard {
                    text: "Open saved game"
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
