import QtQuick

// The dot on a button for new recent messages the player didn't cause (BL-13 step 3): on the game
// page's More button and Home's account button. Put it in the button; `count` is the window's
// unseenMessages.
Rectangle {
    property int count: 0

    visible: count > 0
    width: 8; height: 8; radius: 4
    color: Theme.accent
    anchors { top: parent.top; right: parent.right; topMargin: 8; rightMargin: 8 }
}
