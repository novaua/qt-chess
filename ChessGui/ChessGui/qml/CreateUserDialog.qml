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

    // Pre-fill for edit mode (ProfileDialog reuses this component)
    property string initialName:   ""
    property string initialAvatar: avatarNames[0]
    property bool   editMode:      false

    readonly property var avatarNames: [
        "wizard","unicorn","sun","flower","rabbit","mouse","girl","boy","ball","star",
        "dragon","rocket","penguin","fox","bear","cat","ninja","pirate","alien","crown"
    ]

    property string _selectedAvatar: initialAvatar

    signal confirmed(string name, string avatarName)
    signal cancelled()

    width: 360
    height: content.implicitHeight + 48
    color:        isDarkMode ? "#252525" : "#f4f4f4"
    border.color: isDarkMode ? "#555555" : "#cccccc"
    border.width: 1
    radius: 12

    onInitialNameChanged:   nameField.text = initialName
    onInitialAvatarChanged: _selectedAvatar = initialAvatar

    Column {
        id: content
        anchors.centerIn: parent
        width: parent.width - 40
        spacing: 14

        Text {
            text: dialog.editMode ? "Edit Profile" : "Create Profile"
            font.pixelSize: 20
            font.bold: true
            color: isDarkMode ? "#ffffff" : "#000000"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        QQC.TextField {
            id: nameField
            width: parent.width
            placeholderText: "Your name"
            text: dialog.initialName
            color: isDarkMode ? "#ffffff" : "#000000"
            placeholderTextColor: isDarkMode ? "#888888" : "#aaaaaa"
            background: Rectangle {
                radius: 4
                color:        isDarkMode ? "#3c3c3c" : "#ffffff"
                border.color: nameField.activeFocus
                    ? "#0078d4"
                    : (isDarkMode ? "#555555" : "#cccccc")
                border.width: 1
            }
        }

        Text {
            text: "Choose your avatar"
            font.pixelSize: 13
            color: isDarkMode ? "#bbbbbb" : "#555555"
        }

        Grid {
            columns: 5
            spacing: 8
            anchors.horizontalCenter: parent.horizontalCenter

            Repeater {
                model: dialog.avatarNames
                delegate: Rectangle {
                    width: 52; height: 52
                    radius: 8
                    color: dialog._selectedAvatar === modelData
                        ? (isDarkMode ? "#1a4a7a" : "#cce4ff")
                        : (isDarkMode ? "#3c3c3c" : "#f0f0f0")
                    border.color: dialog._selectedAvatar === modelData ? "#0078d4" : "transparent"
                    border.width: 2

                    Image {
                        anchors.centerIn: parent
                        width: 40; height: 40
                        source: "qrc:/app/pics/avatars/" + modelData + ".png"
                        fillMode: Image.PreserveAspectFit
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: dialog._selectedAvatar = modelData
                    }
                }
            }
        }

        // Lichess token row (profile mode only)
        Column {
            id: lichessSection
            width: parent.width
            spacing: 6
            visible: dialog.editMode

            property bool _validating: false
            property string _lichessStatus: ""   // "" | "ok:username" | "err"

            Connections {
                target: lichessClient
                function onTokenValidated(ok, username) {
                    lichessSection._validating = false
                    if (ok) {
                        lichessSection._lichessStatus = "ok:" + username
                        userManager.saveLichessCredentials(
                            lichessTokenField.text,
                            username)
                    } else {
                        lichessSection._lichessStatus = "err"
                    }
                }
            }

            Text {
                text: "Lichess Token"
                font.pixelSize: 13
                color: isDarkMode ? "#bbbbbb" : "#555555"
            }

            // Show connected state OR token input
            Column {
                width: parent.width
                spacing: 6

                // Already connected: show username + disconnect button
                Row {
                    visible: userManager.lichessConnected && lichessSection._lichessStatus === ""
                    spacing: 8
                    Text {
                        text: "✓ " + userManager.lichessUsername
                        color: "#27ae60"
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Button {
                        text: "Disconnect"
                        implicitWidth: 90
                        implicitHeight: 26
                        onClicked: {
                            userManager.clearLichessToken()
                            lichessSection._lichessStatus = ""
                        }
                    }
                }

                // Token input row
                Row {
                    visible: !userManager.lichessConnected || lichessSection._lichessStatus !== ""
                    width: parent.width
                    spacing: 6

                    QQC.TextField {
                        id: lichessTokenField
                        width: parent.width - validateBtn.width - 6
                        placeholderText: "Paste your Lichess API token"
                        echoMode: TextInput.Password
                        color: isDarkMode ? "#ffffff" : "#000000"
                        placeholderTextColor: isDarkMode ? "#888" : "#aaa"
                        background: Rectangle {
                            radius: 4
                            color:        isDarkMode ? "#3c3c3c" : "#ffffff"
                            border.color: lichessTokenField.activeFocus ? "#0078d4"
                                        : (isDarkMode ? "#555555" : "#cccccc")
                            border.width: 1
                        }
                    }

                    Button {
                        id: validateBtn
                        text: "Validate"
                        implicitWidth: 74
                        implicitHeight: lichessTokenField.height
                        enabled: lichessTokenField.text.trim() !== "" && !lichessSection._validating
                        onClicked: {
                            lichessSection._validating    = true
                            lichessSection._lichessStatus = ""
                            lichessClient.validateToken(lichessTokenField.text.trim())
                        }
                    }
                }

                // Validation status
                Row {
                    spacing: 6
                    visible: lichessSection._validating || lichessSection._lichessStatus !== ""

                    QQC.BusyIndicator {
                        width: 16; height: 16
                        running: lichessSection._validating
                        visible: lichessSection._validating
                    }

                    Text {
                        visible: !lichessSection._validating
                        text: lichessSection._lichessStatus.startsWith("ok:")
                            ? ("✓ Connected as " + lichessSection._lichessStatus.substring(3))
                            : "✗ Invalid token"
                        color: lichessSection._lichessStatus.startsWith("ok:")
                            ? "#27ae60" : "#e74c3c"
                        font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        Row {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: dialog.editMode ? "Save" : "Create"
                implicitWidth: 100
                implicitHeight: 32
                font.bold: true
                enabled: nameField.text.trim().length > 0
                background: Rectangle {
                    radius: 4
                    color: parent.enabled
                        ? (parent.down ? "#005a9e" : "#0078d4")
                        : (isDarkMode ? "#2a2a2a" : "#cccccc")
                    border.color: parent.enabled ? "#005a9e" : "transparent"
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "#ffffff" : (isDarkMode ? "#666666" : "#aaaaaa")
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: dialog.confirmed(nameField.text.trim(), dialog._selectedAvatar)
            }

            Button {
                text: "Cancel"
                implicitWidth: 80
                implicitHeight: 32
                visible: dialog.editMode
                onClicked: dialog.cancelled()
            }
        }
    }
}
