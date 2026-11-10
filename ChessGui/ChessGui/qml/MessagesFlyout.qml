import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import ChessPlusPlus

// The recent messages (BL-13 step 3, §9.9), for one the player missed: newest first, the full text, the
// time, and the game it came from when that isn't the one on screen. An offer still open keeps Accept /
// Decline here; the opponent left keeps Claim victory while it can be claimed. ChessGame.qml owns the
// list (a ListModel: text, severity, kind, offer, status, time, source) and opens this under the button
// whose menu asked for it, or under the command bar's right end (Ctrl+M).
Item {
    id: flyout

    property var model
    property bool canClaim: false   // the opponent left and Lichess takes the claim now
    property real panelX: 0         // where the panel opens, in this item's (the window's) coordinates
    property real panelY: 0
    readonly property real panelWidth: Math.min(380, width - 2 * Theme.s2)

    signal offerAnswered(bool accept)
    signal clearRequested()

    // A panel in the window, not a Popup: a Popup opened from here neither drew nor let the window's
    // frames go on in the screenshot mode's window. A click outside or Esc closes it.
    function open() {
        visible = true
        list.forceActiveFocus()
    }
    function close() { visible = false }

    visible: false
    z: 20

    Shortcut {
        sequence: "Escape"
        enabled: flyout.visible
        onActivated: flyout.close()
    }

    // Outside the panel: a click closes it (and goes no further, as with a menu).
    MouseArea {
        anchors.fill: parent
        onClicked: flyout.close()
    }

    // Inside the window: the list gets the height left under panelY (at least a few rows), and the
    // panel moves up when even that doesn't fit.
    readonly property real _chromeHeight: titleLabel.implicitHeight + Theme.s3 + Theme.s2   // the title and its margins
                                          + 1 + footer.implicitHeight + 2 * Theme.s2        // the divider, the footer
    readonly property real _listMax: Math.max(120, Math.min(420, height - panelY - Theme.s2 - _chromeHeight))
    Rectangle {
        id: panel
        x: Math.max(Theme.s2, Math.min(flyout.panelX, flyout.width - width - Theme.s2))
        y: Math.max(Theme.s2, Math.min(flyout.panelY, flyout.height - height - Theme.s2))
        width: flyout.panelWidth
        height: content.implicitHeight
        radius: Theme.radiusCard
        color: Theme.cardBg
        border.color: Theme.cardStroke
        opacity: flyout.visible ? 1 : 0
        Behavior on opacity { enabled: !Theme.reduceMotion; NumberAnimation { duration: Theme.fast } }

        MouseArea { anchors.fill: parent }   // clicks on the panel stay in it

        ColumnLayout {
            id: content
            width: parent.width
            spacing: 0

            Label {
                id: titleLabel
                text: "Recent messages"
                font: Theme.bodyStrong
                Layout.margins: Theme.s3
                Layout.bottomMargin: Theme.s2
            }

            Label {
                visible: list.count === 0
                text: "No messages yet."
                color: Theme.textSecondary
                Layout.margins: Theme.s3
                Layout.topMargin: 0
            }

            ListView {
                id: list
                visible: count > 0
                model: flyout.model
                clip: true
                focus: true
                keyNavigationEnabled: true
                boundsBehavior: Flickable.StopAtBounds
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(contentHeight, flyout._listMax)
                ScrollBar.vertical: ScrollBar {}
                Accessible.role: Accessible.List
                Accessible.name: "Recent messages"

                delegate: Rectangle {
                    id: row
                    required property int index
                    required property string text
                    required property string severity
                    required property string kind
                    required property string offer
                    required property string status
                    required property string time
                    required property string source

                    readonly property string _currentSource: ChessConnector.InGame ? ChessConnector.GameTitle : ""
                    readonly property bool offerOpen: kind === "offer" && status === "open" && ChessConnector.PendingOffer === offer
                    readonly property bool goneOpen: kind === "gone" && status === "open" && ChessConnector.OpponentGone
                    readonly property string detail: [time,
                                                      source !== "" && source !== _currentSource ? source : "",
                                                      status !== "" && status !== "open" ? status : ""]
                                                     .filter(t => t !== "").join(" · ")

                    width: ListView.view.width
                    implicitHeight: content.implicitHeight + 2 * Theme.s2
                    color: ListView.isCurrentItem && list.activeFocus ? Theme.cardBgHover : "transparent"

                    Accessible.role: Accessible.ListItem
                    Accessible.name: text + ", " + detail + (offerOpen || goneOpen ? ", open" : "")

                    RowLayout {
                        id: content
                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter
                                  leftMargin: Theme.s3; rightMargin: Theme.s3 }
                        spacing: Theme.s2

                        IconImage {
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: 1
                            source: Theme.icon("info")
                            sourceSize.width: Theme.iconSizeDense; sourceSize.height: Theme.iconSizeDense
                            color: ({ success: Theme.success, caution: Theme.caution, critical: Theme.critical })[row.severity]
                                   ?? Theme.accent
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                text: row.text
                                wrapMode: Text.WrapAtWordBoundaryOrAnywhere   // a path has no spaces
                                Layout.fillWidth: true
                            }
                            Label {
                                text: row.detail
                                font: Theme.caption
                                color: Theme.textSecondary
                                Layout.fillWidth: true
                            }
                            Row {
                                visible: row.offerOpen || (row.goneOpen && flyout.canClaim)
                                spacing: Theme.s2
                                Layout.topMargin: Theme.s1
                                Button {
                                    visible: row.offerOpen
                                    text: "Accept"; highlighted: true
                                    onClicked: flyout.offerAnswered(true)
                                }
                                Button {
                                    visible: row.offerOpen
                                    text: "Decline"
                                    onClicked: flyout.offerAnswered(false)
                                }
                                Button {
                                    visible: row.goneOpen && flyout.canClaim
                                    text: "Claim victory"; highlighted: true
                                    onClicked: ChessConnector.claimVictory()
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.divider
            }
            RowLayout {
                id: footer
                Layout.margins: Theme.s2
                Label {
                    text: "This session only"
                    font: Theme.caption
                    color: Theme.textSecondary
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.s1
                }
                Button {
                    text: "Clear"
                    flat: true
                    enabled: list.count > 0
                    onClicked: flyout.clearRequested()
                }
            }
        }
    }
}
