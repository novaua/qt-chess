import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A player beside the board: avatar, name, a detail (the robot's level), the pieces they have taken
// with "+N", whose turn it is (an accent bar and a pulse on the avatar) and their clock.
// In a row above or below the board, or `stacked` in the player column left of it (BL-13): the clock
// on its own row and the captured pieces wrapping; `clockFirst` puts the clock at the card's top (the
// top card) instead of its bottom, so both clocks sit at the column's outer edges, as on lichess.org.
Rectangle {
    id: card

    property string avatarUrl
    property string avatarTip: ""   // tooltip; empty = the avatar is not clickable
    property string name
    property string detail
    property bool   isActive:  false
    property var    pieces:    []
    property int    advantage: 0
    property string clockText: ""      // empty: a game without a clock
    property bool   clockRunning: false
    property bool   clockLow:  false
    property bool   stacked:   false
    property bool   clockFirst: false  // stacked: the clock on top

    signal avatarClicked()

    implicitHeight: stacked ? stackedContent.implicitHeight + 2 * Theme.s3 : Theme.playerCardHeight
    radius: Theme.radiusCard
    color: Theme.cardBg
    border.color: Theme.cardStroke

    Accessible.role: Accessible.Pane
    Accessible.name: name + (isActive ? ", to move" : "") + (clockText !== "" ? ", " + clockText : "")

    // The side to move: an accent bar on the left edge.
    Rectangle {
        visible: card.isActive
        width: 3
        radius: 1.5
        color: Theme.accent
        anchors {
            left: parent.left; leftMargin: 3
            top: parent.top; topMargin: Theme.s3
            bottom: parent.bottom; bottomMargin: Theme.s3
        }
    }

    component Avatar: Rectangle {
        implicitWidth: 40
        implicitHeight: 40
        radius: Theme.radiusAvatar
        color: Theme.layerBg
        border.color: card.isActive ? Theme.accent : Theme.cardStroke
        border.width: card.isActive ? 2 : 1

        Image {
            anchors.fill: parent
            anchors.margins: 3
            fillMode: Image.PreserveAspectFit
            source: card.avatarUrl
            sourceSize: Qt.size(80, 80)
        }

        Rectangle {
            id: pulse
            anchors.fill: parent
            anchors.margins: 1
            radius: parent.radius - 1
            color: Theme.accent
            opacity: 0
            visible: card.isActive

            SequentialAnimation on opacity {
                running: card.isActive && pulse.parent.visible && !Theme.reduceMotion
                loops: Animation.Infinite
                NumberAnimation { to: 0.2; duration: 850; easing.type: Easing.InOutSine }
                NumberAnimation { to: 0.0; duration: 850; easing.type: Easing.InOutSine }
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: card.avatarTip !== ""
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: card.avatarClicked()
            ToolTip.visible: containsMouse
            ToolTip.text: card.avatarTip
            ToolTip.delay: 300
        }
    }

    component NameRow: RowLayout {
        spacing: Theme.s2
        Label {
            text: card.name
            font: Theme.bodyStrong
            elide: Text.ElideRight
            Layout.fillWidth: !detailLabel.visible
        }
        Label {
            id: detailLabel
            text: card.detail
            visible: text !== ""
            font: Theme.caption
            color: Theme.textSecondary
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    // The clock, as on Lichess: lit while it runs, red when low. At least as wide as 00:00.0, so it
    // doesn't jump at 10 s.
    component Clock: Rectangle {
        property font font: Theme.subtitle
        readonly property real neededWidth: Math.max(clockMetrics.width, clockLabel.implicitWidth) + 2 * Theme.s2
        visible: card.clockText !== ""
        radius: Theme.radiusAvatar
        color: card.clockLow ? Theme.criticalBg : card.clockRunning ? Theme.selectedBg : "transparent"
        border.color: !card.clockRunning ? "transparent" : card.clockLow ? Theme.critical : Theme.accent

        TextMetrics {
            id: clockMetrics
            font: clockLabel.font
            text: "00:00.0"
        }
        Label {
            id: clockLabel
            anchors.centerIn: parent
            text: card.clockText
            color: card.clockLow ? Theme.critical : Theme.textPrimary
            font.pixelSize: parent.font.pixelSize
            font.weight: parent.font.weight
            font.features: { "tnum": 1 }
        }
    }

    component StackedClock: Clock {
        Layout.fillWidth: true
        Layout.preferredHeight: 48
        font: Theme.title
    }
    component StackedPieces: CapturedStrip {
        pieces: card.pieces
        advantage: card.advantage
        pieceSize: 18
        wrapWidth: width
        Layout.fillWidth: true
    }

    // ── In a row above or below the board ──
    RowLayout {
        visible: !card.stacked
        anchors {
            fill: parent
            leftMargin: Theme.s3; rightMargin: Theme.s3
        }
        spacing: Theme.s3

        Avatar {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            NameRow { Layout.fillWidth: true }

            CapturedStrip {
                pieces: card.pieces
                advantage: card.advantage
                pieceSize: 18
                Layout.preferredHeight: 18
            }
        }

        // The name gives way on a narrow card.
        Clock {
            id: rowClock
            Layout.preferredHeight: 40
            Layout.minimumWidth: rowClock.neededWidth
            Layout.preferredWidth: Layout.minimumWidth
        }
    }

    // ── Stacked in the player column (BL-13): clock, name, pieces; mirrored for the bottom card ──
    ColumnLayout {
        id: stackedContent
        visible: card.stacked
        anchors {
            left: parent.left; right: parent.right; top: parent.top
            margins: Theme.s3
        }
        spacing: Theme.s2

        StackedClock  { visible: card.clockFirst && card.clockText !== "" }
        StackedPieces { visible: !card.clockFirst }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.s3
            Avatar {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
            }
            NameRow { Layout.fillWidth: true }
        }
        StackedPieces { visible: card.clockFirst }
        StackedClock  { visible: !card.clockFirst && card.clockText !== "" }
    }
}
