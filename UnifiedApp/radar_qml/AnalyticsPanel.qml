import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Item {
    id: ap
    property var   model:     null
    property var   udpRecv:   null
    property var   cmdSender: null
    property bool  dark:      true

    // ── Design tokens ─────────────────────────────────────────────
    readonly property color bgBase:      dark ? "#0c1017" : "#f1f5f9"
    readonly property color bgCard:      dark ? "#131a26" : "#ffffff"
    readonly property color bgCardAlt:   dark ? "#0f1620" : "#f8fafc"
    readonly property color borderCol:   dark ? "rgba(255,255,255,0.07)" : "rgba(0,0,0,0.07)"
    readonly property color accentBlue:  dark ? "#3B82F6" : "#2563EB"
    readonly property color accentCyan:  dark ? "#06B6D4" : "#0891B2"
    readonly property color textPrimary: dark ? "#e2e8f0" : "#1e293b"
    readonly property color textMuted:   dark ? "#64748b" : "#94a3b8"
    readonly property color healthyCol:  dark ? "#10B981" : "#059669"
    readonly property color warningCol:  dark ? "#F59E0B" : "#D97706"
    readonly property color critCol:     dark ? "#EF4444" : "#DC2626"
    readonly property color unknownCol:  dark ? "#334155" : "#cbd5e1"
    readonly property color rowOdd:      dark ? "rgba(255,255,255,0.025)" : "rgba(0,0,0,0.025)"

    // ── Repaint triggers ──────────────────────────────────────────
    Connections {
        target: ap.model
        function onStatsChanged()   { chartsCanvas.requestPaint() }
        function onHistoryChanged() { chartsCanvas.requestPaint() }
    }
    Connections {
        target: ap.cmdSender
        function onCommandLogChanged() {
            cmdLogList.model = ap.cmdSender ? ap.cmdSender.commandLog : []
        }
    }

    // Root column
    Column {
        anchors.fill: parent
        spacing: 0

        // ── Charts section (64% height) ───────────────────────────
        Canvas {
            id: chartsCanvas
            width:  parent.width
            height: parent.height * 0.64

            onPaint: {
                if (!ap.model || width < 20 || height < 20) return
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                // Background
                ctx.fillStyle = ap.bgBase
                ctx.fillRect(0, 0, width, height)

                var pad = 12

                // ──────────────────────────────────────────────────
                // SECTION 1: Header strip
                // ──────────────────────────────────────────────────
                var headerH = 36
                // header card bg
                ctx.fillStyle = ap.bgCard
                roundRect(ctx, 0, 0, width, headerH, 0)
                ctx.fill()

                // Blue left accent bar
                ctx.fillStyle = ap.accentBlue
                ctx.fillRect(0, 0, 3, headerH)

                // Title
                ctx.fillStyle = ap.textPrimary
                ctx.font = "bold 11px 'Segoe UI'"
                ctx.textAlign = "left"
                ctx.textBaseline = "middle"
                ctx.fillText(ap.model.antennaName || "Analytics", 14, headerH / 2)

                // Right: last update
                ctx.fillStyle = ap.textMuted
                ctx.font = "9px 'Segoe UI'"
                ctx.textAlign = "right"
                ctx.fillText(ap.model.lastUpdateTime, width - pad, headerH / 2)
                ctx.textAlign = "left"
                ctx.textBaseline = "alphabetic"

                var y = headerH + 8

                // ──────────────────────────────────────────────────
                // SECTION 2: Metric tiles row
                // ──────────────────────────────────────────────────
                var total = ap.model.totalElements
                var h  = ap.model.healthyCount
                var w2 = ap.model.warningCount
                var c  = ap.model.criticalCount
                var u  = ap.model.noDataCount
                var gPct = total > 0 ? Math.round(h * 100 / total) : 0

                var pkt = ap.udpRecv ? ap.udpRecv.packetsReceived : 0

                var tiles = [
                    { label: "HEALTH",   value: gPct + "%",     color: gPct >= 75 ? ap.healthyCol : gPct >= 50 ? ap.warningCol : ap.critCol },
                    { label: "ELEMENTS", value: "" + total,     color: ap.accentBlue },
                    { label: "PACKETS",  value: "" + pkt,       color: ap.accentCyan },
                    { label: "CRITICAL", value: "" + c,         color: c > 0 ? ap.critCol : ap.textMuted }
                ]

                var tileW = (width - pad * 2 - 6 * 3) / 4
                var tileH = 46
                for (var ti = 0; ti < 4; ti++) {
                    var tx = pad + ti * (tileW + 6)
                    // card bg
                    ctx.fillStyle = ap.bgCard
                    roundRect(ctx, tx, y, tileW, tileH, 6)
                    ctx.fill()
                    // top accent line
                    ctx.fillStyle = tiles[ti].color
                    roundRect(ctx, tx, y, tileW, 3, [3, 3, 0, 0])
                    ctx.fill()
                    // value
                    ctx.fillStyle = tiles[ti].color
                    ctx.font = "bold 15px 'Segoe UI'"
                    ctx.textAlign = "center"
                    ctx.fillText(tiles[ti].value, tx + tileW / 2, y + 26)
                    // label
                    ctx.fillStyle = ap.textMuted
                    ctx.font = "8px 'Segoe UI'"
                    ctx.fillText(tiles[ti].label, tx + tileW / 2, y + 39)
                }
                ctx.textAlign = "left"

                y += tileH + 10

                // ──────────────────────────────────────────────────
                // SECTION 3: Donut + legend card
                // ──────────────────────────────────────────────────
                var donutCardH = 82
                ctx.fillStyle = ap.bgCard
                roundRect(ctx, pad, y, width - pad * 2, donutCardH, 8)
                ctx.fill()

                // Section label
                ctx.fillStyle = ap.textMuted
                ctx.font = "bold 8px 'Segoe UI'"
                ctx.textAlign = "left"
                ctx.fillText("STATUS DISTRIBUTION", pad + 10, y + 14)

                var donutR  = Math.min(28, (donutCardH - 20) / 2)
                var donutCx = pad + 10 + donutR + 4
                var donutCy = y + donutCardH / 2 + 4
                var donutIn = donutR * 0.56

                if (total > 0) {
                    var segs = [
                        [h / total,  ap.healthyCol],
                        [w2 / total, ap.warningCol],
                        [c / total,  ap.critCol],
                        [u / total,  ap.unknownCol]
                    ]
                    var ang = -Math.PI / 2
                    for (var si = 0; si < 4; si++) {
                        var sw = segs[si][0] * Math.PI * 2
                        if (sw < 0.002) continue
                        ctx.beginPath()
                        ctx.moveTo(donutCx, donutCy)
                        ctx.arc(donutCx, donutCy, donutR, ang, ang + sw)
                        ctx.closePath()
                        ctx.fillStyle = segs[si][1]
                        ctx.fill()
                        ang += sw
                    }
                    // hole
                    ctx.beginPath()
                    ctx.arc(donutCx, donutCy, donutIn, 0, Math.PI * 2)
                    ctx.fillStyle = ap.bgCard
                    ctx.fill()
                } else {
                    ctx.beginPath()
                    ctx.arc(donutCx, donutCy, donutR, 0, Math.PI * 2)
                    ctx.fillStyle = ap.unknownCol
                    ctx.fill()
                    ctx.beginPath()
                    ctx.arc(donutCx, donutCy, donutIn, 0, Math.PI * 2)
                    ctx.fillStyle = ap.bgCard
                    ctx.fill()
                }

                // center pct text
                ctx.fillStyle = gPct >= 75 ? ap.healthyCol : gPct >= 50 ? ap.warningCol : ap.critCol
                ctx.font = "bold 11px 'Segoe UI'"
                ctx.textAlign = "center"
                ctx.fillText(gPct + "%", donutCx, donutCy + 4)
                ctx.textAlign = "left"

                // Legend
                var lx = donutCx + donutR + 14
                var ly = donutCy - 24
                var lbls = [
                    ["Healthy",  h,  ap.healthyCol],
                    ["Warning",  w2, ap.warningCol],
                    ["Critical", c,  ap.critCol],
                    ["No Data",  u,  ap.unknownCol]
                ]
                for (var li = 0; li < 4; li++) {
                    // dot
                    ctx.fillStyle = lbls[li][2]
                    ctx.beginPath()
                    ctx.arc(lx + 3, ly + li * 15 + 4, 3, 0, Math.PI * 2)
                    ctx.fill()
                    // text
                    ctx.fillStyle = ap.textMuted
                    ctx.font = "9px 'Segoe UI'"
                    ctx.fillText(lbls[li][0], lx + 10, ly + li * 15 + 8)
                    // count
                    ctx.fillStyle = ap.textPrimary
                    ctx.font = "bold 9px 'Segoe UI'"
                    ctx.textAlign = "right"
                    ctx.fillText(lbls[li][1], width - pad - 10, ly + li * 15 + 8)
                    ctx.textAlign = "left"
                }

                y += donutCardH + 8

                // ──────────────────────────────────────────────────
                // SECTION 4: Per-quadrant health bars card
                // ──────────────────────────────────────────────────
                var qCardH = 14 + ap.model.quadrantCount * 22 + 10
                ctx.fillStyle = ap.bgCard
                roundRect(ctx, pad, y, width - pad * 2, qCardH, 8)
                ctx.fill()

                ctx.fillStyle = ap.textMuted
                ctx.font = "bold 8px 'Segoe UI'"
                ctx.fillText("PER QUADRANT", pad + 10, y + 14)

                var barX   = pad + 36
                var barMaxW = width - pad * 2 - 36 - 38
                var qy     = y + 22
                var qLbls  = ["Q1", "Q2", "Q3", "Q4"]

                for (var qi = 0; qi < 4 && qi < ap.model.quadrantCount; qi++) {
                    var qv  = ap.model.quadrantHealth(qi)
                    var qs  = ap.model.quadrantStats(qi)
                    var qpct = qv < 0 ? 0 : qv
                    var qcol = qv < 0 ? ap.unknownCol
                             : qs[2] > 0 ? ap.critCol
                             : qs[1] > 0 ? ap.warningCol
                             : qs[0] > 0 ? ap.healthyCol : ap.unknownCol

                    // Label
                    ctx.fillStyle = ap.textMuted
                    ctx.font = "bold 9px 'Segoe UI'"
                    ctx.textAlign = "left"
                    ctx.fillText(qLbls[qi], pad + 10, qy + 10)

                    // Track
                    ctx.fillStyle = dark ? "rgba(255,255,255,0.06)" : "rgba(0,0,0,0.06)"
                    roundRect(ctx, barX, qy + 2, barMaxW, 8, 4)
                    ctx.fill()

                    // Fill
                    if (qv >= 0 && barMaxW * (qpct / 100) > 0) {
                        ctx.fillStyle = qcol
                        roundRect(ctx, barX, qy + 2, barMaxW * (qpct / 100), 8, 4)
                        ctx.fill()
                    }

                    // Pct label
                    ctx.fillStyle = ap.textPrimary
                    ctx.font = "bold 9px 'Segoe UI'"
                    ctx.textAlign = "right"
                    ctx.fillText(qv < 0 ? "N/A" : qpct + "%", width - pad - 10, qy + 10)
                    ctx.textAlign = "left"

                    qy += 22
                }

                y += qCardH + 8

                // ──────────────────────────────────────────────────
                // SECTION 5: Health trend sparkline card
                // ──────────────────────────────────────────────────
                var spCardH = 72
                ctx.fillStyle = ap.bgCard
                roundRect(ctx, pad, y, width - pad * 2, spCardH, 8)
                ctx.fill()

                // accent left bar
                ctx.fillStyle = ap.healthyCol
                ctx.fillRect(pad, y, 3, spCardH)

                ctx.fillStyle = ap.textMuted
                ctx.font = "bold 8px 'Segoe UI'"
                ctx.textAlign = "left"
                ctx.fillText("HEALTH TREND  ·  2 min", pad + 10, y + 13)

                var spX = pad + 10
                var spW = width - pad * 2 - 20
                var spY = y + 20
                var spH = spCardH - 28

                // grid lines at 25/50/75%
                ctx.strokeStyle = dark ? "rgba(255,255,255,0.06)" : "rgba(0,0,0,0.06)"
                ctx.lineWidth = 0.5
                for (var ri = 1; ri <= 3; ri++) {
                    var ry3 = spY + spH - (ri / 4) * spH
                    ctx.beginPath(); ctx.moveTo(spX, ry3); ctx.lineTo(spX + spW, ry3); ctx.stroke()
                }

                var hist = ap.model.healthHistory
                if (hist && hist.length > 1) {
                    // area
                    ctx.beginPath()
                    var first2 = true
                    for (var hi = 0; hi < hist.length; hi++) {
                        var hv = hist[hi]; if (hv < 0) { first2 = true; continue }
                        var hx = spX + (hi / (hist.length - 1)) * spW
                        var hy2 = spY + spH - (hv / 100) * spH
                        if (first2) { ctx.moveTo(hx, hy2); first2 = false } else ctx.lineTo(hx, hy2)
                    }
                    var gr2 = ctx.createLinearGradient(0, spY, 0, spY + spH)
                    gr2.addColorStop(0, dark ? "rgba(16,185,129,0.22)" : "rgba(16,185,129,0.12)")
                    gr2.addColorStop(1, "rgba(0,0,0,0)")
                    ctx.lineTo(spX + spW, spY + spH); ctx.lineTo(spX, spY + spH); ctx.closePath()
                    ctx.fillStyle = gr2; ctx.fill()

                    // line
                    ctx.beginPath(); first2 = true
                    for (var hi2 = 0; hi2 < hist.length; hi2++) {
                        var hv2 = hist[hi2]; if (hv2 < 0) { first2 = true; continue }
                        var hx2 = spX + (hi2 / (hist.length - 1)) * spW
                        var hy3 = spY + spH - (hv2 / 100) * spH
                        if (first2) { ctx.moveTo(hx2, hy3); first2 = false } else ctx.lineTo(hx2, hy3)
                    }
                    ctx.strokeStyle = ap.healthyCol; ctx.lineWidth = 1.5; ctx.stroke()

                    // endpoint dot
                    var lastH = hist[hist.length - 1]
                    var lastX = spX + spW
                    var lastY = spY + spH - (lastH / 100) * spH
                    ctx.beginPath()
                    ctx.arc(lastX, lastY, 3, 0, Math.PI * 2)
                    ctx.fillStyle = ap.healthyCol; ctx.fill()
                }

                // Y axis labels
                ctx.fillStyle = ap.textMuted; ctx.font = "8px 'Segoe UI'"
                ctx.textAlign = "right"
                ctx.fillText("100", spX - 3, spY + 7)
                ctx.fillText("50",  spX - 3, spY + spH / 2 + 4)
                ctx.fillText("0",   spX - 3, spY + spH + 1)
                ctx.textAlign = "left"

                y += spCardH + 8

                // ──────────────────────────────────────────────────
                // SECTION 6: Worst performers
                // ──────────────────────────────────────────────────
                var worst = ap.model ? ap.model.worstElements(4) : []
                if (worst && worst.length > 0) {
                    var worstCardH = 16 + Math.min(4, worst.length) * 22 + 8
                    ctx.fillStyle = ap.bgCard
                    roundRect(ctx, pad, y, width - pad * 2, worstCardH, 8)
                    ctx.fill()

                    // accent left bar (red)
                    ctx.fillStyle = ap.critCol
                    ctx.fillRect(pad, y, 3, worstCardH)

                    ctx.fillStyle = ap.textMuted
                    ctx.font = "bold 8px 'Segoe UI'"
                    ctx.fillText("WORST ELEMENTS", pad + 10, y + 14)

                    var wy = y + 22
                    var maxE = Math.min(4, worst.length)
                    for (var wi = 0; wi < maxE; wi++) {
                        var elem = worst[wi]
                        if (!elem) continue
                        var elemStr = "Q" + ((elem["quad"] || 0) + 1) + "·E" + ((elem["elem"] || 0) + 1)
                        var elemStatus = elem["status"] || 0
                        var wCol = elemStatus === 3 ? ap.critCol
                                 : elemStatus === 2 ? ap.warningCol : ap.unknownCol

                        // Status dot
                        ctx.fillStyle = wCol
                        ctx.beginPath()
                        ctx.arc(pad + 14, wy + 7, 4, 0, Math.PI * 2)
                        ctx.fill()

                        ctx.fillStyle = ap.textPrimary; ctx.font = "bold 9px 'Segoe UI'"
                        ctx.fillText(elemStr, pad + 24, wy + 10)

                        var pwr = elem["power"]       !== undefined ? elem["power"].toFixed(1) + "dBm" : "—"
                        var tmp = elem["temperature"]  !== undefined ? elem["temperature"].toFixed(0) + "°C" : "—"
                        ctx.fillStyle = ap.textMuted; ctx.font = "8px 'Segoe UI'"
                        ctx.textAlign = "right"
                        ctx.fillText(pwr + "  " + tmp, width - pad - 10, wy + 10)
                        ctx.textAlign = "left"

                        wy += 22
                    }
                }
            }

            // ── Utility: rounded rect helper (4-corner or per-corner) ──
            function roundRect(ctx, x, y, w, h, r) {
                var tl = 0, tr = 0, br = 0, bl = 0
                if (typeof r === "number") { tl = tr = br = bl = r }
                else if (Array.isArray(r) && r.length === 4) { tl = r[0]; tr = r[1]; br = r[2]; bl = r[3] }
                ctx.beginPath()
                ctx.moveTo(x + tl, y)
                ctx.lineTo(x + w - tr, y)
                ctx.quadraticCurveTo(x + w, y, x + w, y + tr)
                ctx.lineTo(x + w, y + h - br)
                ctx.quadraticCurveTo(x + w, y + h, x + w - br, y + h)
                ctx.lineTo(x + bl, y + h)
                ctx.quadraticCurveTo(x, y + h, x, y + h - bl)
                ctx.lineTo(x, y + tl)
                ctx.quadraticCurveTo(x, y, x + tl, y)
                ctx.closePath()
            }
        }

        // ── Thin separator ────────────────────────────────────────
        Rectangle {
            width: parent.width
            height: 1
            color: ap.borderCol
        }

        // ── Command log header ────────────────────────────────────
        Rectangle {
            id: cmdLogHeader
            width: parent.width
            height: 32
            color: ap.bgCard

            // left accent
            Rectangle {
                width: 3; height: parent.height
                color: ap.accentCyan
            }

            RowLayout {
                anchors { fill: parent; leftMargin: 14; rightMargin: 10 }

                Text {
                    text: "COMMAND LOG"
                    font { family: "Segoe UI"; pixelSize: 10; bold: true; letterSpacing: 1 }
                    color: ap.accentCyan
                }

                Text {
                    visible: ap.cmdSender && ap.cmdSender.commandLog.length > 0
                    text: {
                        if (!ap.cmdSender) return ""
                        var log = ap.cmdSender.commandLog
                        var ok = 0, err = 0
                        for (var i = 0; i < log.length; i++) {
                            var s = log[i]["status"]
                            if (s === 1) ok++; else if (s === 3) err++
                        }
                        var total2 = log.length
                        var okPct = total2 > 0 ? Math.round(ok * 100 / total2) : 0
                        return "ACK " + okPct + "% · " + ok + " ok · " + err + " err"
                    }
                    font { family: "Segoe UI"; pixelSize: 9 }
                    color: ap.textMuted
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
                        font { family: "Segoe UI"; pixelSize: 8; bold: true }
                        color: "#000"
                    }
                }

                // Clear button
                Rectangle {
                    width: clrTxt.implicitWidth + 12; height: 18; radius: 4
                    color: "transparent"
                    border { color: ap.borderCol; width: 1 }
                    Text {
                        id: clrTxt; anchors.centerIn: parent
                        text: "Clear"
                        font { family: "Segoe UI"; pixelSize: 9 }
                        color: ap.textMuted
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
            color: ap.bgBase
            clip: true

            // Empty state
            Column {
                anchors.centerIn: parent
                visible: cmdLogList.count === 0
                spacing: 6
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "—"
                    font { family: "Segoe UI"; pixelSize: 18 }
                    color: ap.textMuted
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "No commands sent yet"
                    font { family: "Segoe UI"; pixelSize: 11 }
                    color: ap.textMuted
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Click an element to open the control panel"
                    font { family: "Segoe UI"; pixelSize: 10 }
                    color: ap.textMuted
                    opacity: 0.6
                }
            }

            ListView {
                id: cmdLogList
                anchors.fill: parent
                model: ap.cmdSender ? ap.cmdSender.commandLog : []
                clip: true
                spacing: 0

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: Rectangle {
                    width: cmdLogList.width
                    height: 48
                    color: index % 2 === 0 ? ap.rowOdd : "transparent"

                    // Left status stripe
                    Rectangle {
                        width: 3; height: parent.height
                        color: {
                            var s = modelData["status"]
                            if (s === 1) return ap.healthyCol
                            if (s === 2) return ap.warningCol
                            if (s === 3) return ap.critCol
                            return ap.accentBlue
                        }
                    }

                    Column {
                        anchors {
                            left: parent.left; right: parent.right
                            leftMargin: 12; topMargin: 7
                        }
                        spacing: 4

                        Row {
                            spacing: 8
                            Text {
                                text: modelData["cmdName"] || ""
                                font { family: "Segoe UI"; pixelSize: 11; bold: true }
                                color: {
                                    var cmd = modelData["cmd"]
                                    if (cmd === 1) return ap.critCol
                                    if (cmd === 2) return ap.accentBlue
                                    return ap.healthyCol
                                }
                            }
                            Text {
                                text: "E" + (modelData["elem"] || 0)
                                font { family: "Segoe UI"; pixelSize: 11 }
                                color: ap.textPrimary
                            }
                            Text {
                                text: "Q" + ((modelData["quad"] || 0) + 1)
                                font { family: "Segoe UI"; pixelSize: 10 }
                                color: ap.textMuted
                            }
                        }

                        Row {
                            spacing: 6
                            // Status pill
                            Rectangle {
                                width: sPill.implicitWidth + 10; height: 14; radius: 7
                                color: {
                                    var s = modelData["status"]
                                    if (s === 1) return Qt.rgba(0.063, 0.725, 0.506, 0.15)
                                    if (s === 2) return Qt.rgba(0.961, 0.620, 0.043, 0.15)
                                    if (s === 3) return Qt.rgba(0.937, 0.267, 0.267, 0.15)
                                    return Qt.rgba(0.231, 0.510, 0.965, 0.15)
                                }
                                Text {
                                    id: sPill
                                    anchors.centerIn: parent
                                    text: modelData["statName"] || ""
                                    font { family: "Segoe UI"; pixelSize: 8; bold: true }
                                    color: {
                                        var s = modelData["status"]
                                        if (s === 1) return ap.healthyCol
                                        if (s === 2) return ap.warningCol
                                        if (s === 3) return ap.critCol
                                        return ap.accentBlue
                                    }
                                }
                            }
                            Text {
                                text: modelData["time"] || ""
                                font { family: "Courier New"; pixelSize: 9 }
                                color: ap.textMuted
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }
        }
    }
}
