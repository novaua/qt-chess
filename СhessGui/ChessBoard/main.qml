import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import "utils1.js" as MyUtils1;
ApplicationWindow {
    title: qsTr("Hello World")
    width: 800
    height: 820
    visible: true

    Rectangle {
        width: 642; height: 642; color: "black"
        Grid {
            id: boardGrid
            x:5; y:5;
            columns: 8; rows: 8; spacing: 1

            Repeater { model: 64
                id: repeater
                Rectangle {
                    id: box
                    width: 70; height: 70

                    scale: mouseArea.containsMouse ? 0.8 : 1.0
                    smooth: mouseArea.containsMouse

                    color: ((Math.floor(index / 8) % 2) === 0)
                           ? (index % 2  === 1 ? "teal" : "white")
                           : (index % 2  === 0 ? "teal" : "white")

                    Text { text: index + 1
                        font.pointSize: 30
                        anchors.centerIn: parent
                    }

                    MouseArea
                    {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            console.log("Clicked index: " + index) ;
                            MyUtils1.handleBoxSelected(index, repeater);
                        }
                    }
                }
            }
        }
    }
}
