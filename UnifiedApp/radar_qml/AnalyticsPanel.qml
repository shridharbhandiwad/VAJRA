import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Item {
    id: ap
    property var   model:     null
    property var   udpRecv:   null
    property var   cmdSender: null
    property bool  dark:      true

    // ── Theme ────────────────────────────────────────────────────
    readonly property color bgCol:      dark ? "#0d1625"  : "#ffffff"
    readonly property color borderCol:  dark ? "#1e3050"  : "#d0dce8"
    readonly property color accentCol:  dark ? "#1ab4f0"  : "#0070c0"
    readonly property color textCol:    dark ? "#d8eaf8"  : "#1a2840"
    readonly property color dimCol:     dark ? "#4a6888"  : "#7090b0"
    readonly property color healthyCol: dark ? "#00e87a"  : "#00a854"
    readonly property color warningCol: dark ? "#ffb700"  : "#e07800"
    readonly property color critCol:    dark ? "#ff3a3a"  : "#d41010"
    readonly property color unknownCol: dark ? "#2a3e58"  : "#b0c8e0"
    readonly property color surfaceCol: dark ? "#111c2e"  : "#f7fafd"
    readonly property color rowAlt:     dark ? "rgba(255,255,255,0.03)" : "rgba(0,0,0,0.03)"
    readonly property color cardCol:    dark ? "#0f1e35"  : "#f0f6ff"

    // ── Repaint triggers ─────────────────────────────────────────
    Connections {
        target: ap.model
        function onStatsChanged()  { chartsCanvas.requestPaint() }
        function onHistoryChanged(){ chartsCanvas.requestPaint() }
    }
    Connections {
        target: ap.cmdSender
        function onCommandLogChanged() {
            cmdLogList.model = ap.cmdSender ? ap.cmdSender.commandLog : []
        }
    }

    // ── Layout ───────────────────────────────────────────────────
    Column {
        anchors.fill: parent
        spacing: 0

        // ── Charts canvas ─────────────────────────────────────────
        Canvas {
            id: chartsCanvas
            width: parent.width
            height: parent.height * 0.64

            onPaint: {
                if (!ap.model || width < 20 || height < 20) return
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = ap.bgCol
                ctx.fillRect(0, 0, width, height)

                var pad = 14
                var cw  = width - pad * 2
                var y   = pad

                // ══ Title bar ═════════════════════════════════════
                ctx.fillStyle = ap.accentCol
                ctx.font = "bold 12px 'Segoe UI'"
                ctx.textAlign = "left"
                ctx.fillText("ANALYTICS  ·  " + ap.model.antennaName, pad, y + 12)

                // Last update (right-aligned)
                ctx.fillStyle = ap.dimCol
                ctx.font = "9px 'Segoe UI'"
                ctx.textAlign = "right"
                ctx.fillText(ap.model.lastUpdateTime, width - pad, y + 12)
                ctx.textAlign = "left"
                y += 24

                ctx.strokeStyle = ap.borderCol; ctx.lineWidth = 1
                ctx.beginPath(); ctx.moveTo(pad, y); ctx.lineTo(width - pad, y); ctx.stroke()
                y += 10

                // ══ Top row: Donut + summary stats ════════════════
                var total = ap.model.totalElements
                var h  = ap.model.healthyCount
                var w2 = ap.model.warningCount
                var c  = ap.model.criticalCount
                var u  = ap.model.noDataCount

                var donutR  = Math.max(4, Math.min(cw * 0.28, 52))
                var donutCx = pad + donutR + 4
                var donutCy = y + donutR + 8
                var donutIn = donutR * 0.55

                // Donut segments
                if (total > 0 && donutR > 4) {
                    var segs = [[h/total, ap.healthyCol], [w2/total, ap.warningCol],
                                [c/total, ap.critCol],   [u/total, ap.unknownCol]]
                    var ang = -Math.PI / 2
                    for (var si = 0; si < 4; si++) {
                        var sw = segs[si][0] * Math.PI * 2
                        if (sw < 0.001) continue
                        ctx.beginPath(); ctx.moveTo(donutCx, donutCy)
                        ctx.arc(donutCx, donutCy, donutR, ang, ang + sw)
                        ctx.closePath(); ctx.fillStyle = segs[si][1]; ctx.fill()
                        ang += sw
                    }
                    ctx.beginPath()
                    ctx.arc(donutCx, donutCy, donutIn, 0, Math.PI * 2)
                    ctx.fillStyle = ap.bgCol; ctx.fill()
                }

                var gPct = total > 0 ? Math.round(h * 100 / total) : 0
                ctx.fillStyle = gPct >= 75 ? ap.healthyCol : gPct >= 50 ? ap.warningCol : ap.critCol
                ctx.font = "bold 15px 'Segoe UI'"
                ctx.textAlign = "center"
                ctx.fillText(gPct + "%", donutCx, donutCy + 4)
                ctx.fillStyle = ap.dimCol; ctx.font = "7px 'Segoe UI'"
                ctx.fillText("HEALTH", donutCx, donutCy + 14)

                // Legend beside donut
                var lx2 = donutCx + donutR + 14
                var ly2 = donutCy - donutR + 4
                var lbls2 = [["Healthy", h, ap.healthyCol], ["Warning", w2, ap.warningCol],
                             ["Critical", c, ap.critCol],  ["No Data", u, ap.unknownCol]]
                for (var li = 0; li < 4; li++) {
                    ctx.fillStyle = lbls2[li][2]; ctx.fillRect(lx2, ly2 + li * 16, 8, 8)
                    ctx.fillStyle = ap.textCol; ctx.font = "10px 'Segoe UI'"; ctx.textAlign = "left"
                    ctx.fillText(lbls2[li][0] + "  " + lbls2[li][1], lx2 + 12, ly2 + li * 16 + 8)
                }

                // Packet / element summary (right column)
                var rx = lx2 + 90
                if (rx + 60 < width - pad) {
                    var pkt = ap.udpRecv ? ap.udpRecv.packetsReceived : 0
                    var summaryItems = [
                        ["TOTAL", total, ap.textCol],
                        ["PKTS",  pkt,   ap.accentCol]
                    ]
                    for (var si2 = 0; si2 < 2; si2++) {
                        ctx.fillStyle = ap.dimCol; ctx.font = "8px 'Segoe UI'"
                        ctx.textAlign = "left"
                        ctx.fillText(summaryItems[si2][0], rx, ly2 + si2 * 26 + 8)
                        ctx.fillStyle = summaryItems[si2][2]; ctx.font = "bold 13px 'Segoe UI'"
                        ctx.fillText(summaryItems[si2][1], rx, ly2 + si2 * 26 + 22)
                    }
                }

                y = donutCy + donutR + 14

                ctx.strokeStyle = ap.borderCol; ctx.lineWidth = 1
                ctx.beginPath(); ctx.moveTo(pad, y); ctx.lineTo(width - pad, y); ctx.stroke()
                y += 8

                // ══ Per-quadrant health bars ═══════════════════════
                ctx.fillStyle = ap.accentCol; ctx.font = "bold 9px 'Segoe UI'"
                ctx.textAlign = "left"; ctx.fillText("PER QUADRANT", pad, y + 9); y += 18

                var barMaxW = cw - 60
                var qLbls   = ["Q1", "Q2", "Q3", "Q4"]
                for (var qi = 0; qi < 4 && qi < ap.model.quadrantCount; qi++) {
                    var qv   = ap.model.quadrantHealth(qi)
                    var qpct = qv < 0 ? 0 : qv
                    var qs   = ap.model ? ap.model.quadrantStats(qi) : [0, 0, 0, 0]
                    var qcol = qv < 0 ? ap.unknownCol
                             : qs[2] > 0 ? ap.critCol
                             : qs[1] > 0 ? ap.warningCol
                             : qs[0] > 0 ? ap.healthyCol : ap.unknownCol

                    // Label
                    ctx.fillStyle = ap.dimCol; ctx.font = "9px 'Segoe UI'"
                    ctx.textAlign = "left"; ctx.fillText(qLbls[qi], pad, y + 10)

                    // Bar background
                    ctx.fillStyle = ap.dark ? "rgba(255,255,255,0.06)" : "rgba(0,0,0,0.06)"
                    ctx.fillRect(pad + 28, y, barMaxW, 11)

                    // Bar fill
                    if (qv >= 0) {
                        ctx.fillStyle = qcol
                        ctx.fillRect(pad + 28, y, barMaxW * (qpct / 100), 11)
                    }

                    // % label
                    ctx.fillStyle = ap.textCol; ctx.font = "bold 9px 'Segoe UI'"
                    ctx.textAlign = "right"
                    ctx.fillText(qv < 0 ? "N/A" : qpct + "%", width - pad, y + 10)

                    // Mini counts
                    if (qv >= 0) {
                        var countStr = qs[0] + "✓ " + qs[1] + "! " + qs[2] + "✗"
                        ctx.fillStyle = ap.dimCol; ctx.font = "8px 'Segoe UI'"
                        ctx.textAlign = "left"
                        ctx.fillText(countStr, pad + 28, y + 21)
                    }

                    y += (qv >= 0 ? 28 : 18)
                }

                ctx.strokeStyle = ap.borderCol; ctx.lineWidth = 1
                ctx.beginPath(); ctx.moveTo(pad, y); ctx.lineTo(width - pad, y); ctx.stroke()
                y += 8

                // ══ Sparkline ═════════════════════════════════════
                ctx.fillStyle = ap.accentCol; ctx.font = "bold 9px 'Segoe UI'"
                ctx.textAlign = "left"; ctx.fillText("HEALTH TREND  (2 min)", pad, y + 9); y += 17

                var hist  = ap.model.healthHistory
                var spH   = 40; var spW = cw; var spX = pad
                ctx.fillStyle = ap.dark ? "rgba(255,255,255,0.04)" : "rgba(0,0,0,0.04)"
                ctx.fillRect(spX, y, spW, spH)

                // Reference lines (25 / 50 / 75 %)
                ctx.strokeStyle = ap.dark ? "rgba(255,255,255,0.07)" : "rgba(0,0,0,0.07)"
                ctx.lineWidth = 0.5
                for (var ri = 1; ri <= 3; ri++) {
                    var ry3 = y + spH - (ri / 4) * spH
                    ctx.beginPath(); ctx.moveTo(spX, ry3); ctx.lineTo(spX + spW, ry3); ctx.stroke()
                }

                if (hist && hist.length > 1) {
                    // Area fill
                    ctx.beginPath()
                    var first2 = true
                    for (var hi = 0; hi < hist.length; hi++) {
                        var hv = hist[hi]; if (hv < 0) { first2 = true; continue }
                        var hx = spX + (hi / (hist.length - 1)) * spW
                        var hy2 = y + spH - (hv / 100) * spH
                        if (first2) { ctx.moveTo(hx, hy2); first2 = false } else ctx.lineTo(hx, hy2)
                    }
                    var gr2 = ctx.createLinearGradient(0, y, 0, y + spH)
                    gr2.addColorStop(0, ap.dark ? "rgba(0,232,122,0.28)" : "rgba(0,168,84,0.18)")
                    gr2.addColorStop(1, "rgba(0,0,0,0)")
                    ctx.lineTo(spX + spW, y + spH); ctx.lineTo(spX, y + spH); ctx.closePath()
                    ctx.fillStyle = gr2; ctx.fill()
                    // Line
                    ctx.beginPath(); first2 = true
                    for (var hi2 = 0; hi2 < hist.length; hi2++) {
                        var hv2 = hist[hi2]; if (hv2 < 0) { first2 = true; continue }
                        var hx2 = spX + (hi2 / (hist.length - 1)) * spW
                        var hy3 = y + spH - (hv2 / 100) * spH
                        if (first2) { ctx.moveTo(hx2, hy3); first2 = false } else ctx.lineTo(hx2, hy3)
                    }
                    ctx.strokeStyle = ap.healthyCol; ctx.lineWidth = 1.5; ctx.stroke()

                    // Current value dot
                    if (hist.length > 0) {
                        var lastH = hist[hist.length - 1]
                        var lastX = spX + spW
                        var lastY = y + spH - (lastH / 100) * spH
                        ctx.beginPath()
                        ctx.arc(lastX, lastY, 3.5, 0, Math.PI * 2)
                        ctx.fillStyle = ap.healthyCol; ctx.fill()
                    }
                }

                // Y axis labels
                ctx.fillStyle = ap.dimCol; ctx.font = "8px 'Segoe UI'"
                ctx.textAlign = "right"
                ctx.fillText("100%", spX - 2, y + 8)
                ctx.fillText("50%",  spX - 2, y + spH / 2 + 4)
                ctx.fillText("0%",   spX - 2, y + spH)
                y += spH + 8

                // ══ Worst performers ═══════════════════════════════
                var worst = ap.model ? ap.model.worstElements(4) : []
                if (worst && worst.length > 0) {
                    ctx.strokeStyle = ap.borderCol; ctx.lineWidth = 1
                    ctx.beginPath(); ctx.moveTo(pad, y); ctx.lineTo(width - pad, y); ctx.stroke()
                    y += 8

                    ctx.fillStyle = ap.critCol; ctx.font = "bold 9px 'Segoe UI'"
                    ctx.textAlign = "left"; ctx.fillText("WORST ELEMENTS", pad, y + 9); y += 18

                    var maxEntries = Math.min(4, worst.length)
                    for (var wi = 0; wi < maxEntries; wi++) {
                        var elem = worst[wi]
                        if (!elem) continue
                        var elemStr = "Q" + ((elem["quad"] || 0) + 1) + " E" + ((elem["elem"] || 0) + 1)
                        var elemStatus = elem["status"] || 0

                        var wCol = elemStatus === 3 ? ap.critCol
                               : elemStatus === 2 ? ap.warningCol : ap.unknownCol
                        ctx.fillStyle = wCol
                        ctx.fillRect(pad, y, 6, 10)

                        ctx.fillStyle = ap.textCol; ctx.font = "9px 'Segoe UI'"
                        ctx.textAlign = "left"
                        ctx.fillText(elemStr, pad + 10, y + 9)

                        // Power / Temp / Current
                        var pwr  = elem["power"]       !== undefined ? elem["power"].toFixed(1) + " dBm" : "—"
                        var tmp  = elem["temperature"] !== undefined ? elem["temperature"].toFixed(1) + " °C" : "—"
                        var cur  = elem["current"]     !== undefined ? elem["current"].toFixed(2) + " A" : "—"
                        ctx.fillStyle = ap.dimCol; ctx.font = "8px 'Segoe UI'"
                        ctx.fillText(pwr + "  " + tmp + "  " + cur, pad + 56, y + 9)

                        y += 15
                    }
                }
            }
        }

        // ── Divider ───────────────────────────────────────────────
        Rectangle {
            width: parent.width; height: 1; color: ap.borderCol
        }

        // ── Command log header ────────────────────────────────────
        Rectangle {
            id: cmdLogHeader
            width: parent.width; height: 30
            color: ap.dark ? "#091320" : "#e8f0f8"

            RowLayout {
                anchors { fill: parent; leftMargin: 12; rightMargin: 10 }

                Text {
                    text: "COMMAND LOG"
                    font { family: "Segoe UI"; pixelSize: 10; bold: true; letterSpacing: 1 }
                    color: ap.accentCol
                }

                // Command stats
                Text {
                    visible: ap.cmdSender && (ap.cmdSender.commandLog.length > 0)
                    text: {
                        if (!ap.cmdSender) return ""
                        var log = ap.cmdSender.commandLog
                        var ok = 0, err = 0, pending = 0
                        for (var i = 0; i < log.length; i++) {
                            var s = log[i]["status"]
                            if (s === 1) ok++
                            else if (s === 3) err++
                            else pending++
                        }
                        var total = log.length
                        var okPct = total > 0 ? Math.round(ok * 100 / total) : 0
                        return "ACK " + okPct + "%  (" + ok + "✓ " + err + "✗)"
                    }
                    font { family: "Segoe UI"; pixelSize: 9 }
                    color: ap.dimCol
                    Layout.leftMargin: 10
                }

                Item { Layout.fillWidth: true }

                // Pending badge
                Rectangle {
                    visible: ap.cmdSender && ap.cmdSender.pendingCount > 0
                    width: pendTxt.implicitWidth + 12; height: 16; radius: 8
                    color: ap.warningCol
                    Text {
                        id: pendTxt
                        anchors.centerIn: parent
                        text: (ap.cmdSender ? ap.cmdSender.pendingCount : 0) + " pending"
                        font { family: "Segoe UI"; pixelSize: 9; bold: true }
                        color: "#000"
                    }
                }

                // Clear button
                Rectangle {
                    width: clrTxt.implicitWidth + 10; height: 16; radius: 4
                    color: "transparent"
                    border { color: ap.borderCol; width: 1 }
                    Text {
                        id: clrTxt; anchors.centerIn: parent
                        text: "Clear"
                        font { family: "Segoe UI"; pixelSize: 9 }
                        color: ap.dimCol
                    }
                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: { if (ap.cmdSender) ap.cmdSender.clearLog() }
                    }
                }
            }
        }

        // ── Command log list ──────────────────────────────────────
        Rectangle {
            width: parent.width
            height: ap.height - chartsCanvas.height - 1 - cmdLogHeader.height
            color: ap.bgCol
            clip: true

            Text {
                anchors.centerIn: parent
                visible: cmdLogList.count === 0
                text: "No commands sent yet.\nClick an element to open\nthe control panel."
                font { family: "Segoe UI"; pixelSize: 11 }
                color: ap.dimCol
                horizontalAlignment: Text.AlignHCenter
            }

            ListView {
                id: cmdLogList
                anchors.fill: parent
                model: ap.cmdSender ? ap.cmdSender.commandLog : []
                clip: true

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                delegate: Rectangle {
                    width: cmdLogList.width
                    height: 50
                    color: index % 2 === 0 ? ap.rowAlt : "transparent"

                    // Left status stripe
                    Rectangle {
                        width: 3; height: parent.height
                        color: {
                            var s = modelData["status"]
                            if (s === 1) return ap.healthyCol
                            if (s === 2) return ap.warningCol
                            if (s === 3) return ap.critCol
                            return ap.accentCol
                        }
                    }

                    Column {
                        anchors {
                            left: parent.left; right: parent.right
                            leftMargin: 10; topMargin: 6
                        }
                        spacing: 3

                        // Row 1: command name + element ID + quadrant
                        Row {
                            spacing: 6
                            Text {
                                text: modelData["cmdName"] || ""
                                font { family: "Segoe UI"; pixelSize: 11; bold: true }
                                color: {
                                    var cmd = modelData["cmd"]
                                    if (cmd === 1) return ap.critCol
                                    if (cmd === 2) return ap.accentCol
                                    return ap.healthyCol
                                }
                            }
                            Text {
                                text: "E" + (modelData["elem"] || 0)
                                font { family: "Segoe UI"; pixelSize: 11 }
                                color: ap.textCol
                            }
                            Text {
                                text: "Q" + ((modelData["quad"] || 0) + 1)
                                font { family: "Segoe UI"; pixelSize: 10 }
                                color: ap.dimCol
                            }
                        }

                        // Row 2: status pill + timestamp
                        Row {
                            spacing: 6
                            Rectangle {
                                width: statusPill.implicitWidth + 10
                                height: 14; radius: 7
                                color: {
                                    var s = modelData["status"]
                                    if (s === 1) return Qt.rgba(0, 0.91, 0.48, 0.18)
                                    if (s === 2) return Qt.rgba(1, 0.72, 0, 0.18)
                                    if (s === 3) return Qt.rgba(1, 0.23, 0.23, 0.18)
                                    return Qt.rgba(0.1, 0.71, 0.94, 0.18)
                                }
                                border.color: {
                                    var s = modelData["status"]
                                    if (s === 1) return ap.healthyCol
                                    if (s === 2) return ap.warningCol
                                    if (s === 3) return ap.critCol
                                    return ap.accentCol
                                }
                                border.width: 1
                                Text {
                                    id: statusPill
                                    anchors.centerIn: parent
                                    text: modelData["statName"] || ""
                                    font { family: "Segoe UI"; pixelSize: 8; bold: true }
                                    color: {
                                        var s = modelData["status"]
                                        if (s === 1) return ap.healthyCol
                                        if (s === 2) return ap.warningCol
                                        if (s === 3) return ap.critCol
                                        return ap.accentCol
                                    }
                                }
                            }
                            Text {
                                text: modelData["time"] || ""
                                font { family: "Courier New"; pixelSize: 9 }
                                color: ap.dimCol
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }
        }
    }
}
