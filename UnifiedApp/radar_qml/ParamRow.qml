import QtQuick 2.15

// Single measurement row in ControlPanel
Rectangle {
    id: row

    property string icon:        ""
    property string label:       ""
    property string value:       "—"
    property string threshold:   ""
    property color  statusColor: "#2a3e58"

    height: 46; radius: 8
    color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.07)
    border { color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.30); width: 1 }

    // Left colour stripe
    Rectangle { width: 3; height: parent.height; radius: 1; color: statusColor }

    Column {
        anchors { left: parent.left; right: parent.right; leftMargin: 14; topMargin: 7 }
        spacing: 1
        Row {
            spacing: 6
            Text { text: row.icon;  font.pixelSize: 13; color: row.statusColor; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: row.label
                font { family: "Segoe UI"; pixelSize: 11 }
                color: Qt.rgba(0.55,0.75,0.85,1.0)
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Text {
            text: row.value
            font { family: "Courier New"; pixelSize: 14; bold: true }
            color: row.statusColor
        }
    }
    Text {
        anchors { right: parent.right; bottom: parent.bottom; rightMargin: 10; bottomMargin: 5 }
        text: row.threshold
        font { family: "Segoe UI"; pixelSize: 9; italic: true }
        color: Qt.rgba(0.29, 0.41, 0.53, 0.55)
    }
}
