import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import ChessPlusPlus

// The active profile: name and avatar (applied right away), the linked Lichess account, the
// statistics per opponent, and export / delete. Pushed over Home or the game; Back pops it.
Page {
    id: page
    objectName: "profile"

    signal backRequested()
    signal exportRequested(string userId)
    signal deleteRequested(string userId)
    signal disconnectLichessRequested()

    property string _linkError
    property bool   _showToken: false

    Connections {
        target: UserManager
        function onLichessLinkFailed(error) { page._linkError = error }
        function onLichessChanged() { if (UserManager.lichessConnected) page._linkError = "" }
    }

    function _levelName(level) {
        var levels = ChessConnector.Levels
        return level >= 1 && level <= levels.length ? "Level " + level + " · " + levels[level - 1].name
                                                    : "Level not recorded"
    }
    function _opponentText(row) {
        switch (row.kind) {
        case "computer": return "Robot · " + _levelName(row.level)
        case "guest":    return "Guest"
        case "lichess":  return "Lichess games"
        case "earlier":  return "Two players, before results were kept"
        default:         return row.name
        }
    }
    readonly property var _ratingPools: [
        { key: "bullet", name: "Bullet" }, { key: "blitz", name: "Blitz" }, { key: "rapid", name: "Rapid" },
        { key: "classical", name: "Classical" }, { key: "correspondence", name: "Correspondence" }
    ]

    background: Rectangle { color: Theme.windowBg }

    header: CommandBar {
        IconButton {
            icon.source: Theme.icon("back")
            tip: "Back (Esc)"
            onClicked: page.backRequested()
        }
        Label {
            text: "Profile"
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

            // ── Identity ──
            SectionCard {
                title: "Name and avatar"
                Layout.fillWidth: true

                RowLayout {
                    spacing: Theme.s3
                    Layout.fillWidth: true

                    Image {
                        source: "qrc:/app/pics/avatars/" + UserManager.activeUserAvatar + ".png"
                        sourceSize.width: 56; sourceSize.height: 56
                    }
                    TextField {
                        id: nameField
                        text: UserManager.activeUserName
                        placeholderText: "Your name"
                        maximumLength: 40
                        Layout.fillWidth: true
                        Accessible.name: "Name"
                        onEditingFinished: {
                            if (text.trim() === "") text = UserManager.activeUserName
                            else if (text.trim() !== UserManager.activeUserName)
                                UserManager.updateProfile(text.trim(), UserManager.activeUserAvatar)
                        }
                    }
                }
                AvatarPicker {
                    columns: Math.max(5, Math.min(10, Math.floor((content.width - 2 * Theme.s4 + spacing) / (cellSize + spacing))))
                    cellSize: 48
                    selected: UserManager.activeUserAvatar
                    onPicked: name => UserManager.updateProfile(UserManager.activeUserName, name)
                }
            }

            // ── Lichess ──
            SectionCard {
                title: "Lichess account"
                subtitle: UserManager.lichessConnected || UserManager.lichessNeedsReconnect ? ""
                          : "Link your Lichess account to play online. The app can then create challenges and play your moves."
                Layout.fillWidth: true

                // Expired or imported: the account is known, but this PC has no working sign-in.
                InfoBar {
                    visible: UserManager.lichessNeedsReconnect && !UserManager.lichessLinking
                    closable: false
                    severity: "caution"
                    text: "Lichess sign-in expired for " + UserManager.lichessUsername + "."
                    Layout.fillWidth: true
                    Button {
                        text: "Reconnect"
                        highlighted: true
                        onClicked: { page._linkError = ""; UserManager.linkLichess() }
                    }
                }

                // Linked: who, ratings, links.
                RowLayout {
                    visible: UserManager.lichessConnected
                    spacing: Theme.s2
                    Layout.fillWidth: true
                    IconImage {
                        source: Theme.icon("globe")
                        sourceSize.width: Theme.iconSize; sourceSize.height: Theme.iconSize
                        color: Theme.accent
                    }
                    Label {
                        text: UserManager.lichessUsername
                        font: Theme.bodyStrong
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Label {
                        text: "Connected"
                        color: Theme.success
                        font: Theme.caption
                    }
                }
                Flow {
                    visible: UserManager.lichessConnected || UserManager.lichessNeedsReconnect
                    spacing: Theme.s2
                    Layout.fillWidth: true

                    Repeater {
                        model: page._ratingPools.filter(p => UserManager.lichessRatings[p.key] !== undefined)
                        delegate: Rectangle {
                            required property var modelData
                            readonly property var perf: UserManager.lichessRatings[modelData.key]
                            width: 112; height: 56
                            radius: Theme.radiusCard
                            color: Theme.layerBg
                            border.color: Theme.cardStroke
                            Accessible.name: modelData.name + " rating " + perf.rating
                            Column {
                                anchors.centerIn: parent
                                Label {
                                    text: perf.rating + (perf.prov ? "?" : "")
                                    font: Theme.subtitle
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Label {
                                    text: modelData.name
                                    font: Theme.caption
                                    color: Theme.textSecondary
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }
                    }
                }
                Label {
                    visible: UserManager.lichessConnected && Object.keys(UserManager.lichessRatings).length === 0
                    text: "No rated games yet."
                    font: Theme.caption
                    color: Theme.textSecondary
                }
                RowLayout {
                    visible: UserManager.lichessConnected || UserManager.lichessNeedsReconnect
                    spacing: Theme.s2
                    Button {
                        text: "Open profile on lichess.org"
                        icon.source: Theme.icon("link")
                        flat: true
                        onClicked: Qt.openUrlExternally("https://lichess.org/@/" + UserManager.lichessUsername)
                    }
                    Button {
                        text: "Disconnect…"
                        icon.source: Theme.icon("sign-out")
                        flat: true
                        enabled: !UserManager.lichessLinking
                        onClicked: page.disconnectLichessRequested()
                    }
                }

                // Not linked: OAuth, or a personal API token behind Advanced.
                Button {
                    visible: !UserManager.lichessConnected && !UserManager.lichessNeedsReconnect
                    text: UserManager.lichessLinking ? "Waiting for the browser…" : "Link Lichess account"
                    icon.source: Theme.icon("globe")
                    highlighted: true
                    enabled: !UserManager.lichessLinking
                    onClicked: { page._linkError = ""; UserManager.linkLichess() }
                }

                // Waiting for the browser, with Cancel (BL-08).
                RowLayout {
                    visible: UserManager.lichessLinking
                    spacing: Theme.s2
                    Layout.fillWidth: true
                    BusyIndicator {
                        running: UserManager.lichessLinking
                        Layout.preferredWidth: 20; Layout.preferredHeight: 20
                    }
                    Label {
                        text: "Sign in on lichess.org and allow Chess++ to play for you. Didn't see a browser tab? Cancel and try again."
                        font: Theme.caption
                        color: Theme.textSecondary
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Button {
                        text: "Cancel"
                        onClicked: UserManager.cancelLichessLink()
                    }
                }

                Label {
                    visible: page._linkError !== "" && !UserManager.lichessLinking
                    text: "Couldn't link the account: " + page._linkError
                    color: Theme.critical
                    font: Theme.caption
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    visible: !UserManager.lichessConnected && !UserManager.lichessLinking
                    spacing: Theme.s2
                    Layout.fillWidth: true

                    Button {
                        text: "Advanced: use an API token"
                        icon.source: Theme.icon(page._showToken ? "prev" : "next")
                        flat: true
                        onClicked: page._showToken = !page._showToken
                    }
                    RowLayout {
                        visible: page._showToken
                        spacing: Theme.s2
                        Layout.fillWidth: true
                        TextField {
                            id: tokenField
                            placeholderText: "Personal API token (scopes board:play, challenge:write)"
                            echoMode: TextInput.Password
                            Layout.fillWidth: true
                        }
                        Button {
                            text: "Link"
                            enabled: tokenField.text.trim() !== ""
                            onClicked: {
                                page._linkError = ""
                                UserManager.linkLichessToken(tokenField.text)
                                tokenField.text = ""
                            }
                        }
                    }
                }
            }

            // ── Statistics ──
            SectionCard {
                title: "Statistics"
                subtitle: UserManager.gamesPlayed === 0 ? "No finished games yet."
                    : UserManager.totals.games + " games · " + UserManager.totals.wins + " won · "
                      + UserManager.totals.draws + " drawn · " + UserManager.totals.losses + " lost · since "
                      + UserManager.statsCreatedDate
                Layout.fillWidth: true

                GridLayout {
                    visible: UserManager.gamesPlayed > 0
                    columns: 5
                    columnSpacing: Theme.s4
                    rowSpacing: Theme.s1
                    Layout.fillWidth: true

                    Repeater {
                        model: ["Opponent", "Games", "Won", "Drawn", "Lost"]
                        Label {
                            required property string modelData
                            required property int index
                            text: modelData
                            font: Theme.caption
                            color: Theme.textSecondary
                            horizontalAlignment: index === 0 ? Text.AlignLeft : Text.AlignRight
                            Layout.fillWidth: index === 0
                        }
                    }
                    Repeater {
                        model: UserManager.statistics
                        delegate: Repeater {
                            required property var modelData
                            readonly property var row: modelData
                            model: [page._opponentText(row), row.games, row.wins, row.draws, row.losses]
                            Label {
                                required property var modelData
                                required property int index
                                text: modelData
                                elide: Text.ElideRight
                                font.features: { "tnum": 1 }
                                horizontalAlignment: index === 0 ? Text.AlignLeft : Text.AlignRight
                                Layout.fillWidth: index === 0
                            }
                        }
                    }
                }
            }

            // ── Data ──
            SectionCard {
                title: "Profile data"
                subtitle: "An export is one file with this profile, its statistics and saved games, for another PC or "
                          + "as a backup. It never contains your Lichess sign-in."
                Layout.fillWidth: true

                RowLayout {
                    spacing: Theme.s2
                    Button {
                        text: "Export profile…"
                        icon.source: Theme.icon("export")
                        onClicked: page.exportRequested(UserManager.activeUserId)
                    }
                    Button {
                        text: "Delete profile…"
                        icon.source: Theme.icon("delete")
                        enabled: !ChessConnector.InGame
                        onClicked: page.deleteRequested(UserManager.activeUserId)
                    }
                }
                Label {
                    visible: ChessConnector.InGame
                    text: "Leave the game to delete this profile."
                    font: Theme.caption
                    color: Theme.textSecondary
                }
            }
        }
    }
}
