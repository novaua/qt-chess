import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AppDialog {
    id: dialog

    property bool   _busy:       false
    property string _statusText: ""

    signal closeRequested()

    title: "Play on Lichess"
    width: 420
    acceptEnabled: false   // two different actions (create / join): no default for Enter

    onRejected: {
        lichessClient.stopStream()
        dialog._busy       = false
        dialog._statusText = ""
        dialog.closeRequested()
    }

    Connections {
        target: lichessClient

        function onChallengeCreated(gameId, joinUrl) {
            dialog._busy = false
        }

        function onGameStarted(gameId, playingAsWhite, opponentName, opponentAvatarUrl) {
            dialog._busy = false
        }

        function onNetworkError(message) {
            dialog._busy       = false
            dialog._statusText = "Error: " + message
        }
    }

    // A row of mutually exclusive option buttons (time control, side).
    component ChoiceRow: Row {
        id: choiceRow
        property var options: []          // [{ label, ... }]
        property int checkedIndex: 0
        spacing: Theme.s1
        Repeater {
            model: choiceRow.options
            delegate: Button {
                required property var modelData
                required property int index
                text: modelData.label
                checked: index === choiceRow.checkedIndex   // not checkable: a click can't uncheck it
                onClicked: choiceRow.checkedIndex = index
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.s3

        // ── No token configured ─────────────────────────────────────────────
        ColumnLayout {
            visible: !userManager.lichessConnected
            Layout.fillWidth: true
            spacing: Theme.s3

            Label {
                text: "Connect your Lichess account in your profile to play online."
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Button {
                text: "Open profile"
                icon.source: Theme.icon("person")
                highlighted: true
                onClicked: {
                    _showProfile = true
                    dialog.closeRequested()
                }
            }
        }

        // ── Connected — create + join ────────────────────────────────────────
        ColumnLayout {
            visible: userManager.lichessConnected
            Layout.fillWidth: true
            spacing: Theme.s2

            Label { text: "Challenge a friend"; font: Theme.bodyStrong }

            Label { text: "Game type"; color: Theme.textSecondary; font: Theme.caption }
            ComboBox {
                id: comboVariant
                model: ["Standard", "Chess960"]
                Layout.preferredWidth: 160
            }

            Label { text: "Time control"; color: Theme.textSecondary; font: Theme.caption }
            ChoiceRow {
                id: tcType
                options: [{ label: "Unlimited" }, { label: "Real time" }]
            }

            // Preset time controls — shown only for Real time
            ChoiceRow {
                id: tcPreset
                visible: tcType.checkedIndex === 1
                checkedIndex: 2
                options: [{ label: "1+0",   min: 1,  inc: 0 },
                          { label: "3+2",   min: 3,  inc: 2 },
                          { label: "5+3",   min: 5,  inc: 3 },
                          { label: "10+0",  min: 10, inc: 0 },
                          { label: "15+10", min: 15, inc: 10 }]
            }

            Label { text: "Side"; color: Theme.textSecondary; font: Theme.caption }
            ChoiceRow {
                id: side
                options: [{ label: "Random", value: "random" },
                          { label: "White",  value: "white"  },
                          { label: "Black",  value: "black"  }]
            }

            Button {
                text: "Create challenge"
                icon.source: Theme.icon("link")
                highlighted: true
                enabled: !dialog._busy
                Layout.topMargin: Theme.s1
                onClicked: {
                    var preset = tcType.checkedIndex === 1 ? tcPreset.options[tcPreset.checkedIndex]
                                                           : { min: 0, inc: 0 }
                    var variantValues = ["standard", "chess960"]
                    var chosenVariant = variantValues[comboVariant.currentIndex] || "standard"
                    dialog._busy = true
                    lichessClient.createOpenChallenge(preset.min, preset.inc,
                                                      side.options[side.checkedIndex].value, chosenVariant)
                }
            }

            Label {
                visible: dialog._statusText !== ""
                text: dialog._statusText
                color: Theme.critical
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s2; Layout.bottomMargin: Theme.s2
                implicitHeight: 1
                color: Theme.divider
            }

            Label { text: "Join a game"; font: Theme.bodyStrong }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.s2

                TextField {
                    id: joinUrlField
                    Layout.fillWidth: true
                    placeholderText: "Paste a Lichess link or game ID"
                }

                Button {
                    text: "Join"
                    highlighted: true
                    enabled: joinUrlField.text.trim() !== "" && !dialog._busy
                    onClicked: {
                        var gameId = lichessClient.gameIdFromUrl(joinUrlField.text.trim())
                        if (gameId === "") {
                            dialog._statusText = "Invalid URL or game ID"
                            return
                        }
                        dialog._busy       = true
                        dialog._statusText = ""
                        lichessClient.acceptChallenge(gameId)
                    }
                }
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
