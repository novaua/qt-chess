import QtQuick
import QtQuick.Controls

Rectangle {
    id: loginScreen
    anchors.fill: parent

    color: Theme.windowBg

    Image {
        anchors.fill: parent
        source: "qrc:/app/pics/ChessBackground.jpg"
        fillMode: Image.PreserveAspectCrop
        opacity: Theme.homeBackdropOpacity
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.s5
        width: Math.min(parent.width - 2 * Theme.s7, 480)

        Text {
            text: "Chess++"
            font: Theme.titleLarge
            color: Theme.textPrimary
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "Who's playing?"
            font: Theme.subtitle
            color: Theme.textSecondary
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Flow {
            id: userFlow
            spacing: Theme.s3
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width

            Repeater {
                model: userManager.users
                delegate: Rectangle {
                    readonly property bool isActive: modelData.id === userManager.activeUserId

                    width: 96
                    height: 116
                    radius: Theme.radiusCard
                    color: isActive ? Theme.selectedBg
                         : cardMouse.containsMouse ? Theme.cardBgHover : Theme.cardBg
                    border.color: isActive || cardMouse.containsMouse ? Theme.accent : Theme.cardStroke
                    border.width: isActive ? 2 : 1

                    Column {
                        anchors.centerIn: parent
                        spacing: Theme.s2

                        Image {
                            width: 56; height: 56
                            source: modelData.avatarUrl
                            fillMode: Image.PreserveAspectFit
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Text {
                            text: modelData.name
                            color: Theme.textPrimary
                            font: isActive ? Theme.bodyStrong : Theme.body
                            horizontalAlignment: Text.AlignHCenter
                            width: 84
                            elide: Text.ElideRight
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }

                    MouseArea {
                        id: cardMouse
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onClicked: userManager.login(modelData.id)
                    }
                }
            }
        }

        Button {
            text: "Add profile"
            icon.source: Theme.icon("person-add")
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: loginScreen.addUserRequested()
        }
    }

    signal addUserRequested()
}
