import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Window

Rectangle {
    id: dialog

    readonly property bool isDarkMode: {
        if (!Window.window) return false
        var c = Window.window.color
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) <= 0.5
    }

    property string _gameUrl:    ""
    property bool   _waiting:    false
    property string _statusText: ""

    signal closeRequested()

    width: 380
    height: dialogContent.implicitHeight + 48
    color:        isDarkMode ? "#252525" : "#f4f4f4"
    border.color: isDarkMode ? "#555555" : "#cccccc"
    border.width: 1
    radius: 12

    Connections {
        target: lichessClient

        function onChallengeCreated(gameId, joinUrl) {
            dialog._gameUrl    = joinUrl
            dialog._waiting    = true
            dialog._statusText = "Waiting for opponent…"
        }

        function onGameStarted(gameId, playingAsWhite, opponentName, opponentAvatarUrl) {
            dialog._waiting    = false
            dialog._statusText = ""
            dialog.closeRequested()
        }

        function onNetworkError(message) {
            dialog._waiting    = false
            dialog._statusText = "Error: " + message
        }
    }

    Column {
        id: dialogContent
        anchors.centerIn: parent
        width: parent.width - 40
        spacing: 16

        Text {
            text: "Lichess Online"
            font.pixelSize: 20
            font.bold: true
            color: isDarkMode ? "#ffffff" : "#000000"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // ── No token configured ─────────────────────────────────────────────
        Column {
            visible: !userManager.lichessConnected
            width: parent.width
            spacing: 10

            Text {
                text: "Connect your Lichess account in Profile to play online."
                color: isDarkMode ? "#dddddd" : "#444444"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                text: "Open Profile"
                anchors.horizontalCenter: parent.horizontalCenter
                implicitWidth: 120
                implicitHeight: 32
                onClicked: {
                    _showProfile = true
                    dialog.closeRequested()
                }
            }
        }

        // ── Connected — create + join ────────────────────────────────────────
        Column {
            visible: userManager.lichessConnected
            width: parent.width
            spacing: 16

            // ── Challenge a Friend ──────────────────────────────────────────
            Column {
                width: parent.width
                spacing: 8

                Text {
                    text: "Game setup"
                    font.pixelSize: 15
                    font.bold: true
                    color: isDarkMode ? "#ffffff" : "#000000"
                }

                // Game type
                Text {
                    text: "Game type"
                    font.pixelSize: 12
                    color: isDarkMode ? "#aaaaaa" : "#666666"
                }

                QQC.ComboBox {
                    id: comboVariant
                    model: ["Standard", "Chess960"]
                    implicitWidth: 140
                    implicitHeight: 30
                    contentItem: Text {
                        leftPadding: 8
                        text: comboVariant.displayText
                        font: comboVariant.font
                        color: isDarkMode ? "#ffffff" : "#000000"
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 4
                        color: isDarkMode ? "#3c3c3c" : "#ffffff"
                        border.color: isDarkMode ? "#555555" : "#cccccc"
                        border.width: 1
                    }
                    popup: QQC.Popup {
                        y: comboVariant.height
                        width: comboVariant.width
                        padding: 1
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: comboVariant.delegateModel
                        }
                        background: Rectangle {
                            color: isDarkMode ? "#3c3c3c" : "#ffffff"
                            border.color: isDarkMode ? "#555555" : "#cccccc"
                            border.width: 1
                            radius: 4
                        }
                    }
                }

                // Time control
                Text {
                    text: "Time control"
                    font.pixelSize: 12
                    color: isDarkMode ? "#aaaaaa" : "#666666"
                }

                Row {
                    spacing: 6
                    QQC.ButtonGroup { id: tcTypeGroup }

                    Repeater {
                        model: [{ label: "Unlimited", realTime: false },
                                { label: "Real time",  realTime: true  }]
                        delegate: QQC.Button {
                            text: modelData.label
                            checkable: true
                            checked: index === 0
                            QQC.ButtonGroup.group: tcTypeGroup
                            implicitWidth: 84
                            implicitHeight: 28
                            background: Rectangle {
                                radius: 4
                                color: parent.checked ? "#0078d4"
                                     : parent.down ? (isDarkMode ? "#282828" : "#c0c0c0")
                                     : (isDarkMode ? "#3c3c3c" : "#e0e0e0")
                                border.color: parent.checked ? "#005a9e"
                                            : (isDarkMode ? "#555555" : "#bbbbbb")
                            }
                            contentItem: Text {
                                text: parent.text
                                color: parent.checked ? "#ffffff"
                                     : (isDarkMode ? "#ffffff" : "#000000")
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                            }
                        }
                    }
                }

                // Preset time controls — shown only for Real time
                Row {
                    visible: tcTypeGroup.checkedButton !== null
                             && tcTypeGroup.checkedButton.text === "Real time"
                    spacing: 6
                    QQC.ButtonGroup { id: tcGroup }

                    Repeater {
                        id: tcChips
                        model: [{ label: "1+0",   min: 1,  inc: 0 },
                                { label: "3+2",   min: 3,  inc: 2 },
                                { label: "5+3",   min: 5,  inc: 3 },
                                { label: "10+0",  min: 10, inc: 0 },
                                { label: "15+10", min: 15, inc: 10 }]
                        delegate: QQC.Button {
                            text: modelData.label
                            checkable: true
                            checked: index === 2
                            QQC.ButtonGroup.group: tcGroup
                            implicitWidth: 54
                            implicitHeight: 28
                            background: Rectangle {
                                radius: 4
                                color: parent.checked ? "#0078d4"
                                     : parent.down ? (isDarkMode ? "#282828" : "#c0c0c0")
                                     : (isDarkMode ? "#3c3c3c" : "#e0e0e0")
                                border.color: parent.checked ? "#005a9e"
                                            : (isDarkMode ? "#555555" : "#bbbbbb")
                            }
                            contentItem: Text {
                                text: parent.text
                                color: parent.checked ? "#ffffff"
                                     : (isDarkMode ? "#ffffff" : "#000000")
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                            }
                        }
                    }
                }

                // Side selection
                Text {
                    text: "Side"
                    font.pixelSize: 12
                    color: isDarkMode ? "#aaaaaa" : "#666666"
                }

                Row {
                    spacing: 6
                    QQC.ButtonGroup { id: colorGroup }

                    Repeater {
                        id: colorChips
                        model: [{ label: "Random ⇄", value: "random" },
                                { label: "White ♔",  value: "white"  },
                                { label: "Black ♚",  value: "black"  }]
                        delegate: QQC.Button {
                            text: modelData.label
                            checkable: true
                            checked: index === 0
                            QQC.ButtonGroup.group: colorGroup
                            implicitWidth: 80
                            implicitHeight: 28
                            background: Rectangle {
                                radius: 4
                                color: parent.checked ? "#0078d4"
                                     : parent.down ? (isDarkMode ? "#282828" : "#c0c0c0")
                                     : (isDarkMode ? "#3c3c3c" : "#e0e0e0")
                                border.color: parent.checked ? "#005a9e"
                                            : (isDarkMode ? "#555555" : "#bbbbbb")
                            }
                            contentItem: Text {
                                text: parent.text
                                color: parent.checked ? "#ffffff"
                                     : (isDarkMode ? "#ffffff" : "#000000")
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                            }
                        }
                    }
                }

                Button {
                    text: "Challenge a Friend"
                    implicitWidth: 160
                    implicitHeight: 32
                    enabled: !dialog._waiting
                    font.bold: true
                    background: Rectangle {
                        radius: 4
                        color: parent.enabled
                            ? (parent.down ? "#005a9e" : "#0078d4")
                            : (isDarkMode ? "#2a2a2a" : "#cccccc")
                        border.color: parent.enabled ? "#005a9e" : "transparent"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "#ffffff" : (isDarkMode ? "#666" : "#aaa")
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment:   Text.AlignVCenter
                    }
                    onClicked: {
                        var isRealTime = tcTypeGroup.checkedButton !== null
                                        && tcTypeGroup.checkedButton.text === "Real time"
                        var mins = 0
                        var inc  = 0
                        if (isRealTime) {
                            var minsArr = [1, 3, 5, 10, 15]
                            var incArr  = [0, 2, 3,  0, 10]
                            for (var i = 0; i < tcGroup.buttons.length; i++) {
                                if (tcGroup.buttons[i].checked) {
                                    mins = minsArr[i]
                                    inc  = incArr[i]
                                    break
                                }
                            }
                        }
                        var colorValues = ["random", "white", "black"]
                        var chosenColor = "random"
                        for (var j = 0; j < colorGroup.buttons.length; j++) {
                            if (colorGroup.buttons[j].checked) {
                                chosenColor = colorValues[j]
                                break
                            }
                        }
                        var variantValues = ["standard", "chess960"]
                        var chosenVariant = variantValues[comboVariant.currentIndex] || "standard"
                        lichessClient.createOpenChallenge(mins, inc, chosenColor, chosenVariant)
                    }
                }

                // URL + QR after challenge created
                Column {
                    visible: dialog._gameUrl !== ""
                    width: parent.width
                    spacing: 8

                    Row {
                        width: parent.width
                        spacing: 6

                        QQC.TextField {
                            id: urlField
                            text: dialog._gameUrl
                            readOnly: true
                            width: parent.width - copyBtn.width - 6
                            color: isDarkMode ? "#ffffff" : "#000000"
                            background: Rectangle {
                                radius: 4
                                color:        isDarkMode ? "#3c3c3c" : "#ffffff"
                                border.color: isDarkMode ? "#555555" : "#cccccc"
                                border.width: 1
                            }
                        }

                        Button {
                            id: copyBtn
                            text: "Copy"
                            implicitWidth: 56
                            implicitHeight: urlField.height
                            onClicked: {
                                urlField.selectAll()
                                urlField.copy()
                            }
                        }
                    }

                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 160; height: 160
                        source: dialog._gameUrl !== ""
                            ? ("https://api.qrserver.com/v1/create-qr-code/?size=160x160&data="
                               + encodeURIComponent(dialog._gameUrl))
                            : ""
                        fillMode: Image.PreserveAspectFit
                    }
                }

                Row {
                    visible: dialog._waiting || dialog._statusText !== ""
                    spacing: 8

                    QQC.BusyIndicator {
                        width: 20; height: 20
                        running: dialog._waiting
                        visible: dialog._waiting
                    }

                    Text {
                        text: dialog._statusText
                        color: dialog._statusText.startsWith("Error")
                            ? "#e74c3c"
                            : (isDarkMode ? "#dddddd" : "#444444")
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // ── Divider ──────────────────────────────────────────────────────
            Rectangle {
                width: parent.width; height: 1
                color: isDarkMode ? "#444444" : "#dddddd"
            }

            // ── Join game ────────────────────────────────────────────────────
            Column {
                width: parent.width
                spacing: 8

                Text {
                    text: "Join Game"
                    font.pixelSize: 15
                    font.bold: true
                    color: isDarkMode ? "#ffffff" : "#000000"
                }

                Row {
                    width: parent.width
                    spacing: 6

                    QQC.TextField {
                        id: joinUrlField
                        width: parent.width - joinBtn.width - 6
                        placeholderText: "Paste Lichess URL or game ID"
                        color: isDarkMode ? "#ffffff" : "#000000"
                        placeholderTextColor: isDarkMode ? "#888" : "#aaa"
                        background: Rectangle {
                            radius: 4
                            color:        isDarkMode ? "#3c3c3c" : "#ffffff"
                            border.color: joinUrlField.activeFocus ? "#0078d4"
                                        : (isDarkMode ? "#555555" : "#cccccc")
                            border.width: 1
                        }
                    }

                    Button {
                        id: joinBtn
                        text: "Join"
                        implicitWidth: 56
                        implicitHeight: joinUrlField.height
                        enabled: joinUrlField.text.trim() !== "" && !dialog._waiting
                        font.bold: true
                        background: Rectangle {
                            radius: 4
                            color: parent.enabled
                                ? (parent.down ? "#005a9e" : "#0078d4")
                                : (isDarkMode ? "#2a2a2a" : "#cccccc")
                            border.color: parent.enabled ? "#005a9e" : "transparent"
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.enabled ? "#ffffff" : (isDarkMode ? "#666" : "#aaa")
                            font: parent.font
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment:   Text.AlignVCenter
                        }
                        onClicked: {
                            var gameId = lichessClient.gameIdFromUrl(joinUrlField.text.trim())
                            if (gameId === "") {
                                dialog._statusText = "Invalid URL or game ID"
                                return
                            }
                            dialog._waiting    = true
                            dialog._statusText = "Connecting…"
                            lichessClient.acceptChallenge(gameId)
                        }
                    }
                }
            }
        }

        // ── Close button ─────────────────────────────────────────────────────
        Button {
            text: "Cancel"
            implicitWidth: 90
            implicitHeight: 32
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: {
                lichessClient.stopStream()
                dialog._waiting    = false
                dialog._gameUrl    = ""
                dialog._statusText = ""
                dialog.closeRequested()
            }
        }
    }
}
