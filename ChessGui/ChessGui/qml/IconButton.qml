import QtQuick
import QtQuick.Controls

// Icon-only button: `icon.source: Theme.icon("undo")`, with `tip` as tooltip and accessible name.
ToolButton {
    property string tip
    property bool   dense: false   // 16 px icon on a 32 px button (move list); else 20 on 40 (command bar)

    display: AbstractButton.IconOnly
    icon.width:  dense ? Theme.iconSizeDense : Theme.iconSize
    icon.height: dense ? Theme.iconSizeDense : Theme.iconSize
    implicitWidth:  dense ? 32 : 40
    implicitHeight: dense ? 32 : 40
    Accessible.name: tip
    ToolTip.visible: hovered && tip !== ""
    ToolTip.text: tip
    ToolTip.delay: 400
}
