import QtQuick 2.15
import QtQuick.Layouts 1.15

/*
 * ControlPanel.qml
 * Shows element detail (power / temperature / current) and
 * provides Power Off / Calibrate / Restart command buttons.
 *
 * Properties supplied by main.qml:
 *   dark, globalId, quadrant, localElem, cluster, localIdx
 *   cmdSender – CommandSender QObject*
 * Signal: closeRequested()
 */
Item {
    id: panel

    property bool dark:      true
    property int  globalId:  0
    property int  quadrant:  0
    property int  localElem: 0
    property int  cluster:   0
    property int  localIdx:  0
    property var  cmdSender: null
    property var  model:     null     // RadarModel* passed from main.qml

    signal closeRequested()

    // ── Derived element data (live, re-reads from model on open) ──
    property var detail: ({})   // elementDetail map

    // Refresh detail from model whenever the panel is shown for an element
    function refreshDetail() {
        if (!panel.model) return
        detail = panel.model.elementDetail(quadrant, localElem)
    }

    // ── Theme ─────────────────────────────────────────────────────
    readonly property color bg:       dark ? "#0b1828" : "#ffffff"
    readonly property color surface:  dark ? "#0d1e30" : "#f4f8fc"
    readonly property color border2:  dark ? "#1e3050" : "#c8dcea"
    readonly property color accent2:  dark ? "#1ab4f0" : "#0070c0"
    readonly property color textCol:  dark ? "#d8eaf8" : "#1a2840"
    readonly property color dimCol:   dark ? "#4a6888" : "#7090b0"
    readonly property color healthy:  dark ? "#00e87a" : "#00a854"
    readonly property color warning2: dark ? "#ffb700" : "#e07800"
    readonly property color critical: dark ? "#ff3a3a" : "#d41010"
    readonly property color nodata:   dark ? "#2a3e58" : "#b0c8e0"

    // Status 0-3 → colour
    function stCol(s) {
        if (s <= 0) return nodata
        if (s === 1) return healthy
        if (s === 2) return warning2
        return critical
    }
    function stLabel(s) {
        if (s <= 0) return "NO DATA"
        if (s === 1) return "HEALTHY"
        if (s === 2) return "WARNING"
        return "CRITICAL"
    }
    // Overall status = worst of the three param statuses
    function overallStatus() {
        return Math.max(detail.powerStatus||0, detail.tempStatus||0, detail.currStatus||0)
    }

    function fmtPwr(v) { return (!v || v <= -900) ? "—" : parseFloat(v).toFixed(1) + " dBm" }
    function fmtTmp(v) { return (!v || v <= -900) ? "—" : parseFloat(v).toFixed(1) + " °C"  }
    function fmtCur(v) { return (!v || v <= -900) ? "—" : parseFloat(v).toFixed(3) + " A"   }

    // ── Card ───────────────────────────────────────────────────────
    Rectangle {
        id: card
        anchors.centerIn: parent
        width:  360
        height: cardCol.implicitHeight + 32
        radius: 12
        color:  panel.bg
        border { color: panel.accent2; width: 1 }

        // Glow ring
        Rectangle {
            anchors { fill: parent; margins: -3 }
            radius: parent.radius + 3
            color: "transparent"
            border { color: Qt.rgba(0.1, 0.71, 0.94, 0.18); width: 5 }
            z: -1
        }

        // Top status stripe
        Rectangle {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 4; radius: parent.radius
            color: panel.stCol(panel.overallStatus())
        }

        Column {
            id: cardCol
            anchors { top: parent.top; left: parent.left; right: parent.right; margins: 16; topMargin: 22 }
            spacing: 0

            // ── Header ─────────────────────────────────────────────
            RowLayout {
                width: parent.width

                Column {
                    spacing: 3
                    Text {
                        text: "Element  " + panel.globalId
                        font { family: "Segoe UI"; pixelSize: 17; bold: true }
                        color: panel.textCol
                    }
                    Text {
                        text: "Q" + (panel.quadrant+1) + "  ·  Cluster " + (panel.cluster+1) + "  ·  Local #" + panel.localIdx
                        font { family: "Segoe UI"; pixelSize: 11 }
                        color: panel.dimCol
                    }
                }

                Item { Layout.fillWidth: true }

                // Overall status badge
                Rectangle {
                    width:  badgeRow.implicitWidth + 18
                    height: 28; radius: 14
                    color:  Qt.rgba(panel.stCol(panel.overallStatus()).r,
                                    panel.stCol(panel.overallStatus()).g,
                                    panel.stCol(panel.overallStatus()).b, 0.18)
                    border { color: panel.stCol(panel.overallStatus()); width: 1 }
                    Row {
                        id: badgeRow
                        anchors.centerIn: parent; spacing: 5
                        Rectangle {
                            width: 7; height: 7; radius: 3.5
                            color: panel.stCol(panel.overallStatus())
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: panel.stLabel(panel.overallStatus())
                            font { family: "Segoe UI"; pixelSize: 10; bold: true }
                            color: panel.stCol(panel.overallStatus())
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                // Close button
                Rectangle {
                    width: 28; height: 28; radius: 14
                    color: "transparent"
                    border { color: panel.border2; width: 1 }
                    Text { anchors.centerIn: parent; text: "✕"; font.pixelSize: 13; color: panel.dimCol }
                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: panel.closeRequested()
                    }
                }
            }

            Item { height: 12 }
            Rectangle { width: parent.width; height: 1; color: panel.border2; opacity: 0.6 }
            Item { height: 12 }

            // ── Three-parameter table ──────────────────────────────
            Text {
                text: "LIVE MEASUREMENTS"
                font { family: "Segoe UI"; pixelSize: 10; letterSpacing: 1.2 }
                color: panel.dimCol
            }
            Item { height: 10 }

            // Power row
            ParamRow {
                icon: "⚡"; label: "Forward Power"
                value: panel.fmtPwr(panel.detail.power)
                threshold: "threshold ≥ 43 dBm"
                statusColor: panel.stCol(panel.detail.powerStatus || 0)
                width: parent.width
            }
            Item { height: 6 }
            // Temperature row
            ParamRow {
                icon: "🌡"; label: "Temperature"
                value: panel.fmtTmp(panel.detail.temperature)
                threshold: "threshold ≤ 50 °C"
                statusColor: panel.stCol(panel.detail.tempStatus || 0)
                width: parent.width
            }
            Item { height: 6 }
            // Current row
            ParamRow {
                icon: "⚡"; label: "Supply Current"
                value: panel.fmtCur(panel.detail.current)
                threshold: "threshold ≤ 2.0 A"
                statusColor: panel.stCol(panel.detail.currStatus || 0)
                width: parent.width
            }

            Item { height: 14 }
            Rectangle { width: parent.width; height: 1; color: panel.border2; opacity: 0.6 }
            Item { height: 12 }

            // ── Command buttons ────────────────────────────────────
            Text {
                text: "SEND COMMAND"
                font { family: "Segoe UI"; pixelSize: 10; letterSpacing: 1.2 }
                color: panel.dimCol
            }
            Item { height: 10 }

            Row {
                width: parent.width; spacing: 10

                CmdButton {
                    cmdId:   1; label: "Power Off"; icon: "⏻"
                    bgColor: panel.dark ? "#2a0808" : "#ffe8e8"
                    bdColor: panel.critical; txColor: panel.critical
                    width:   (parent.width - 20) / 3
                    onSend:  panel.dispatchCmd(cmdId)
                }
                CmdButton {
                    cmdId:   2; label: "Calibrate"; icon: "⚙"
                    bgColor: panel.dark ? "#0a2448" : "#e0f0ff"
                    bdColor: panel.accent2; txColor: panel.accent2
                    width:   (parent.width - 20) / 3
                    onSend:  panel.dispatchCmd(cmdId)
                }
                CmdButton {
                    cmdId:   3; label: "Restart"; icon: "↺"
                    bgColor: panel.dark ? "#1a2808" : "#e8f8e8"
                    bdColor: panel.healthy; txColor: panel.healthy
                    width:   (parent.width - 20) / 3
                    onSend:  panel.dispatchCmd(cmdId)
                }
            }

            Item { height: 12 }

            // ── Feedback bar ───────────────────────────────────────
            Rectangle {
                id: feedbackBar
                width: parent.width; height: 36; radius: 7
                color: panel.surface
                border { color: panel.border2; width: 1 }
                visible: fbText.text !== ""

                Row {
                    anchors { fill: parent; margins: 10 }
                    spacing: 8
                    Rectangle {
                        width: 7; height: 7; radius: 3.5
                        color: feedbackBar.fbColor
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        id: fbText; text: ""
                        font { family: "Segoe UI"; pixelSize: 11 }
                        color: feedbackBar.fbColor
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                property color fbColor: panel.dimCol
            }

            Item { height: 4 }
        }
    }

    // ── Command dispatch ──────────────────────────────────────────
    function dispatchCmd(cmd) {
        var names = ["", "POWER OFF", "CALIBRATE", "RESTART"]
        var name  = (cmd >= 1 && cmd <= 3) ? names[cmd] : "CMD"
        if (!panel.cmdSender) {
            showFeedback("✗  " + name + "  —  no sender", panel.critical)
            return
        }
        var ok = panel.cmdSender.sendCommand(cmd, panel.quadrant, panel.localElem)
        if (ok) showFeedback("↑  " + name + "  →  sent to E" + panel.globalId, panel.healthy)
        else    showFeedback("✗  " + name + "  —  send failed", panel.critical)
    }
    function showFeedback(msg, col) {
        fbText.text = msg
        feedbackBar.fbColor = col
    }

    // ACK handler
    Connections {
        target: panel.cmdSender
        function onCommandAcknowledged(command, quadrant, elementId, status) {
            var gid = quadrant * (panel.model ? panel.model.elementsPerQuadrant : 256) + elementId
            if (gid !== panel.globalId) return
            var n = ["","POWER OFF","CALIBRATE","RESTART"]
            var name = (command>=1&&command<=3) ? n[command] : "CMD"
            if (status === 0) panel.showFeedback("✓  " + name + "  —  ACK OK",    panel.healthy)
            else if (status === 1) panel.showFeedback("⚠  " + name + "  —  BUSY", panel.warning2)
            else panel.showFeedback("✗  " + name + "  —  ERROR",                  panel.critical)
        }
    }


    // ── Entrance animation ─────────────────────────────────────────
    opacity: 0; scale: 0.90; transformOrigin: Item.Center

    function show() {
        panel.refreshDetail()
        opacity = 1; scale = 1.0
        fbText.text = ""
    }
    function hide() { opacity = 0; scale = 0.90 }

    Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
    Behavior on scale   { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
}
