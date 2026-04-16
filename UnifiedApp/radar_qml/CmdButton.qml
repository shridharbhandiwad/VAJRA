import QtQuick 2.15

Rectangle {
    id: btn

    property int    cmdId:   0
    property string label:   ""
    property string icon:    ""
    property color  bgColor: "transparent"
    property color  bdColor: "white"
    property color  txColor: "white"

    signal send()

    height: 68
    radius: 8
    color:  bgColor
    border { color: bdColor; width: 1 }

    Column {
        anchors.centerIn: parent
        spacing: 4
        Text {
            text: btn.icon
            font { family: "Segoe UI"; pixelSize: 22 }
            color: btn.txColor
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            text: btn.label
            font { family: "Segoe UI"; pixelSize: 11; bold: true }
            color: btn.txColor
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    property bool pressed2: false
    scale: pressed2 ? 0.95 : 1.0
    Behavior on scale { NumberAnimation { duration: 80 } }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed:  btn.pressed2 = true
        onReleased: btn.pressed2 = false
        onClicked:  btn.send()
    }
}
