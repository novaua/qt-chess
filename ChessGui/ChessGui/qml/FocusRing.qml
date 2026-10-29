import QtQuick

// The keyboard focus outline (Fluent: 2 px in the text colour, just outside the item) for
// items the style doesn't draw, e.g. an avatar or a player card. Bind `visible` to the focus.
Rectangle {
    anchors.fill: parent
    anchors.margins: -3
    radius: (parent.radius ?? 0) + 3
    color: "transparent"
    border.color: Theme.focusStroke
    border.width: 2
}
