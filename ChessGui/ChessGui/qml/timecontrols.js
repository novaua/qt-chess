.pragma library

// lichess.org's quick pairing clocks (LI-09): the setup dialogs' and the challenge card's clock tiles
// (TimeControlPicker) and the Online page's quick pairing grid (QuickPairingGrid) show the same eleven.
// label "10+5", limitSec, incSec, speed: Lichess's name of its speed ("Rapid").
var presets = [
    [1, 0, "Bullet"], [2, 1, "Bullet"], [3, 0, "Blitz"], [3, 2, "Blitz"], [5, 0, "Blitz"], [5, 3, "Blitz"],
    [10, 0, "Rapid"], [10, 5, "Rapid"], [15, 10, "Rapid"], [30, 0, "Classical"], [30, 20, "Classical"]
].map(p => ({ label: p[0] + "+" + p[1], limitSec: p[0] * 60, incSec: p[1], speed: p[2] }))

// A clock as lichess.org writes it: "10+5", "½+12".
function label(limitSec, incSec) {
    const minutes = limitSec / 60
    const whole = Math.floor(minutes)
    const fraction = ({ 0.25: "¼", 0.5: "½", 0.75: "¾" })[minutes - whole] ?? ""
    return (whole > 0 || fraction === "" ? String(whole) : "") + fraction + "+" + incSec
}

// "0:42", "12:05": the time a seek has waited.
function elapsed(seconds) {
    const s = Math.max(0, Math.floor(seconds))
    return Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0")
}
