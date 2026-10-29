import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import ChessPlusPlus

// "Who's playing?": the profiles, centred, with the Lichess account and when each last played.
// Shown at start with several profiles and after Switch user. Each card's menu exports or
// deletes that profile; ChessGame.qml owns the dialogs.
Page {
    id: page
    objectName: "login"

    signal addUserRequested()
    signal importRequested()
    signal exportRequested(string userId)
    signal deleteRequested(string userId)

    readonly property int _cardWidth:  128
    readonly property int _cardHeight: 156

    background: Rectangle {
        color: Theme.windowBg
        Image {
            anchors.fill: parent
            source: "qrc:/app/pics/ChessBackground.jpg"
            fillMode: Image.PreserveAspectCrop
            opacity: Theme.homeBackdropOpacity
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: Math.max(height, content.implicitHeight + 2 * Theme.s6)
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: content
            width: Math.min(flick.width - 2 * Theme.s5, 720)
            x: (flick.width - width) / 2
            y: Math.max(Theme.s6, (flick.height - implicitHeight) / 2)
            spacing: Theme.s4

            Label {
                text: "Chess++"
                font: Theme.titleLarge
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: "Who's playing?"
                font: Theme.subtitle
                color: Theme.textSecondary
                Layout.alignment: Qt.AlignHCenter
            }

            // A Flow can't centre its rows: it is as wide as the cards that fit, and centred itself.
            Flow {
                id: cards
                readonly property int perRow: Math.max(1, Math.floor((content.width + spacing) / (page._cardWidth + spacing)))
                readonly property int count: UserManager.userCount
                spacing: Theme.s3
                Layout.preferredWidth: Math.min(count, perRow) * (page._cardWidth + spacing) - spacing
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Theme.s2

                Repeater {
                    model: UserManager.users
                    delegate: AbstractButton {
                        id: card
                        required property var modelData
                        readonly property bool isLast: modelData.id === UserManager.activeUserId

                        width: page._cardWidth; height: page._cardHeight
                        hoverEnabled: true
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: modelData.name
                        Accessible.description: details.text
                        onClicked: UserManager.login(modelData.id)

                        background: Rectangle {
                            radius: Theme.radiusCard
                            color: card.hovered || card.down ? Theme.cardBgHover : Theme.cardBg
                            border.color: card.visualFocus || card.isLast || (Theme.highContrast && card.hovered) ? Theme.accent : Theme.cardStroke
                            border.width: card.visualFocus ? 2 : 1
                        }

                        contentItem: ColumnLayout {
                            spacing: Theme.s1

                            Image {
                                source: card.modelData.avatarUrl
                                sourceSize.width: 64; sourceSize.height: 64
                                fillMode: Image.PreserveAspectFit
                                Layout.alignment: Qt.AlignHCenter
                                Layout.topMargin: Theme.s3
                            }
                            Label {
                                text: card.modelData.name
                                font: Theme.bodyStrong
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                Layout.leftMargin: Theme.s2; Layout.rightMargin: Theme.s2
                            }
                            RowLayout {
                                spacing: 2
                                visible: card.modelData.lichessUsername !== ""
                                Layout.alignment: Qt.AlignHCenter
                                IconImage {
                                    source: Theme.icon("globe")
                                    sourceSize.width: 12; sourceSize.height: 12
                                    color: Theme.accent
                                }
                                Label {
                                    text: card.modelData.lichessUsername
                                    font: Theme.caption
                                    color: Theme.textSecondary
                                    elide: Text.ElideRight
                                    Layout.maximumWidth: page._cardWidth - 2 * Theme.s2 - 14
                                }
                            }
                            Label {
                                id: details
                                text: card.modelData.lastPlayed !== "" ? "Played " + card.modelData.lastPlayed.toLowerCase()
                                                                        : "New profile"
                                font: Theme.caption
                                color: Theme.textSecondary
                                horizontalAlignment: Text.AlignHCenter
                                Layout.fillWidth: true
                            }
                            Item { Layout.fillHeight: true }
                        }

                        IconButton {
                            anchors { top: parent.top; right: parent.right; margins: 2 }
                            dense: true
                            // Hidden, not invisible: Tab still reaches it after the card.
                            opacity: card.hovered || card.visualFocus || visualFocus || cardMenu.visible ? 1 : 0
                            icon.source: Theme.icon("more")
                            tip: "More for " + card.modelData.name
                            onClicked: cardMenu.popup(this, 0, height)

                            Menu {
                                id: cardMenu
                                MenuItem {
                                    text: "Export profile…"
                                    icon.source: Theme.icon("export")
                                    onTriggered: page.exportRequested(card.modelData.id)
                                }
                                MenuItem {
                                    text: "Delete profile…"
                                    icon.source: Theme.icon("delete")
                                    onTriggered: page.deleteRequested(card.modelData.id)
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                spacing: Theme.s2
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Theme.s2

                Button {
                    text: "Add profile"
                    icon.source: Theme.icon("person-add")
                    onClicked: page.addUserRequested()
                }
                Button {
                    text: "Import profile…"
                    icon.source: Theme.icon("import")
                    flat: true
                    onClicked: page.importRequested()
                }
            }
        }
    }
}
