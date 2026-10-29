import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

// A clickable card on Home: a 24 px icon (`icon.source`), the title (`text`) and a description.
AbstractButton {
    id: card

    property string description
    property bool   dimmed: false   // looks unavailable but still clicks: it then explains what's missing
    property string shortcut        // shown as the tooltip, e.g. "Ctrl+N" (ChessGame.qml handles the keys)

    padding: Theme.s4
    implicitWidth:  implicitContentWidth + leftPadding + rightPadding
    implicitHeight: implicitContentHeight + topPadding + bottomPadding
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    // Space clicks any button; Enter too on a card, like a link.
    Keys.onReturnPressed: click()
    Keys.onEnterPressed:  click()

    Accessible.name: text
    Accessible.description: description
    ToolTip.visible: hovered && shortcut !== ""
    ToolTip.text: shortcut
    ToolTip.delay: 600

    background: Rectangle {
        radius: Theme.radiusCard
        color: card.hovered || card.down ? Theme.cardBgHover : Theme.cardBg
        border.color: card.visualFocus || (Theme.highContrast && card.hovered) ? Theme.accent : Theme.cardStroke
        border.width: card.visualFocus ? 2 : 1
    }

    contentItem: RowLayout {
        spacing: Theme.s3

        IconImage {
            source: card.icon.source
            sourceSize.width: Theme.iconSizeCard; sourceSize.height: Theme.iconSizeCard
            color: card.dimmed ? Theme.textDisabled : Theme.accent
            Layout.alignment: Qt.AlignTop
        }
        ColumnLayout {
            spacing: 2
            Layout.fillWidth: true

            Label {
                text: card.text
                font: Theme.bodyStrong
                color: card.dimmed ? Theme.textSecondary : Theme.textPrimary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Label {
                text: card.description
                visible: text !== ""
                font: Theme.caption
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}
