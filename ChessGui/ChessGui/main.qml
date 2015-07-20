import QtQuick 2.4
import QtQuick.Controls 1.3

ApplicationWindow {
    title: qsTr("Chess ++")
    width: 800
    height: 600
    visible: true

    ChessGame {
        anchors.fill: parent
    }
}
