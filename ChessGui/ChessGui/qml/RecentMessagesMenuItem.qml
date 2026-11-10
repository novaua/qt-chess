import QtQuick
import QtQuick.Controls

// "Recent messages" in the game's More menu and Home's account menu (BL-13 step 3), with how many are
// new. `count` is the window's unseenMessages.
MenuItem {
    property int count: 0

    text: count > 0 ? "Recent messages (" + count + " new)" : "Recent messages"
    icon.source: Theme.icon("info")
}
