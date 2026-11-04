import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The clock of a new game, laid out like Lichess's quick pairing: a tile per time control (minutes +
// increment) with its speed under it, and No clock. One Tab stop, like radio buttons: the arrow keys
// pick a tile. restore() and remember() carry the choice over to the next game. online: what a Lichess
// challenge can be from a Board API app (LI-02): no Bullet, and Blitz only to a named player.
GridLayout {
    id: picker

    property int limitSec: 0
    property int incSec: 0
    property bool online: false
    property bool blitzAllowed: true

    readonly property var presets: [
        [1, 0, "Bullet"], [2, 1, "Bullet"], [3, 0, "Blitz"], [3, 2, "Blitz"], [5, 0, "Blitz"], [5, 3, "Blitz"],
        [10, 0, "Rapid"], [10, 5, "Rapid"], [15, 10, "Rapid"], [30, 0, "Classical"], [30, 20, "Classical"]
    ].map(p => ({ label: p[0] + "+" + p[1], limitSec: p[0] * 60, incSec: p[1], speed: p[2] }))
     .concat([{ label: "No clock", limitSec: 0, incSec: 0, speed: "" }])
     .filter(p => !online || (p.speed !== "Bullet" && p.speed !== ""))

    readonly property int _index: presets.findIndex(p => p.limitSec === limitSec && p.incSec === incSec)
    readonly property string speed: _index >= 0 ? presets[_index].speed : ""
    readonly property bool ready: _index >= 0 && usable(_index)   // the chosen tile can be played

    function usable(index) { return blitzAllowed || presets[index].speed !== "Blitz" }
    function restore() {
        limitSec = AppSettings.clockLimitSec
        incSec = AppSettings.clockIncSec
    }
    function remember() {
        AppSettings.clockLimitSec = limitSec
        AppSettings.clockIncSec = incSec
    }
    function select(index) {
        limitSec = presets[index].limitSec
        incSec = presets[index].incSec
    }
    function step(delta) {
        let next = _index + delta
        while (next >= 0 && next < presets.length && !usable(next))
            next += delta
        if (next >= 0 && next < presets.length)
            select(next)
    }

    columns: 4
    columnSpacing: Theme.s1
    rowSpacing: Theme.s1
    uniformCellWidths: true
    activeFocusOnTab: true
    Accessible.role: Accessible.List
    Accessible.name: "Clock"
    Keys.onLeftPressed:  step(-1)
    Keys.onRightPressed: step(1)
    Keys.onUpPressed:    step(-columns)
    Keys.onDownPressed:  step(columns)

    Repeater {
        model: picker.presets
        delegate: Rectangle {
            required property var modelData
            required property int index
            readonly property bool isSelected: picker._index === index

            enabled: picker.usable(index)
            opacity: enabled ? 1 : 0.45
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: Theme.radiusCard
            color: isSelected ? Theme.selectedBg
                 : tileMouse.containsMouse ? Theme.cardBgHover : Theme.cardBg
            border.color: isSelected ? Theme.accent : Theme.cardStroke
            border.width: isSelected ? 2 : 1

            Accessible.role: Accessible.RadioButton
            Accessible.name: modelData.label + (modelData.speed !== "" ? ", " + modelData.speed : "")
            Accessible.checked: isSelected

            Column {
                anchors.centerIn: parent

                Text {
                    text: modelData.label
                    color: Theme.textPrimary
                    font: Theme.bodyStrong
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: modelData.speed
                    visible: text !== ""
                    color: Theme.textSecondary
                    font: Theme.caption
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            FocusRing { visible: picker.activeFocus && isSelected }

            MouseArea {
                id: tileMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: picker.select(index)
            }
        }
    }
}
