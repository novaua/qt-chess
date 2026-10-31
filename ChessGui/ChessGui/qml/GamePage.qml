import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import ChessPlusPlus

// The game: command bar, the board between the two player cards, and the moves beside it (wide
// window) or in a sheet at the bottom (narrow). The result shows as a card over the board.
// ChessGame.qml owns the dialogs and answers the signals.
Page {
    id: page
    objectName: "game"

    property bool rematchPending: false   // a Lichess rematch challenge waits for the opponent

    signal homeRequested()
    signal newGameRequested()      // the result card: a new game of the same kind
    signal exportPgnRequested()
    signal resignRequested()
    signal levelsHelpRequested()
    signal settingsRequested()
    signal profileRequested()
    signal switchUserRequested()
    signal checkShown()            // the page opened on a position with the king in check (Continue)

    readonly property string mode: ChessConnector.GameMode
    readonly property bool gameOver: ChessConnector.GameResult !== ""
    property bool _resultShown: false       // a moment after the game ends, so the last move is seen first
    property bool _resultCardClosed: false
    readonly property bool pieceDragging: chessBoard.dragging   // Esc puts the piece back, doesn't leave
    property alias board: chessBoard                            // for ScreenshotScene's drag scene

    function flipBoard() { chessBoard.angle = chessBoard.angle === 0 ? 180 : 0 }
    function _facePlayer() { chessBoard.angle = ChessConnector.PlayerPlaysWhite ? 0 : 180 }

    Component.onCompleted: {
        _facePlayer()
        _resultShown = gameOver
        _movesWhenOpened = ChessConnector.MoveCount
        _syncMoveTiming()
        openTimer.start()
    }
    Component.onDestruction: ChessConnector.setBoardSettled(false)

    // A resumed game may start in check. Say so when the board first settles — the page open for a
    // second, the board turned to the player, and nothing waiting to be shown. Not while the saved
    // moves are replayed or just before a held robot answer; a move made meanwhile announces its own
    // check itself.
    property int _movesWhenOpened: -1   // set once when the page opens
    property bool _checkAsked: false
    Connections {
        target: ChessConnector
        function onBoardIdle() {
            if (page._checkAsked) return
            page._checkAsked = true
            if (ChessConnector.MoveCount === page._movesWhenOpened
                    && ChessConnector.CheckedKing >= 0 && !page.gameOver)
                page.checkShown()
        }
    }

    // ── The board has settled (BL-14): the page has been open for a second, the board isn't turning
    // and it shows the live position (not an earlier move). The move gate may then show a held robot
    // answer.
    property bool _openSecondPassed: false
    readonly property bool _boardSettled: _openSecondPassed && !chessBoard.turning && !ChessConnector.ReviewMode
    function _reportSettled() { ChessConnector.setBoardSettled(page._boardSettled) }
    // Reduce animations: pieces don't slide, so the gate needs no slide time (it keeps the pause).
    function _syncMoveTiming() { ChessConnector.setMoveTiming(Theme.reduceMotion ? 0 : Theme.slow) }

    Timer {
        id: openTimer
        interval: 1000
        onTriggered: {
            page._openSecondPassed = true
            page._reportSettled()
        }
    }
    Connections {
        target: chessBoard
        function onTurningChanged() { page._reportSettled() }
    }
    Connections {
        target: Theme
        function onReduceMotionChanged() { page._syncMoveTiming() }
    }

    Connections {
        target: ChessConnector
        function onNewGameStarted() {
            page._facePlayer()
            page.sheetOpen = false
        }
        function onPlayerPlaysWhiteChanged() { page._facePlayer() }
        function onReviewStateChanged() { page._reportSettled() }   // the board shows an earlier move
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
            enabled: ChessConnector.CanOfferDraw // Lichess allows one offer per 10 moves
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
                    text: "Copy PGN"
                    icon.source: Theme.icon("copy")
                    enabled: ChessConnector.PlyCount > 0
                    onTriggered: ChessConnector.copyPgn()
                }
                MenuItem {
                    text: "Export PGN…"
                    icon.source: Theme.icon("export")
                    enabled: ChessConnector.PlyCount > 0
                    onTriggered: page.exportPgnRequested()
                }
                MenuSeparator {}
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

    // ── Status for the move panel and the sheet ──
    readonly property bool _playerToMove: (ChessConnector.IsWhiteMove === 1) === ChessConnector.PlayerPlaysWhite
    readonly property string status: {
        if (gameOver)                      return ChessConnector.ResultText
        if (mode === "replay")             return "Move " + ChessConnector.ShownPly + " of " + ChessConnector.PlyCount
        if (ChessConnector.EngineThinking) return "Robot is thinking…"
        return (_playerToMove ? ChessConnector.PlayerName : ChessConnector.OpponentName) + " to move"
    }
    // The move on the board, e.g. "12. Nf3" or "12… exd5"
    readonly property string shownMove: ChessConnector.ShownMove
    // Reviewing earlier moves of the game (a saved game has no live end, so not there).
    readonly property string reviewText: !ChessConnector.ReviewMode ? ""
        : ChessConnector.ShownPly === 0 ? "Viewing the start position"
        : "Viewing " + shownMove

    readonly property bool canRematch: gameOver && ChessConnector.CanRematch && !rematchPending
    // The game is over and its result card closed: Rematch / New game move to the moves panel.
    readonly property bool _resultHidden: gameOver && _resultShown && _resultCardClosed

    // The board flipped away from the player: the player's card goes on top.
    readonly property bool _flipped: chessBoard.angle !== (ChessConnector.PlayerPlaysWhite ? 0 : 180)
    property bool sheetOpen: false

    // A player card bound to the player (bottom) or the opponent (top).
    component SideCard: PlayerCard {
        property bool isPlayer
        readonly property bool white: isPlayer === ChessConnector.PlayerPlaysWhite
        readonly property var level: ChessConnector.Levels[ChessConnector.LastLevel - 1]
        readonly property bool isRobot: !isPlayer && page.mode === "computer"

        avatarUrl: isPlayer ? AvatarProvider.playerUrl : AvatarProvider.opponentUrl
        name:      isPlayer ? ChessConnector.PlayerName : ChessConnector.OpponentName
        detail:    isRobot ? "Level " + level.level + " · " + level.rating : ""
        avatarTip: isRobot ? "Robot · Level " + level.level + " — " + level.name + "\n" + level.rating
                             + "\nClick for level info"
                           : ""
        pieces:    white ? ChessConnector.CapturedByLight : ChessConnector.CapturedByDark
        advantage: white ? ChessConnector.MaterialBalance : -ChessConnector.MaterialBalance
        isActive:  !page.gameOver && page.mode !== "replay" && (ChessConnector.IsWhiteMove === 1) === white
        onAvatarClicked: page.levelsHelpRequested()
    }

    Item {
        id: area
        anchors {
            fill: parent
            margins: Theme.s4
            bottomMargin: wide ? Theme.s4 : Theme.sheetHandleHeight + Theme.s2
        }

        // Side panel at width >= 960 or a landscape window (ThemeTokens §5); else the move sheet.
        readonly property bool wide: page.width >= 960 || page.width > page.height * 1.15
        readonly property real gap: Theme.s2
        readonly property real panelW: wide ? Theme.sidePanelWidth + Theme.s4 : 0
        // The board item: its squares take 95 % and the coordinates the edge around them (all of it
        // with the coordinates inside, Theme.boardSquares).
        readonly property real boardSize: Math.max(200, Math.min(width - panelW,
                                                height - 2 * (Theme.playerCardHeight + gap)))
        readonly property real squaresSize: boardSize * Theme.boardSquares
        onWideChanged: if (wide) page.sheetOpen = false   // the sheet is hidden then

        Column {
            id: boardColumn
            x: (area.width - width - area.panelW) / 2
            anchors.verticalCenter: parent.verticalCenter
            spacing: area.gap

            SideCard {
                isPlayer: page._flipped
                width: area.squaresSize
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Item {
                width: area.boardSize
                height: area.boardSize

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

                // The result, over the dimmed board.
                Rectangle {
                    anchors.centerIn: parent
                    width: area.squaresSize
                    height: width
                    visible: resultCard.visible
                    color: Theme.scrim
                }
                ResultCard {
                    id: resultCard
                    visible: page._resultShown && !page._resultCardClosed
                    anchors.centerIn: parent
                    width: Math.min(area.squaresSize - 2 * Theme.s5, 440)
                    resultText: ChessConnector.ResultText
                    canRematch: page.canRematch
                    onRematchRequested: ChessConnector.rematch()
                    onNewGameRequested: page.newGameRequested()
                    onReviewRequested: {
                        page._resultCardClosed = true
                        page.sheetOpen = !area.wide
                        ChessConnector.reviewFirst()
                    }
                    onCloseRequested: page._resultCardClosed = true
                }
            }

            SideCard {
                isPlayer: !page._flipped
                width: area.squaresSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        // Wide window: the moves beside the board, as tall as the cards and the board.
        Rectangle {
            visible: area.wide
            x: boardColumn.x + boardColumn.width + Theme.s4
            y: boardColumn.y + (area.boardSize - area.squaresSize) / 2
            width: Theme.sidePanelWidth
            height: boardColumn.height - (area.boardSize - area.squaresSize)
            radius: Theme.radiusCard
            color: Theme.cardBg
            border.color: Theme.cardStroke

            MovesPanel {
                anchors.fill: parent
                anchors.margins: 1
                status: page.status
                reviewText: page.reviewText
                resultActions: page._resultHidden
                canRematch: page.canRematch
                onExportRequested: page.exportPgnRequested()
                onRematchRequested: ChessConnector.rematch()
                onNewGameRequested: page.newGameRequested()
            }
        }
    }

    // ── Narrow window: the moves in a sheet at the bottom ──
    MouseArea {
        anchors.fill: parent
        visible: page.sheetOpen && !area.wide
        onClicked: page.sheetOpen = false
        Rectangle { anchors.fill: parent; color: Theme.scrim }
    }

    Rectangle {
        id: sheet
        visible: !area.wide
        width: Math.min(page.width - 2 * Theme.s4, Math.max(area.squaresSize, 360))
        height: page.sheetOpen ? Math.min(page.height * 0.6, 460) : Theme.sheetHandleHeight
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
        radius: Theme.radiusCard
        color: Theme.cardBg
        border.color: Theme.cardStroke
        clip: true

        Behavior on height { enabled: !Theme.reduceMotion; NumberAnimation { duration: Theme.slow; easing.type: Easing.OutCubic } }

        // Square bottom corners: the sheet sits on the window edge.
        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: Theme.radiusCard
            color: Theme.cardBg
        }

        AbstractButton {
            id: handle
            width: parent.width
            height: Theme.sheetHandleHeight
            hoverEnabled: true
            Accessible.name: page.sheetOpen ? "Hide moves" : "Show moves"
            onClicked: page.sheetOpen = !page.sheetOpen

            background: Rectangle {
                radius: Theme.radiusCard
                color: handle.hovered ? Theme.cardBgHover : "transparent"
            }
            contentItem: RowLayout {
                spacing: Theme.s2
                Label {
                    Layout.leftMargin: Theme.s4
                    text: "Moves"
                    font: Theme.bodyStrong
                }
                Label {
                    text: page.shownMove
                    font.pixelSize: Theme.body.pixelSize
                    font.features: { "tnum": 1 }
                }
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    text: page.reviewText !== "" ? page.reviewText : page.status
                    font: Theme.caption
                    color: Theme.textSecondary
                    elide: Text.ElideRight
                }
                Button {
                    visible: page.reviewText !== ""
                    text: "Back to live"
                    onClicked: ChessConnector.reviewLast()
                }
                // The closed result card again: Rematch, New game.
                Button {
                    visible: page._resultHidden && page.reviewText === ""
                    text: "Result"
                    icon.source: Theme.icon("trophy")
                    onClicked: page._resultCardClosed = false
                }
                IconImage {
                    Layout.rightMargin: Theme.s4
                    source: Theme.icon("next")
                    sourceSize.width: Theme.iconSizeDense; sourceSize.height: Theme.iconSizeDense
                    color: Theme.textSecondary
                    rotation: page.sheetOpen ? 90 : -90
                    Behavior on rotation { enabled: !Theme.reduceMotion; NumberAnimation { duration: Theme.normal } }
                }
            }
        }

        MovesPanel {
            anchors { top: handle.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
            visible: sheet.height > Theme.sheetHandleHeight
            showTitle: false   // the handle says "Moves", the status and the review
            onExportRequested: page.exportPgnRequested()
        }
    }
}
