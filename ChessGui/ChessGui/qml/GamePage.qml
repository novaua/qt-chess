import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus

// The game: command bar, the board between the two players' captured pieces and, in a wide window,
// the move list and result beside it. ChessGame.qml owns the dialogs and answers the signals.
Page {
    id: page
    objectName: "game"

    property bool rematchPending: false   // a Lichess rematch challenge waits for the opponent

    signal homeRequested()
    signal resignRequested()
    signal levelsHelpRequested()
    signal settingsRequested()
    signal profileRequested()
    signal switchUserRequested()

    readonly property string mode: ChessConnector.GameMode
    readonly property bool gameOver: ChessConnector.GameResult !== ""
    property bool _resultShown: false       // a moment after the game ends, so the last move is seen first
    property bool _resultCardClosed: false

    function flipBoard() { chessBoard.angle = chessBoard.angle === 0 ? 180 : 0 }
    function _facePlayer() { chessBoard.angle = ChessConnector.PlayerPlaysWhite ? 0 : 180 }

    Component.onCompleted: {
        _facePlayer()
        _resultShown = gameOver
    }

    Connections {
        target: ChessConnector
        function onNewGameStarted()          { page._facePlayer() }
        function onPlayerPlaysWhiteChanged() { page._facePlayer() }
        // Undo after mate / resign vs Robot resumes the game: the result goes away again.
        function onGameResultChanged() {
            if (ChessConnector.GameResult !== "") {
                page._resultCardClosed = false
                resultTimer.restart()
            } else {
                resultTimer.stop()
                page._resultShown = false
            }
        }
    }

    Timer {
        id: resultTimer
        interval: 1600
        onTriggered: page._resultShown = true
    }

    background: Rectangle { color: Theme.windowBg }

    header: CommandBar {
        IconButton {
            icon.source: Theme.icon("home")
            tip: "Home (Esc)"
            onClicked: page.homeRequested()
        }
        Rectangle {
            Layout.preferredWidth: 1; Layout.preferredHeight: 24
            color: Theme.divider
        }
        Label {
            text: ChessConnector.GameTitle
            font: Theme.bodyStrong
            elide: Text.ElideRight
            Layout.fillWidth: true
            Layout.leftMargin: Theme.s2
        }

        IconButton {
            visible: page.mode === "replay"
            icon.source: Theme.icon("prev"); tip: "Previous move (←)"
            onClicked: ChessConnector.movePrev()
        }
        IconButton {
            visible: page.mode === "replay"
            icon.source: Theme.icon("next"); tip: "Next move (→)"
            onClicked: ChessConnector.moveNext()
        }
        IconButton {
            // vs Robot this also takes back the last moves of a finished game, which resumes it.
            visible: page.mode === "computer" || (page.mode === "online" && !page.gameOver)
            icon.source: Theme.icon("undo")
            tip: page.mode === "online" ? "Propose takeback (Ctrl+Z)" : "Undo move (Ctrl+Z)"
            onClicked: ChessConnector.requestTakeback()
        }
        IconButton {
            icon.source: Theme.icon("flip-board"); tip: "Flip board (F)"
            onClicked: page.flipBoard()
        }
        IconButton {
            visible: page.mode === "computer" || page.mode === "local"
            icon.source: Theme.icon("save"); tip: "Save game (Ctrl+S)"
            onClicked: ChessConnector.saveGame()
        }
        IconButton {
            visible: page.mode === "online" && !page.gameOver
            icon.source: Theme.icon("draw"); tip: "Offer draw"
            onClicked: ChessConnector.offerDraw()
        }
        IconButton {
            visible: page.mode !== "replay" && !page.gameOver
            icon.source: Theme.icon("flag"); tip: "Resign"
            onClicked: page.resignRequested()
        }
        IconButton {
            id: moreButton
            icon.source: Theme.icon("more"); tip: "More"
            onClicked: moreMenu.popup(moreButton, moreButton.width - moreMenu.width, moreButton.height)

            Menu {
                id: moreMenu
                MenuItem {
                    text: "Computer levels"
                    icon.source: Theme.icon("robot")
                    onTriggered: page.levelsHelpRequested()
                }
                MenuSeparator {}
                MenuItem {
                    text: "Settings"
                    icon.source: Theme.icon("settings")
                    onTriggered: page.settingsRequested()
                }
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
    }

    Item {
        id: area
        anchors.fill: parent

        // Worked out from the area, not from the board, to avoid a binding loop
        // (board size → panel height → board size):
        //   portrait (width-limited):   panelH = width * 0.95 / 16
        //   landscape (height-limited): panelH = (height - 2 * panelH) * 0.95 / 16 = height * 0.95 / 17.9
        readonly property real panelH: width * 1.11875 < height ? width * 0.95 / 16 : height * 0.95 / 17.9
        readonly property real boardSize: Math.min(width, height - 2 * panelH) * 0.95
        readonly property real sidePanelW: 160
        readonly property bool showSidePanels: page.width > page.height * 1.15

        Column {
            anchors.fill: parent

            CapturedPanel {
                width: area.boardSize
                height: area.panelH
                anchors.horizontalCenter: parent.horizontalCenter
                avatarUrl: AvatarProvider.opponentUrl
                // The robot only plays in computer games.
                avatarTip: {
                    if (page.mode !== "computer") return ""
                    var info = ChessConnector.Levels[ChessConnector.LastLevel - 1]
                    return "Robot · Level " + info.level + " — " + info.name + "\n"
                        + info.rating + "\n"
                        + "Click for level info"
                }
                onAvatarClicked: page.levelsHelpRequested()
                pieces: ChessConnector.PlayerPlaysWhite ? ChessConnector.CapturedByDark : ChessConnector.CapturedByLight
                isActive: (ChessConnector.IsWhiteMove === 1) !== ChessConnector.PlayerPlaysWhite
            }

            Item {
                width: parent.width
                height: area.height - 2 * area.panelH

                ChessBoard {
                    id: chessBoard
                    anchors.fill: parent
                }

                // The robot is thinking: no clicks on the board.
                MouseArea {
                    anchors.fill: parent
                    visible: ChessConnector.EngineThinking
                    cursorShape: Qt.WaitCursor
                }

                MoveHistoryPanel {
                    visible: area.showSidePanels
                    width: area.sidePanelW
                    height: area.boardSize
                    rematchPending: page.rematchPending
                    anchors {
                        verticalCenter: parent.verticalCenter
                        left: parent.left; leftMargin: (parent.width + area.boardSize) / 2 + 5
                    }
                }

                GameResultDialog {
                    visible: page._resultShown && area.showSidePanels
                    width: area.sidePanelW
                    anchors {
                        verticalCenter: parent.verticalCenter
                        right: parent.horizontalCenter
                        rightMargin: area.boardSize / 2 + 5
                    }
                }

                // Narrow / portrait window: no side panels, so the result (and Rematch) shows on
                // the board itself.
                GameResultCard {
                    visible: page._resultShown && !area.showSidePanels && !page._resultCardClosed
                    anchors.centerIn: parent
                    width: Math.min(area.boardSize * 0.8, 320)
                    z: 5
                    resultText: ChessConnector.ResultText
                    canRematch: page.gameOver && ChessConnector.CanRematch && !page.rematchPending
                    onRematchRequested: ChessConnector.rematch()
                    onCloseRequested:   page._resultCardClosed = true
                }
            }

            CapturedPanel {
                width: area.boardSize
                height: area.panelH
                anchors.horizontalCenter: parent.horizontalCenter
                avatarUrl: AvatarProvider.playerUrl
                pieces: ChessConnector.PlayerPlaysWhite ? ChessConnector.CapturedByLight : ChessConnector.CapturedByDark
                isActive: (ChessConnector.IsWhiteMove === 1) === ChessConnector.PlayerPlaysWhite
            }
        }
    }
}
