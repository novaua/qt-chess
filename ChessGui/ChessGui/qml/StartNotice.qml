import QtQuick
import QtQuick.Controls

// BL-16: "Starting the game…" while the game page is built (ChessGame.route(), which sets `shown`). It
// takes the clicks meanwhile. It fades in late, by an Animator (on the render thread, like the ring's turn:
// both go on while the GUI thread builds the page): a start of 150 ms barely shows it, a slow one (a Debug
// build, a slow PC or a phone) has it after about 0.3 s. Not a motion: also with Reduce animations.
// Over the popups too (the Overlay layer): a setup dialog that started the game is still closing (its fade
// waits for the GUI thread as well), and would cover it.
MouseArea {
    id: notice

    property bool shown: false

    parent: Overlay.overlay
    anchors.fill: parent
    visible: shown
    hoverEnabled: true
    acceptedButtons: Qt.AllButtons
    z: 1000
    onVisibleChanged: if (visible) noticeFade.restart(); else noticeFade.stop()

    Item {
        id: noticeLayer
        anchors.fill: parent
        opacity: 0

        Rectangle {   // the page behind, dimmed as under a dialog
            anchors.fill: parent
            color: Theme.scrim
        }
        Rectangle {
            id: noticeCard
            anchors.centerIn: parent
            width: noticeRow.implicitWidth + 2 * Theme.s5
            height: noticeRow.implicitHeight + 2 * Theme.s5
            radius: Theme.radiusCard
            color: Theme.layerBg
            border.color: Theme.cardStroke
            Accessible.role: Accessible.AlertMessage
            Accessible.name: noticeLabel.text

            Row {
                id: noticeRow
                anchors.centerIn: parent
                spacing: Theme.s3
                BusyRing {
                    width: Theme.iconSize
                    height: Theme.iconSize
                    anchors.verticalCenter: parent.verticalCenter
                }
                Label {
                    id: noticeLabel
                    text: "Starting the game…"
                    font: Theme.bodyStrong
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
    OpacityAnimator {
        id: noticeFade
        target: noticeLayer
        from: 0
        to: 1
        duration: 450
        easing.type: Easing.InExpo   // ~0 for the first 250 ms
    }
}
