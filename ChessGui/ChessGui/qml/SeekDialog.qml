import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus
import "timecontrols.js" as TimeControls
import "lichess.js" as Lichess

// LI-09 (decision D2): the Quick pairing grid's Custom tile, as lichess.org's "Create a game": a seek
// with the player's own clock. Real time (minutes and increment on lichess.org's slider steps), or
// Correspondence (days per move). Rated or casual and the variant come from the Quick pairing card
// (openWith). The Board API seeks Rapid, Classical and Correspondence only: a faster clock can't be
// sent, and says why. A real-time seek waits as a tile's does (the Custom tile shows it:
// realTimeSeekStarted); a correspondence one is left on Lichess (LichessClient.seekSent, D6). The last
// choice is remembered. available: a seek may start now (ChessGame: no seek waiting, no game starting or
// running in real time), checked again on Seek, as the state may change while the dialog is open.
AppDialog {
    id: dialog

    property bool rated: true
    property bool chess960: false
    property bool available: true

    signal realTimeSeekStarted()

    // lichess.org's steps: minutes ¼ … 180, increment 0 … 180 s, days per move.
    readonly property var minuteSteps: [0.25, 0.5, 0.75, 1, 1.5, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                                        17, 18, 19, 20, 25, 30, 35, 40, 45, 60, 75, 90, 105, 120, 135, 150, 165, 180]
    readonly property var incrementSteps: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                                           25, 30, 35, 40, 45, 60, 90, 120, 150, 180]
    readonly property var daySteps: [1, 2, 3, 5, 7, 10, 14]

    readonly property bool correspondence: kind.checkedIndex === 1
    readonly property int limitSec: Math.round(minuteSteps[Math.round(minutesSlider.value)] * 60)
    readonly property int incSec: incrementSteps[Math.round(incrementSlider.value)]
    readonly property int days: daySteps[daysRow.checkedIndex]
    readonly property string speed: LichessClient.speedOf(limitSec, incSec)   // "rapid"
    readonly property bool tooFast: !correspondence && speed !== "rapid" && speed !== "classical"
    readonly property bool canSeek: !tooFast && available
    readonly property string minutesText: TimeControls.label(limitSec, 0).replace(/\+0$/, "")   // "20", "½"

    // The card's choices; the clock as last sent from here.
    function openWith(isRated, isChess960) {
        rated = isRated
        chess960 = isChess960
        kind.checkedIndex = AppSettings.seekCorrespondence ? 1 : 0
        setClock(AppSettings.seekLimitSec, AppSettings.seekIncSec)
        daysRow.checkedIndex = Math.max(0, daySteps.indexOf(AppSettings.seekDays))
        open()
    }
    // The nearest steps to a clock (the scenes, a remembered one).
    function setClock(seconds, increment) {
        minutesSlider.value = nearest(minuteSteps, seconds / 60)
        incrementSlider.value = nearest(incrementSteps, increment)
    }
    function nearest(steps, value) {
        let best = 0
        for (let i = 1; i < steps.length; ++i)
            if (Math.abs(steps[i] - value) < Math.abs(steps[best] - value))
                best = i
        return best
    }

    title: "Custom seek"
    width: 440
    acceptEnabled: canSeek

    onAccepted: {
        if (!canSeek)   // Seek and Enter are off then; this is the last check
            return
        AppSettings.seekCorrespondence = correspondence
        if (correspondence) {
            AppSettings.seekDays = days
        } else {
            AppSettings.seekLimitSec = limitSec
            AppSettings.seekIncSec = incSec
        }
        LichessClient.startSeek(correspondence ? 0 : limitSec, correspondence ? 0 : incSec, correspondence ? days : 0,
                                rated, chess960 ? "chess960" : "standard")
        if (!correspondence && LichessClient.seeking)
            realTimeSeekStarted()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.s2

        Label {
            objectName: "seekChoices"
            text: (dialog.rated ? "Rated" : "Casual") + " · " + (dialog.chess960 ? "Chess960" : "Standard")
                  + ", as chosen on the Quick pairing card."
            color: Theme.textSecondary
            font: Theme.caption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            text: "Time control"
            color: Theme.textSecondary; font: Theme.caption
            Layout.topMargin: Theme.s2
        }
        ChoiceRow {
            id: kind
            objectName: "seekKind"
            options: [{ label: "Real time" }, { label: "Correspondence" }]
        }

        // ── Real time: the clock and its speed, then the two sliders ──
        Label {
            objectName: "seekClockText"
            visible: !dialog.correspondence
            text: TimeControls.label(dialog.limitSec, dialog.incSec) + " · " + Lichess.speedName(dialog.speed)
            font: Theme.subtitle
            color: dialog.tooFast ? Theme.critical : Theme.textPrimary
            Layout.topMargin: Theme.s2
        }
        Label {
            visible: !dialog.correspondence
            text: "Minutes per side: " + dialog.minutesText
            color: Theme.textSecondary; font: Theme.caption
        }
        Slider {
            id: minutesSlider
            objectName: "seekMinutes"
            visible: !dialog.correspondence
            from: 0; to: dialog.minuteSteps.length - 1; stepSize: 1
            snapMode: Slider.SnapAlways
            Layout.fillWidth: true
            Accessible.name: "Minutes per side, " + dialog.minutesText   // the value is a step's index
        }
        Label {
            visible: !dialog.correspondence
            text: "Increment in seconds: " + dialog.incSec
            color: Theme.textSecondary; font: Theme.caption
        }
        Slider {
            id: incrementSlider
            objectName: "seekIncrement"
            visible: !dialog.correspondence
            from: 0; to: dialog.incrementSteps.length - 1; stepSize: 1
            snapMode: Slider.SnapAlways
            Layout.fillWidth: true
            Accessible.name: "Increment, " + dialog.incSec + " seconds"
        }

        // ── Correspondence: the days each player has for a move ──
        Label {
            visible: dialog.correspondence
            text: "Days per move"
            color: Theme.textSecondary; font: Theme.caption
            Layout.topMargin: Theme.s2
        }
        ChoiceRow {
            id: daysRow
            objectName: "seekDays"
            visible: dialog.correspondence
            buttonWidth: 48
            options: dialog.daySteps.map(d => ({ label: String(d) }))
        }
        Label {
            visible: dialog.correspondence
            text: "Your seek stays on Lichess until someone takes it; only lichess.org can cancel it."
            color: Theme.textSecondary; font: Theme.caption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // Why Seek is off: the Board API's speeds.
        Label {
            objectName: "seekTooFast"
            visible: dialog.tooFast
            text: "Lichess allows this app Rapid, Classical and Correspondence only: the clock's seconds "
                  + "plus 40 × the increment must reach 480 (8 minutes). Play faster games on lichess.org."
            color: Theme.critical
            font: Theme.caption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.topMargin: Theme.s1
        }
        // Why Seek is off: something else is under way.
        Label {
            objectName: "seekUnavailable"
            visible: !dialog.available
            text: LichessClient.seeking ? "A seek is waiting already: cancel it first."
                                        : "A game is starting or going on: one real-time game at a time."
            color: Theme.critical
            font: Theme.caption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.topMargin: Theme.s1
        }
    }

    footer: DialogButtonBox {
        defaultButton: seekButton // highlighted (the box sets it, Qt 6.11+)
        Button {
            id: seekButton
            objectName: "seekButton"
            text: "Seek"
            enabled: dialog.canSeek
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
