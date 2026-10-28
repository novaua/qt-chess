import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AppDialog {
    id: dialog

    // Pre-fill for edit mode (ProfileDialog reuses this component)
    property string initialName:   ""
    property string initialAvatar: avatarNames[0]
    property bool   editMode:      false
    property bool   canCancel:     editMode

    readonly property var avatarNames: [
        "wizard","unicorn","sun","flower","rabbit","mouse","girl","boy","ball","star",
        "dragon","rocket","penguin","fox","bear","cat","ninja","pirate","alien","crown"
    ]

    property string _selectedAvatar: initialAvatar

    signal confirmed(string name, string avatarName)
    signal cancelled()

    title: editMode ? "Edit profile" : "Create profile"
    width: 380
    acceptEnabled: nameField.text.trim().length > 0
    rejectEnabled: canCancel

    onInitialNameChanged:   nameField.text = initialName
    onInitialAvatarChanged: _selectedAvatar = initialAvatar
    onOpened: {
        nameField.text = initialName
        _selectedAvatar = initialAvatar
        nameField.forceActiveFocus()
    }
    onAccepted: confirmed(nameField.text.trim(), _selectedAvatar)
    onRejected: cancelled()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.s3

        TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: "Your name"
            text: dialog.initialName
        }

        Label {
            text: "Choose your avatar"
            color: Theme.textSecondary
        }

        Grid {
            columns: 5
            spacing: Theme.s2
            Layout.alignment: Qt.AlignHCenter

            Repeater {
                model: dialog.avatarNames
                delegate: Rectangle {
                    readonly property bool isSelected: dialog._selectedAvatar === modelData
                    width: 52; height: 52
                    radius: Theme.radiusCard
                    color: isSelected ? Theme.selectedBg
                         : avatarMouse.containsMouse ? Theme.cardBgHover : "transparent"
                    border.color: isSelected ? Theme.accent : "transparent"
                    border.width: 2

                    Image {
                        anchors.centerIn: parent
                        width: 40; height: 40
                        source: "qrc:/app/pics/avatars/" + modelData + ".png"
                        fillMode: Image.PreserveAspectFit
                    }

                    MouseArea {
                        id: avatarMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: dialog._selectedAvatar = modelData
                    }
                }
            }
        }

        // Lichess section (profile mode only)
        ColumnLayout {
            id: lichessSection
            Layout.fillWidth: true
            spacing: Theme.s2
            visible: dialog.editMode

            property bool _validating: false
            property string _lichessStatus: ""   // "" | "ok:username" | "err"
            property bool _showTokenField: false

            Connections {
                target: chessConnector
                function onLichessLoginResult(ok, username) {
                    lichessSection._validating = false
                    lichessSection._lichessStatus = ok ? ("ok:" + username) : "err"
                }
            }
            Connections {
                target: lichessClient
                function onTokenValidated(ok, username) {
                    lichessSection._validating = false
                    if (ok) {
                        lichessSection._lichessStatus = "ok:" + username
                        userManager.saveLichessCredentials(lichessTokenField.text, username)
                    } else {
                        lichessSection._lichessStatus = "err"
                    }
                }
            }

            Label {
                text: "Lichess"
                font: Theme.bodyStrong
            }

            // Already connected: show username + disconnect button
            RowLayout {
                visible: userManager.lichessConnected && lichessSection._lichessStatus === ""
                spacing: Theme.s2
                Label {
                    text: "✓ " + userManager.lichessUsername
                    color: Theme.success
                }
                Button {
                    text: "Disconnect"
                    icon.source: Theme.icon("sign-out")
                    onClicked: {
                        userManager.clearLichessToken()
                        lichessSection._lichessStatus = ""
                        lichessSection._showTokenField = false
                    }
                }
            }

            // Primary: browser OAuth button
            RowLayout {
                visible: !userManager.lichessConnected || lichessSection._lichessStatus !== ""
                spacing: Theme.s2

                Button {
                    text: lichessSection._validating ? "Waiting for browser…" : "Sign in with Lichess"
                    icon.source: Theme.icon("globe")
                    enabled: !lichessSection._validating
                    onClicked: {
                        lichessSection._validating    = true
                        lichessSection._lichessStatus = ""
                        chessConnector.loginWithLichess()
                    }
                }

                Button {
                    flat: true
                    text: lichessSection._showTokenField ? "Hide API token" : "Use an API token"
                    visible: !lichessSection._validating
                    onClicked: lichessSection._showTokenField = !lichessSection._showTokenField
                }
            }

            // Fallback: manual API token (collapsed by default)
            RowLayout {
                visible: lichessSection._showTokenField &&
                         (!userManager.lichessConnected || lichessSection._lichessStatus !== "")
                Layout.fillWidth: true
                spacing: Theme.s2

                TextField {
                    id: lichessTokenField
                    Layout.fillWidth: true
                    placeholderText: "Paste your Lichess API token"
                    echoMode: TextInput.Password
                }

                Button {
                    text: "Validate"
                    enabled: lichessTokenField.text.trim() !== "" && !lichessSection._validating
                    onClicked: {
                        lichessSection._validating    = true
                        lichessSection._lichessStatus = ""
                        lichessClient.validateToken(lichessTokenField.text.trim())
                    }
                }
            }

            // Status row
            RowLayout {
                spacing: Theme.s2
                visible: lichessSection._validating || lichessSection._lichessStatus !== ""

                BusyIndicator {
                    Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    running: lichessSection._validating
                    visible: lichessSection._validating
                }

                Label {
                    visible: !lichessSection._validating
                    text: lichessSection._lichessStatus.startsWith("ok:")
                        ? ("✓ Connected as " + lichessSection._lichessStatus.substring(3))
                        : "✗ Sign-in failed"
                    color: lichessSection._lichessStatus.startsWith("ok:")
                        ? Theme.success : Theme.critical
                    font: Theme.caption
                }
            }
        }
    }

    // A hidden button still takes its slot in a DialogButtonBox, so there are two boxes.
    footer: canCancel ? _boxWithCancel : _boxCreateOnly
    property DialogButtonBox _boxWithCancel: DialogButtonBox {
        Button {
            text: dialog.editMode ? "Save" : "Create profile"
            highlighted: true
            enabled: dialog.acceptEnabled
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
    property DialogButtonBox _boxCreateOnly: DialogButtonBox {
        alignment: Qt.AlignRight
        Button {
            text: "Create profile"
            highlighted: true
            enabled: dialog.acceptEnabled
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
