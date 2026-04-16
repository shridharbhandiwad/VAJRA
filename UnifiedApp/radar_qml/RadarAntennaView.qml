import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Root item — embedded in QQuickWidget inside the Qt Widgets app.
// All LCU / INU code has been removed; this shows Radar Antenna only.
Item {
    id: root
    width: 1400; height: 900

    // ── Theme ────────────────────────────────────────────────────
    property bool darkTheme: true
    QtObject {
        id: th
        property color bg:        root.darkTheme ? "#080d14"  : "#f0f4f8"
        property color surface:   root.darkTheme ? "#0d1625"  : "#ffffff"
        property color surface2:  root.darkTheme ? "#111c2e"  : "#f7fafd"
        property color surfaceHov:root.darkTheme ? "#16233a"  : "#eaf1fa"
        property color border:    root.darkTheme ? "#1e3050"  : "#d0dce8"
        property color accent:    root.darkTheme ? "#1ab4f0"  : "#0070c0"
        property color text:      root.darkTheme ? "#d8eaf8"  : "#1a2840"
        property color textDim:   root.darkTheme ? "#4a6888"  : "#7090b0"
        property color header:    root.darkTheme ? "#0b1828"  : "#1a3a6a"
        property color bar:       root.darkTheme ? "#091320"  : "#e4edf8"
        property color healthy:   root.darkTheme ? "#00e87a"  : "#00a854"
        property color warning:   root.darkTheme ? "#ffb700"  : "#e07800"
        property color critical:  root.darkTheme ? "#ff3a3a"  : "#d41010"
        property color unknown:   root.darkTheme ? "#2a3e58"  : "#b0c8e0"
        property color ttBg:      root.darkTheme ? "#0d1e34"  : "#1a3a6a"
        property color ttText:    root.darkTheme ? "#d8eaf8"  : "#ffffff"
        property color ttBorder:  root.darkTheme ? "#1ab4f0"  : "#5090d0"
    }

    Rectangle { anchors.fill: parent; color: th.bg }

    // ── Antenna selection ────────────────────────────────────────
    property int  currentAntenna: 0
    property var  antennas:    [antenna0, antenna1]
    property var  udpReceivers:[udpReceiver0, udpReceiver1]
    property var  simulators:  [simulator0, simulator1]
    property var  cmdSenders:  [cmdSender0, cmdSender1]
    property var  model:       antennas[currentAntenna]
    property var  udpRecv:     udpReceivers[currentAntenna]
    property var  sim:         simulators[currentAntenna]
    property var  cmdSender:   cmdSenders[currentAntenna]

    // ── Control panel state ──────────────────────────────────────
    property bool cpVisible:    false
    property int  cpGlobalId:  -1
    property int  cpQuadrant:   0
    property int  cpLocalElem:  0
    property int  cpCluster:    0
    property int  cpLocalIdx:   0

    function openControlPanel(gid, quad, localE) {
        var EPQ  = root.model ? root.model.elementsPerQuadrant : 256
        var EPC  = root.model ? root.model.elementsPerCluster  : 16
        cpGlobalId  = gid
        cpQuadrant  = quad
        cpLocalElem = localE
        cpCluster   = Math.floor(localE / EPC)
        cpLocalIdx  = localE % EPC
        cpVisible   = true
    }
    function closeControlPanel() { cpVisible = false }

    // ── LOD / navigation state ───────────────────────────────────
    property int  lodLevel:    1
    property int  selQuad:    -1
    property int  selCluster: -1
    property bool fullView:    false
    property bool analyticsOpen: false

    // ── Expand/Shrink state (LOD1 quadrant sizing) ───────────────
    property int  expandedQuad: -1
    readonly property real bigFrac:   0.68
    readonly property real smallFrac: 0.32

    function qX(q, W) {
        var col = q % 2
        if (expandedQuad < 0 || fullView) return col * W / 2
        var ec = expandedQuad % 2
        return col === 0 ? 0 : (ec === 0 ? bigFrac : smallFrac) * W
    }
    function qY(q, H) {
        var row = Math.floor(q / 2)
        if (expandedQuad < 0 || fullView) return row * H / 2
        var er = Math.floor(expandedQuad / 2)
        return row === 0 ? 0 : (er === 0 ? bigFrac : smallFrac) * H
    }
    function qW(q, W) {
        if (expandedQuad < 0 || fullView) return W / 2
        return (q % 2 === expandedQuad % 2) ? bigFrac * W : smallFrac * W
    }
    function qH(q, H) {
        if (expandedQuad < 0 || fullView) return H / 2
        return (Math.floor(q/2) === Math.floor(expandedQuad/2)) ? bigFrac * H : smallFrac * H
    }

    // ── Colour helpers ───────────────────────────────────────────
    function healthColor(v) {
        if (v <= 0) return root.darkTheme ? "#2a3e58" : "#b0c8e0"
        if (v === 1) return root.darkTheme ? "#00e87a" : "#00a854"
        if (v === 2) return root.darkTheme ? "#ffb700" : "#e07800"
        return root.darkTheme ? "#ff3a3a" : "#d41010"
    }
    function statusLabel(v) {
        if (v <= 0)  return "NO DATA"
        if (v === 1) return "HEALTHY"
        if (v === 2) return "WARNING"
        return "CRITICAL"
    }
    function statusColor(v) {
        if (v <= 0)  return th.unknown
        if (v === 1) return th.healthy
        if (v === 2) return th.warning
        return th.critical
    }
    function borderFromStats(s) {
        if (s[2] > 0) return th.critical
        if (s[1] > 0) return th.warning
        if (s[0] > 0) return th.healthy
        return th.unknown
    }
    function healthPctFromStats(s) {
        var total = s[0] + s[1] + s[2]
        if (total <= 0) return "—"
        return Math.round(s[0] * 100 / total) + "%"
    }
    function fmtPwr(v) { return (v === undefined || v <= -900) ? "—" : v.toFixed(1) + " dBm" }
    function fmtTmp(v) { return (v === undefined || v <= -900) ? "—" : v.toFixed(1) + " °C"  }
    function fmtCur(v) { return (v === undefined || v <= -900) ? "—" : v.toFixed(3) + " A"   }
    function pwrStatus(v)  { if (v <= -900) return 0; if (v >= 43) return 1; if (v >= 40) return 2; return 3 }
    function tmpStatus(v)  { if (v <= -900) return 0; if (v <= 45) return 1; if (v <= 50) return 2; return 3 }
    function curStatus(v)  { if (v <= -900) return 0; if (v <= 1.8) return 1; if (v <= 2.0) return 2; return 3 }
    function healthPct(v)  { return v < 0 ? "—" : v + "%" }

    // ── Hover state ──────────────────────────────────────────────
    property int  hovType:     0
    property int  hovGlobalId: -1
    property int  hovQuad:     -1
    property int  hovCluster:  -1
    property int  hovValue:    -1
    property real hovMouseX:    0
    property real hovMouseY:    0

    function setHoverQuad(q, v, mx, my) {
        hovType=1; hovQuad=q; hovCluster=-1; hovGlobalId=-1; hovValue=v; hovMouseX=mx; hovMouseY=my
    }
    function setHoverCluster(q, c, v, mx, my) {
        hovType=2; hovQuad=q; hovCluster=c; hovGlobalId=q*model.elementsPerQuadrant+c*model.elementsPerCluster
        hovValue=v; hovMouseX=mx; hovMouseY=my
    }
    function setHoverElement(gid, v, mx, my) {
        hovType=3; hovGlobalId=gid; hovQuad=Math.floor(gid/model.elementsPerQuadrant)
        hovCluster=Math.floor((gid%model.elementsPerQuadrant)/model.elementsPerCluster)
        hovValue=v; hovMouseX=mx; hovMouseY=my
    }
    function clearHover() { hovType=0; hovGlobalId=-1 }

    // ── Navigation ───────────────────────────────────────────────
    function drillToQuadrant(q) {
        expandedQuad=-1; selQuad=q; lodLevel=2
        lod1View.opacity=0; lod1View.scale=0.93
        lod2View.scale=1.06; lod2View.opacity=1; lod2View.scale=1.0
        lod2Canvas.requestPaint()
    }
    function drillToCluster(c) {
        selCluster=c; lodLevel=3
        lod2View.opacity=0; lod2View.scale=0.93
        lod3View.scale=1.06; lod3View.opacity=1; lod3View.scale=1.0
        lod3Canvas.requestPaint()
    }
    function backTo(level) {
        clearHover()
        if (level===1) {
            lod2View.opacity=0; lod2View.scale=0.93
            lod3View.opacity=0; lod3View.scale=0.93
            lod1View.scale=1.06; lod1View.opacity=1; lod1View.scale=1.0
            lodLevel=1; selQuad=-1; selCluster=-1
            lod1Canvas.requestPaint()
        } else if (level===2) {
            lod3View.opacity=0; lod3View.scale=0.93
            lod2View.scale=1.06; lod2View.opacity=1; lod2View.scale=1.0
            lodLevel=2; selCluster=-1
            lod2Canvas.requestPaint()
        }
    }
    function showFullView() {
        clearHover(); expandedQuad=-1
        lod1View.opacity=0; lod2View.opacity=0; lod3View.opacity=0
        allView.scale=1.06; allView.opacity=1; allView.scale=1.0
        fullView=true; allCanvas.requestPaint()
    }
    function hideFullView() {
        clearHover(); allView.opacity=0; allView.scale=0.93; fullView=false
        lod1View.scale=1.06; lod1View.opacity=1; lod1View.scale=1.0
        lod1Canvas.requestPaint()
    }

    function switchAntenna(idx) {
        if (idx === currentAntenna) return
        clearHover(); expandedQuad=-1; fullView=false
        lodLevel=1; selQuad=-1; selCluster=-1
        lod1View.opacity=1; lod1View.scale=1.0
        lod2View.opacity=0; lod3View.opacity=0; allView.opacity=0
        currentAntenna=idx
        lod1Canvas.requestPaint()
        analyticsCanvas.model     = root.model
        analyticsCanvas.udpRecv   = root.udpRecv
        analyticsCanvas.cmdSender = root.cmdSender
        ctrlPanel.model           = root.model
    }

    // ════════════════════════════════════════════════════════════
    // HEADER
    // ════════════════════════════════════════════════════════════
    Rectangle {
        id: header
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 56
        color: th.header
        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
            height: 1; color: th.accent; opacity: 0.4
        }
        RowLayout {
            anchors { fill: parent; leftMargin: 18; rightMargin: 18 }
            spacing: 16

            Column {
                spacing: 1
                Text {
                    text: "RADAR ANTENNA MONITOR"
                    font { family:"Segoe UI"; pixelSize:16; bold:true; letterSpacing:1.2 }
                    color: Qt.rgba(1,1,1,0.92)
                }
                Text {
                    text: root.model ? root.model.antennaName + "  ·  " + root.model.totalElements + " elements" : ""
                    font { family:"Segoe UI"; pixelSize:10 }
                    color: Qt.rgba(1,1,1,0.40)
                }
            }

            // Antenna A / B sub-tabs
            Row {
                spacing: 0
                Repeater {
                    model: antennaCount
                    delegate: Rectangle {
                        width: antTxt.implicitWidth + 24; height: 32; radius: 4
                        color: root.currentAntenna===index ? Qt.rgba(1,1,1,0.15) : "transparent"
                        border { color: root.currentAntenna===index ? th.accent : "transparent"; width: 1 }
                        Column {
                            anchors.centerIn: parent; spacing: 0
                            Text {
                                id: antTxt
                                text: index===0 ? "Antenna A" : "Antenna B"
                                font { family:"Segoe UI"; pixelSize:12; bold: root.currentAntenna===index }
                                color: root.currentAntenna===index ? th.accent : Qt.rgba(1,1,1,0.5)
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: index===0 ? "1024 elem" : "2048 elem"
                                font { family:"Segoe UI"; pixelSize:9 }
                                color: Qt.rgba(1,1,1,0.30)
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: root.switchAntenna(index)
                        }
                    }
                }
            }

            Rectangle { width:1; height:30; color:Qt.rgba(1,1,1,0.15) }

            Item { Layout.fillWidth: true }

            // Status pills
            Row {
                spacing: 6
                Repeater {
                    model: [
                        { lbl:"HLTH", cnt: root.model ? root.model.healthyCount  : 0, col:"#00e87a", bg:"#003322" },
                        { lbl:"WARN", cnt: root.model ? root.model.warningCount  : 0, col:"#ffb700", bg:"#332400" },
                        { lbl:"CRIT", cnt: root.model ? root.model.criticalCount : 0, col:"#ff3a3a", bg:"#330808" },
                        { lbl:"UNKN", cnt: root.model ? root.model.noDataCount  : 0, col:"#8aaac8", bg:"#1a2a3a" }
                    ]
                    delegate: Rectangle {
                        height: 26; radius: 13; width: plRow.implicitWidth + 16
                        color: root.darkTheme ? modelData.bg : Qt.lighter(modelData.col, 1.8)
                        border { color: modelData.col; width: 1 }
                        Row {
                            id: plRow; anchors.centerIn: parent; spacing: 5
                            Rectangle {
                                width: 7; height: 7; radius: 3.5; color: modelData.col
                                anchors.verticalCenter: parent.verticalCenter
                                SequentialAnimation on opacity {
                                    running: modelData.lbl==="CRIT" && modelData.cnt > 0
                                    loops: Animation.Infinite
                                    NumberAnimation { to:0.2; duration:600 }
                                    NumberAnimation { to:1.0; duration:600 }
                                }
                            }
                            Text {
                                text: modelData.lbl + "  " + modelData.cnt
                                font { family:"Segoe UI"; pixelSize:11; bold:true }
                                color: root.darkTheme ? modelData.col : "#1a2840"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }

            Rectangle { width:1; height:30; color:Qt.rgba(1,1,1,0.12) }

            // UDP status + last update
            Column {
                spacing: 2
                Row {
                    spacing: 5
                    Rectangle {
                        width:7; height:7; radius:3.5; anchors.verticalCenter: parent.verticalCenter
                        color: root.udpRecv && root.udpRecv.listening ? "#00e87a" : "#ff3a3a"
                        SequentialAnimation on opacity {
                            running: root.udpRecv && root.udpRecv.listening; loops: Animation.Infinite
                            NumberAnimation { to: 0.2; duration: 800 }
                            NumberAnimation { to: 1.0; duration: 800 }
                        }
                    }
                    Text {
                        text: (root.udpRecv && root.udpRecv.listening) ? "UDP :" + (root.model ? root.model.udpPort : "") : "NO SIGNAL"
                        font { family:"Segoe UI"; pixelSize:11; bold:true }
                        color: (root.udpRecv && root.udpRecv.listening) ? "#00e87a" : "#ff3a3a"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Text {
                    text: root.model ? root.model.lastUpdateTime : "—"
                    font { family:"Courier New"; pixelSize:10 }
                    color: Qt.rgba(1,1,1,0.35)
                }
            }

            Rectangle { width:1; height:30; color:Qt.rgba(1,1,1,0.12) }

            // Theme toggle
            Rectangle {
                width:58; height:28; radius:14
                color: root.darkTheme ? "#1a3a5a" : "#ddeeff"
                border { color:th.accent; width:1 }
                Row {
                    anchors.centerIn: parent; spacing:4
                    Text { text:"☀"; font.pixelSize:13; color:root.darkTheme?"#4a6888":"#e07800"; anchors.verticalCenter:parent.verticalCenter }
                    Text { text:"●"; font.pixelSize:10; color:th.accent; anchors.verticalCenter:parent.verticalCenter }
                    Text { text:"☾"; font.pixelSize:13; color:root.darkTheme?th.accent:"#4a6888"; anchors.verticalCenter:parent.verticalCenter }
                }
                MouseArea { anchors.fill:parent; cursorShape:Qt.PointingHandCursor; onClicked: root.darkTheme=!root.darkTheme }
            }
        }
    }

    // ════════════════════════════════════════════════════════════
    // TOOLBAR
    // ════════════════════════════════════════════════════════════
    Rectangle {
        id: toolbar
        anchors { top:header.bottom; left:parent.left; right:parent.right }
        height: 40
        color: th.bar; border { color:th.border; width:1 }

        RowLayout {
            anchors { fill:parent; leftMargin:14; rightMargin:14 }
            spacing: 6

            // Back button
            Rectangle {
                visible: root.lodLevel > 1 && !root.fullView
                height:26; radius:5; width:bkT.implicitWidth+18
                color: "transparent"
                border { color: th.accent; width: 1 }
                Text {
                    id: bkT
                    anchors.centerIn: parent
                    text: "← Back"
                    font { family: "Segoe UI"; pixelSize: 12 }
                    color: th.accent
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.backTo(root.lodLevel-1)
                }
            }

            // Breadcrumb
            Row {
                spacing:0
                Text {
                    text: root.fullView ? "Full View" : "All Quadrants"
                    font{family:"Segoe UI";pixelSize:13;bold:(root.lodLevel===1&&!root.fullView)||root.fullView}
                    color: (root.lodLevel===1&&!root.fullView)||root.fullView ? th.accent : th.textDim
                    MouseArea { anchors.fill:parent; cursorShape:root.lodLevel>1?Qt.PointingHandCursor:Qt.ArrowCursor
                        onClicked: { if(root.fullView) root.hideFullView(); else if(root.lodLevel>1) root.backTo(1) } }
                }
                Text {
                    visible: root.lodLevel>=2 && !root.fullView
                    text: "  /  " + (root.selQuad>=0 ? root.model.antennaName + " Q" + (root.selQuad+1) : "")
                    font{family:"Segoe UI";pixelSize:13;bold:root.lodLevel===2}
                    color: root.lodLevel===2 ? th.accent : th.textDim
                    MouseArea { anchors.fill:parent; cursorShape:root.lodLevel>2?Qt.PointingHandCursor:Qt.ArrowCursor
                        onClicked: if(root.lodLevel>2) root.backTo(2) }
                }
                Text {
                    visible: root.lodLevel===3 && !root.fullView
                    text: "  /  Cluster " + (root.selCluster+1)
                    font { family: "Segoe UI"; pixelSize: 13; bold: true }
                    color: th.accent
                }
            }

            Item { Layout.fillWidth:true }

            // LOD badge
            Rectangle {
                visible:!root.fullView; height:26; radius:5; width:lodT.implicitWidth+18
                color: root.lodLevel===1?(root.darkTheme?"#0a2448":"#cce0f8"):root.lodLevel===2?(root.darkTheme?"#1a2808":"#d8f0cc"):(root.darkTheme?"#281008":"#f8ddd8")
                border.color: root.lodLevel===1?th.accent:root.lodLevel===2?th.warning:th.critical; border.width:1
                Text { id:lodT; anchors.centerIn:parent
                    text:root.lodLevel===1?"LOD 1  Overview":root.lodLevel===2?"LOD 2  Clusters":"LOD 3  Elements"
                    font{family:"Segoe UI";pixelSize:11;bold:true}
                    color:root.lodLevel===1?th.accent:root.lodLevel===2?th.warning:th.critical }
            }

            // Expand reset
            Rectangle {
                visible: root.expandedQuad >= 0 && root.lodLevel===1 && !root.fullView
                height:26; radius:5; width:expResetT.implicitWidth+18
                color: root.darkTheme?"#1a1000":"#fff0cc"
                border { color:th.warning; width:1 }
                Text {
                    id: expResetT
                    anchors.centerIn: parent
                    text: "⊟  Reset Layout"
                    font { family: "Segoe UI"; pixelSize: 11 }
                    color: th.warning
                }
                MouseArea { anchors.fill:parent; cursorShape:Qt.PointingHandCursor
                    onClicked: { root.expandedQuad=-1; lod1Canvas.requestPaint() } }
            }

            // All elements button
            Rectangle {
                height:26; radius:5; width:allT.implicitWidth+18
                color: root.fullView?(root.darkTheme?"#0a2808":"#d8f0cc"):"transparent"
                border { color:root.fullView?th.healthy:th.border; width:1 }
                Text { id:allT; anchors.centerIn:parent
                    text:root.fullView?"⊟  Close Full":"⊞  All "+( root.model?root.model.totalElements:0)
                    font{family:"Segoe UI";pixelSize:11;bold:root.fullView}
                    color:root.fullView?th.healthy:th.textDim }
                MouseArea { anchors.fill:parent; cursorShape:Qt.PointingHandCursor
                    onClicked: root.fullView?root.hideFullView():root.showFullView() }
            }

            // Analytics toggle
            Rectangle {
                height:26; radius:5; width:anaT.implicitWidth+18
                color: root.analyticsOpen?(root.darkTheme?"#0a2448":"#cce0f8"):"transparent"
                border { color:root.analyticsOpen?th.accent:th.border; width:1 }
                Text { id:anaT; anchors.centerIn:parent
                    text:root.analyticsOpen?"✕  Analytics":"📊  Analytics"
                    font{family:"Segoe UI";pixelSize:11;bold:root.analyticsOpen}
                    color:root.analyticsOpen?th.accent:th.textDim }
                MouseArea { anchors.fill:parent; cursorShape:Qt.PointingHandCursor
                    onClicked: root.analyticsOpen=!root.analyticsOpen }
            }

            // Simulator button
            Rectangle {
                height:26; radius:5; width:simRow.implicitWidth+18
                color: root.sim&&root.sim.running?(root.darkTheme?"#2a0808":"#ffe0e0"):"transparent"
                border { color:root.sim&&root.sim.running?th.critical:th.border; width:1 }
                Row {
                    id:simRow; anchors.centerIn:parent; spacing:5
                    Rectangle {
                        width:7; height:7; radius:3.5
                        color: root.sim&&root.sim.running ? th.critical : th.textDim
                        anchors.verticalCenter:parent.verticalCenter
                        SequentialAnimation on opacity {
                            running: root.sim&&root.sim.running; loops:Animation.Infinite
                            NumberAnimation { to: 0.2; duration: 500 }
                            NumberAnimation { to: 1.0; duration: 500 }
                        }
                    }
                    Text {
                        text: root.sim&&root.sim.running ? "Stop Sim" : "▶ Simulate"
                        font{family:"Segoe UI";pixelSize:11;bold:root.sim&&root.sim.running}
                        color:root.sim&&root.sim.running?th.critical:th.textDim
                        anchors.verticalCenter:parent.verticalCenter
                    }
                }
                MouseArea { anchors.fill:parent; cursorShape:Qt.PointingHandCursor
                    onClicked: { if(root.sim) root.sim.running?root.sim.stop():root.sim.start() } }
            }

            Text {
                visible: root.sim && root.sim.running
                text: root.sim ? root.sim.scenario : ""
                font { family: "Segoe UI"; pixelSize: 11; italic: true }
                color: th.warning
                SequentialAnimation on opacity {
                    running: root.sim&&root.sim.running; loops:Animation.Infinite
                    NumberAnimation { to: 0.3; duration: 1100 }
                    NumberAnimation { to: 1.0; duration: 1100 }
                }
            }
        }
    }

    // ════════════════════════════════════════════════════════════
    // MAIN AREA: viewport + analytics panel
    // ════════════════════════════════════════════════════════════
    Item {
        id: mainArea
        anchors { top:toolbar.bottom; bottom:footer.top; left:parent.left; right:parent.right; margins:8 }

        // ── Analytics panel (slides in from right) ────────────────
        Rectangle {
            id: analyticsPane
            anchors { top:parent.top; bottom:parent.bottom; right:parent.right }
            width: root.analyticsOpen ? 280 : 0
            color: th.surface
            border { color:th.border; width:1 }
            clip: true
            Behavior on width { NumberAnimation { duration:250; easing.type:Easing.InOutCubic } }

            AnalyticsPanel {
                id: analyticsCanvas
                anchors { fill:parent; margins:1 }
                model:     root.model
                udpRecv:   root.udpRecv
                cmdSender: root.cmdSender
                dark:      root.darkTheme
            }
        }

        // ── Viewport ──────────────────────────────────────────────
        Item {
            id: viewport
            anchors { fill:parent; rightMargin: root.analyticsOpen ? analyticsPane.width + 4 : 0 }
            Behavior on anchors.rightMargin { NumberAnimation { duration:250; easing.type:Easing.InOutCubic } }

            // ── LOD 1 — Quadrant overview ─────────────────────────
            Item {
                id: lod1View
                anchors.fill: parent; opacity:1; scale:1.0; transformOrigin:Item.Center
                Behavior on opacity { NumberAnimation{duration:230;easing.type:Easing.InOutCubic} }
                Behavior on scale   { NumberAnimation{duration:230;easing.type:Easing.InOutCubic} }

                Canvas {
                    id: lod1Canvas
                    anchors.fill: parent

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0,0,width,height)
                        var Q = root.model ? root.model.quadrantCount : 4
                        var qNames = ["Q1 — NW","Q2 — NE","Q3 — SW","Q4 — SE"]
                        var r2 = 10

                        for (var q = 0; q < Q; q++) {
                            var x = root.qX(q, width)  + 5
                            var y = root.qY(q, height) + 5
                            var w = root.qW(q, width)  - 10
                            var h = root.qH(q, height) - 10
                            var v   = root.model ? root.model.quadrantHealth(q) : -1
                            var s   = root.model ? root.model.quadrantStats(q)  : [0,0,0,0]
                            var hov = (q === lod1Hov.hovCell)
                            var isExp = (root.expandedQuad === q)

                            ctx.fillStyle = hov ? th.surfaceHov : th.surface2
                            ctx.beginPath()
                            ctx.moveTo(x+r2,y);ctx.lineTo(x+w-r2,y);ctx.quadraticCurveTo(x+w,y,x+w,y+r2)
                            ctx.lineTo(x+w,y+h-r2);ctx.quadraticCurveTo(x+w,y+h,x+w-r2,y+h)
                            ctx.lineTo(x+r2,y+h);ctx.quadraticCurveTo(x,y+h,x,y+h-r2)
                            ctx.lineTo(x,y+r2);ctx.quadraticCurveTo(x,y,x+r2,y)
                            ctx.closePath(); ctx.fill()

                            var bc = root.borderFromStats(s)
                            ctx.fillStyle = bc; ctx.fillRect(x,y+r2,4,h-r2*2)

                            ctx.strokeStyle = isExp ? th.accent : (hov ? th.accent : th.border)
                            ctx.lineWidth = isExp ? 2.5 : (hov ? 2 : 1)
                            ctx.beginPath()
                            ctx.moveTo(x+r2,y);ctx.lineTo(x+w-r2,y);ctx.quadraticCurveTo(x+w,y,x+w,y+r2)
                            ctx.lineTo(x+w,y+h-r2);ctx.quadraticCurveTo(x+w,y+h,x+w-r2,y+h)
                            ctx.lineTo(x+r2,y+h);ctx.quadraticCurveTo(x,y+h,x,y+h-r2)
                            ctx.lineTo(x,y+r2);ctx.quadraticCurveTo(x,y,x+r2,y)
                            ctx.closePath(); ctx.stroke()

                            var isTiny = (w < 120 || h < 100)

                            ctx.fillStyle = th.accent
                            ctx.font = "bold " + (isTiny?10:13) + "px 'Segoe UI'"
                            ctx.textAlign = "left"
                            ctx.fillText((isTiny ? "Q"+(q+1) : qNames[q]), x+14, y+22)

                            if (!isTiny) {
                                ctx.fillStyle = th.textDim; ctx.font="10px 'Segoe UI'"
                                ctx.fillText(root.model ? root.model.elementsPerQuadrant+" elements" : "", x+14, y+36)
                            }

                            var st1 = s[2]>0 ? 3 : s[1]>0 ? 2 : s[0]>0 ? 1 : 0
                            ctx.fillStyle = root.healthColor(st1)
                            var bigSz = isTiny ? Math.round(h*0.30) : Math.round(h*0.26)
                            ctx.font = "bold " + bigSz + "px 'Segoe UI'"
                            ctx.textAlign = "center"
                            ctx.fillText(root.healthPct(v), x+w/2, y+h*(isTiny?0.55:0.52))

                            if (!isTiny) {
                                ctx.fillStyle = root.healthColor(st1); ctx.font="11px 'Segoe UI'"
                                ctx.fillText(root.statusLabel(st1), x+w/2, y+h*0.52+20)
                                var cNames=["H","W","C","U"]
                                var cCols=[th.healthy,th.warning,th.critical,th.unknown]
                                for (var ci=0;ci<4;ci++) {
                                    ctx.fillStyle=cCols[ci]; ctx.font="10px 'Segoe UI'"
                                    ctx.textAlign="left"
                                    ctx.fillText(cNames[ci]+":"+s[ci], x+14+(ci*Math.min(50,w/4-2)), y+h-24)
                                }
                            }

                            var total2 = root.model ? root.model.elementsPerQuadrant : 256
                            var barSegs=[[s[0]/total2,th.healthy],[s[1]/total2,th.warning],[s[2]/total2,th.critical],[s[3]/total2,th.unknown]]
                            var bx2=x, by2=y+h-8
                            for (var si=0;si<4;si++){ctx.fillStyle=barSegs[si][1];var bw=w*barSegs[si][0];ctx.fillRect(bx2,by2,bw,6);bx2+=bw}

                            if (isExp && !isTiny) {
                                ctx.fillStyle=th.accent; ctx.globalAlpha=0.7; ctx.font="10px 'Segoe UI'"
                                ctx.textAlign="right"
                                ctx.fillText("EXPANDED  ●", x+w-10, y+20)
                                ctx.globalAlpha=1.0
                            }
                            if (hov && !isTiny) {
                                ctx.fillStyle=th.accent; ctx.globalAlpha=0.8; ctx.font="10px 'Segoe UI'"
                                ctx.textAlign="right"
                                ctx.fillText("Click: clusters  |  ⊞: expand", x+w-10, y+20)
                                ctx.globalAlpha=1.0
                            }
                        }
                        ctx.textAlign="left"
                    }
                }

                MouseArea {
                    id: lod1Hov; anchors.fill:parent; hoverEnabled:true
                    cursorShape: Qt.PointingHandCursor
                    property int hovCell: -1

                    function quadAt(mx, my) {
                        var Q = root.model ? root.model.quadrantCount : 4
                        for (var q=0; q<Q; q++) {
                            var x=root.qX(q,width), y=root.qY(q,height)
                            var w=root.qW(q,width), h=root.qH(q,height)
                            if (mx>=x && mx<x+w && my>=y && my<y+h) return q
                        }
                        return -1
                    }

                    onPositionChanged: {
                        var q = quadAt(mouse.x, mouse.y)
                        if (q !== hovCell) { hovCell=q; lod1Canvas.requestPaint() }
                        if (q >= 0) root.setHoverQuad(q, root.model?root.model.quadrantHealth(q):-1, mouse.x, mouse.y)
                        else root.clearHover()
                    }
                    onExited: { hovCell=-1; root.clearHover(); lod1Canvas.requestPaint() }
                    onClicked: {
                        var q = quadAt(mouse.x, mouse.y)
                        if (q >= 0) root.drillToQuadrant(q)
                    }
                    onDoubleClicked: {
                        var q = quadAt(mouse.x, mouse.y)
                        if (q < 0) return
                        root.expandedQuad = (root.expandedQuad === q) ? -1 : q
                        lod1Canvas.requestPaint()
                    }
                }

                Repeater {
                    model: root.model ? root.model.quadrantCount : 4
                    delegate: Rectangle {
                        property int qi: index
                        x: root.qX(qi, lod1View.width)  + root.qW(qi, lod1View.width)  - 60
                        y: root.qY(qi, lod1View.height) + 8
                        width: 52; height: 22; radius: 4
                        color: root.expandedQuad===qi ? Qt.rgba(0.1,0.7,1,0.25) : Qt.rgba(0,0,0,0.35)
                        border { color: root.expandedQuad===qi ? th.accent : th.border; width:1 }

                        Behavior on x { NumberAnimation{duration:280;easing.type:Easing.InOutCubic} }
                        Behavior on y { NumberAnimation{duration:280;easing.type:Easing.InOutCubic} }

                        Text {
                            anchors.centerIn: parent
                            text: root.expandedQuad===qi ? "⊟ Shrink" : "⊞ Expand"
                            font { family:"Segoe UI"; pixelSize:9; bold:true }
                            color: root.expandedQuad===qi ? th.accent : th.textDim
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.expandedQuad = (root.expandedQuad===qi) ? -1 : qi
                                lod1Canvas.requestPaint()
                            }
                        }
                    }
                }
            }

            // ── LOD 2 — Clusters ─────────────────────────────────
            Item {
                id: lod2View
                anchors.fill:parent; opacity:0; scale:1.0; visible:opacity>0.01
                transformOrigin:Item.Center
                Behavior on opacity { NumberAnimation{duration:230;easing.type:Easing.InOutCubic} }
                Behavior on scale   { NumberAnimation{duration:230;easing.type:Easing.InOutCubic} }

                Canvas {
                    id: lod2Canvas; anchors.fill:parent
                    onPaint: {
                        if (root.selQuad<0||!root.model) return
                        var ctx=getContext("2d"); ctx.clearRect(0,0,width,height)
                        var totalC = root.model.clustersPerQuadrant
                        var COLS   = Math.min(4, totalC)
                        var ROWS   = Math.ceil(totalC / COLS)
                        var pad=8, cw=width/COLS, ch=height/ROWS, r2=8

                        for (var c=0;c<totalC;c++) {
                            var gc=c%COLS,gr=Math.floor(c/COLS)
                            var x=gc*cw+pad,y=gr*ch+pad,w=cw-pad*2,h=ch-pad*2
                            var v=root.model.clusterHealth(root.selQuad,c)
                            var s=root.model.clusterStats(root.selQuad,c)
                            var hov=(c===lod2Hov.hovCell)

                            ctx.fillStyle=hov?th.surfaceHov:th.surface2
                            ctx.beginPath()
                            ctx.moveTo(x+r2,y);ctx.lineTo(x+w-r2,y);ctx.quadraticCurveTo(x+w,y,x+w,y+r2)
                            ctx.lineTo(x+w,y+h-r2);ctx.quadraticCurveTo(x+w,y+h,x+w-r2,y+h)
                            ctx.lineTo(x+r2,y+h);ctx.quadraticCurveTo(x,y+h,x,y+h-r2)
                            ctx.lineTo(x,y+r2);ctx.quadraticCurveTo(x,y,x+r2,y)
                            ctx.closePath();ctx.fill()

                            ctx.fillStyle=root.borderFromStats(s); ctx.fillRect(x,y+r2,4,h-r2*2)
                            ctx.strokeStyle=hov?th.accent:th.border; ctx.lineWidth=hov?2:1
                            ctx.beginPath()
                            ctx.moveTo(x+r2,y);ctx.lineTo(x+w-r2,y);ctx.quadraticCurveTo(x+w,y,x+w,y+r2)
                            ctx.lineTo(x+w,y+h-r2);ctx.quadraticCurveTo(x+w,y+h,x+w-r2,y+h)
                            ctx.lineTo(x+r2,y+h);ctx.quadraticCurveTo(x,y+h,x,y+h-r2)
                            ctx.lineTo(x,y+r2);ctx.quadraticCurveTo(x,y,x+r2,y)
                            ctx.closePath();ctx.stroke()

                            var isTinyC = (w < 80 || h < 80)
                            var base = c*root.model.elementsPerCluster
                            var eid0 = root.selQuad*root.model.elementsPerQuadrant+base

                            ctx.fillStyle=th.accent
                            ctx.font="bold "+(isTinyC?10:12)+"px 'Segoe UI'"; ctx.textAlign="left"
                            ctx.fillText("C"+(c+1), x+10, y+(isTinyC?16:20))

                            if (!isTinyC) {
                                ctx.fillStyle=th.textDim; ctx.font="9px 'Segoe UI'"
                                ctx.fillText("E"+eid0+"–E"+(eid0+root.model.elementsPerCluster-1), x+10, y+32)
                            }

                            var st2 = s[2]>0 ? 3 : s[1]>0 ? 2 : s[0]>0 ? 1 : 0
                            ctx.fillStyle=root.healthColor(st2)
                            ctx.font="bold "+Math.round(h*(isTinyC?0.32:0.26))+"px 'Segoe UI'"
                            ctx.textAlign="center"
                            ctx.fillText(root.healthPct(v), x+w/2, y+h*(isTinyC?0.65:0.60))

                            if (!isTinyC) {
                                ctx.font="10px 'Segoe UI'"; ctx.fillText(root.statusLabel(st2),x+w/2,y+h*0.60+16)
                            }

                            var barSegsC=[[s[0]/root.model.elementsPerCluster,th.healthy],[s[1]/root.model.elementsPerCluster,th.warning],
                                          [s[2]/root.model.elementsPerCluster,th.critical],[s[3]/root.model.elementsPerCluster,th.unknown]]
                            var bx3=x,by3=y+h-7
                            for (var si=0;si<4;si++){ctx.fillStyle=barSegsC[si][1];var bw2=w*barSegsC[si][0];ctx.fillRect(bx3,by3,bw2,5);bx3+=bw2}
                        }
                        ctx.textAlign="left"
                    }
                }
                MouseArea {
                    id:lod2Hov; anchors.fill:parent; hoverEnabled:true
                    cursorShape:Qt.PointingHandCursor; property int hovCell:-1
                    onPositionChanged: {
                        var totalC=root.model?root.model.clustersPerQuadrant:16
                        var COLS=Math.min(4,totalC),ROWS=Math.ceil(totalC/COLS)
                        var c=Math.min(Math.floor(mouse.y/(parent.height/ROWS))*COLS+Math.floor(mouse.x/(parent.width/COLS)),totalC-1)
                        if (c!==hovCell){hovCell=c;lod2Canvas.requestPaint()}
                        root.setHoverCluster(root.selQuad,c,root.model?root.model.clusterHealth(root.selQuad,c):-1,mouse.x,mouse.y)
                    }
                    onExited: {hovCell=-1;root.clearHover();lod2Canvas.requestPaint()}
                    onClicked: {
                        var totalC=root.model?root.model.clustersPerQuadrant:16
                        var COLS=Math.min(4,totalC),ROWS=Math.ceil(totalC/COLS)
                        var c=Math.min(Math.floor(mouse.y/(parent.height/ROWS))*COLS+Math.floor(mouse.x/(parent.width/COLS)),totalC-1)
                        root.drillToCluster(c)
                    }
                }
            }

            // ── LOD 3 — Elements ──────────────────────────────────
            Item {
                id:lod3View; anchors.fill:parent; opacity:0; scale:1.0; visible:opacity>0.01
                transformOrigin:Item.Center
                Behavior on opacity { NumberAnimation{duration:230;easing.type:Easing.InOutCubic} }
                Behavior on scale   { NumberAnimation{duration:230;easing.type:Easing.InOutCubic} }
                Canvas {
                    id:lod3Canvas; anchors.fill:parent
                    onPaint: {
                        if (root.selQuad<0||root.selCluster<0||!root.model) return
                        var ctx=getContext("2d"); ctx.clearRect(0,0,width,height)
                        var EPCl=root.model.elementsPerCluster
                        var COLS=Math.ceil(Math.sqrt(EPCl))
                        var ROWS=Math.ceil(EPCl/COLS)
                        var pad=12,cw=width/COLS,ch=height/ROWS,r2=10
                        var data=root.model.clusterData(root.selQuad,root.selCluster)
                        var base=root.selCluster*EPCl

                        for (var e=0;e<EPCl;e++) {
                            var gc=e%COLS,gr=Math.floor(e/COLS)
                            var x=gc*cw+pad,y=gr*ch+pad,w=cw-pad*2,h=ch-pad*2
                            var v=data[e]
                            var gid=root.selQuad*root.model.elementsPerQuadrant+base+e
                            var hov=(gid===lod3Hov.hovGid)

                            ctx.fillStyle=root.healthColor(v); ctx.globalAlpha=hov?0.32:0.16
                            ctx.beginPath()
                            ctx.moveTo(x+r2,y);ctx.lineTo(x+w-r2,y);ctx.quadraticCurveTo(x+w,y,x+w,y+r2)
                            ctx.lineTo(x+w,y+h-r2);ctx.quadraticCurveTo(x+w,y+h,x+w-r2,y+h)
                            ctx.lineTo(x+r2,y+h);ctx.quadraticCurveTo(x,y+h,x,y+h-r2)
                            ctx.lineTo(x,y+r2);ctx.quadraticCurveTo(x,y,x+r2,y)
                            ctx.closePath();ctx.fill();ctx.globalAlpha=1.0

                            ctx.strokeStyle=hov?root.healthColor(v):th.border; ctx.lineWidth=hov?2.5:1
                            ctx.beginPath()
                            ctx.moveTo(x+r2,y);ctx.lineTo(x+w-r2,y);ctx.quadraticCurveTo(x+w,y,x+w,y+r2)
                            ctx.lineTo(x+w,y+h-r2);ctx.quadraticCurveTo(x+w,y+h,x+w-r2,y+h)
                            ctx.lineTo(x+r2,y+h);ctx.quadraticCurveTo(x,y+h,x,y+h-r2)
                            ctx.lineTo(x,y+r2);ctx.quadraticCurveTo(x,y,x+r2,y)
                            ctx.closePath();ctx.stroke()

                            ctx.fillStyle=root.healthColor(v)
                            ctx.font="bold "+Math.round(Math.min(w,h)*0.17)+"px 'Segoe UI'"
                            ctx.textAlign="left"; ctx.fillText("E"+gid, x+10, y+Math.round(h*0.22))

                            ctx.fillStyle=th.textDim; ctx.font="9px 'Segoe UI'"
                            ctx.fillText("#"+e+" / C"+(root.selCluster+1), x+10, y+Math.round(h*0.22)+14)

                            ctx.strokeStyle=th.border; ctx.globalAlpha=0.35; ctx.lineWidth=1
                            ctx.beginPath(); ctx.moveTo(x+8,y+Math.round(h*0.36)); ctx.lineTo(x+w-8,y+Math.round(h*0.36)); ctx.stroke()
                            ctx.globalAlpha=1.0

                            ctx.fillStyle=root.healthColor(v)
                            ctx.font="bold "+Math.round(h*0.20)+"px 'Segoe UI'"; ctx.textAlign="center"
                            ctx.fillText(root.statusLabel(v), x+w/2, y+h*0.62)

                            ctx.fillStyle=th.textDim; ctx.font="9px 'Segoe UI'"
                            ctx.fillText(v===1?"OK":v===2?"WARN":v===3?"CRIT":"N/A", x+w/2, y+h*0.62+14)
                        }
                        ctx.textAlign="left"
                    }
                }
                MouseArea {
                    id:lod3Hov; anchors.fill:parent; hoverEnabled:true; property int hovGid:-1
                    cursorShape: Qt.PointingHandCursor
                    onPositionChanged: {
                        if (!root.model) return
                        var EPCl=root.model.elementsPerCluster
                        var COLS=Math.ceil(Math.sqrt(EPCl))
                        var cw2=parent.width/COLS,ch2=parent.height/Math.ceil(EPCl/COLS)
                        var e=Math.min(Math.floor(mouse.y/ch2)*COLS+Math.floor(mouse.x/cw2),EPCl-1)
                        var gid=root.selQuad*root.model.elementsPerQuadrant+root.selCluster*EPCl+e
                        if (gid!==hovGid){hovGid=gid;lod3Canvas.requestPaint()}
                        var data=root.model.clusterData(root.selQuad,root.selCluster)
                        root.setHoverElement(gid,data[e],mouse.x,mouse.y)
                    }
                    onExited: { hovGid=-1; root.clearHover(); lod3Canvas.requestPaint() }
                    onClicked: {
                        if (!root.model) return
                        var EPCl=root.model.elementsPerCluster
                        var COLS=Math.ceil(Math.sqrt(EPCl))
                        var cw2=parent.width/COLS,ch2=parent.height/Math.ceil(EPCl/COLS)
                        var e=Math.min(Math.floor(mouse.y/ch2)*COLS+Math.floor(mouse.x/cw2),EPCl-1)
                        var gid=root.selQuad*root.model.elementsPerQuadrant+root.selCluster*EPCl+e
                        root.openControlPanel(gid, root.selQuad, root.selCluster*EPCl+e)
                    }
                }
            }

            // ── FULL VIEW — All elements ──────────────────────────
            Item {
                id:allView; anchors.fill:parent; opacity:0; scale:1.0; visible:opacity>0.01
                transformOrigin:Item.Center
                Behavior on opacity { NumberAnimation{duration:230;easing.type:Easing.InOutCubic} }
                Behavior on scale   { NumberAnimation{duration:230;easing.type:Easing.InOutCubic} }
                Canvas {
                    id:allCanvas; anchors.fill:parent
                    onPaint: {
                        if (!root.model) return
                        var ctx=getContext("2d"); ctx.clearRect(0,0,width,height)
                        var legendW=152, gridW=width-legendW-10, gridH=height
                        var EPQ=root.model.elementsPerQuadrant
                        var Q=root.model.quadrantCount
                        var elemCols=16, elemRows=EPQ/16
                        var totalCols=elemCols*2, totalRows=elemRows*2
                        var cellW=gridW/totalCols, cellH=gridH/totalRows

                        for (var q=0;q<Q;q++){
                            var qdata=root.model.quadrantData(q)
                            var qCO=(q%2)*elemCols, qRO=Math.floor(q/2)*elemRows
                            for (var e=0;e<EPQ;e++){
                                var eC=e%elemCols,eR=Math.floor(e/elemCols)
                                var gc2=qCO+eC,gr2=qRO+eR
                                var gap=Math.max(0.5,Math.min(cellW,cellH)*0.08)
                                var px=gc2*cellW+gap,py=gr2*cellH+gap
                                var pw=cellW-gap*2,ph=cellH-gap*2
                                var v=qdata[e],gid=q*EPQ+e
                                var hov=(gid===allHov.hovGid)
                                ctx.fillStyle=root.healthColor(v)
                                ctx.globalAlpha=hov?1.0:0.82; ctx.fillRect(px,py,pw,ph); ctx.globalAlpha=1.0
                                if (cellW>18) {
                                    ctx.fillStyle="rgba(0,0,0,0.5)"
                                    ctx.font="bold "+Math.max(6,Math.round(cellW*0.26))+"px 'Segoe UI'"
                                    ctx.textAlign="center"
                                    ctx.fillText(String(gid),px+pw/2,py+ph*0.65)
                                }
                                if (hov){ctx.strokeStyle="#fff";ctx.lineWidth=2;ctx.strokeRect(px-1,py-1,pw+2,ph+2)}
                            }
                        }
                        ctx.strokeStyle=root.darkTheme?"rgba(26,180,240,0.10)":"rgba(0,80,160,0.10)"
                        ctx.lineWidth=0.5
                        for (var cc=4;cc<totalCols;cc+=4){if(cc===totalCols/2)continue;ctx.beginPath();ctx.moveTo(cc*cellW,0);ctx.lineTo(cc*cellW,gridH);ctx.stroke()}
                        for (var rr=elemRows/4;rr<totalRows;rr+=elemRows/4){if(rr===totalRows/2)continue;ctx.beginPath();ctx.moveTo(0,rr*cellH);ctx.lineTo(gridW,rr*cellH);ctx.stroke()}
                        ctx.strokeStyle=root.darkTheme?"rgba(26,180,240,0.55)":"rgba(0,80,160,0.55)"; ctx.lineWidth=2
                        ctx.beginPath();ctx.moveTo((totalCols/2)*cellW,0);ctx.lineTo((totalCols/2)*cellW,gridH);ctx.stroke()
                        ctx.beginPath();ctx.moveTo(0,(totalRows/2)*cellH);ctx.lineTo(gridW,(totalRows/2)*cellH);ctx.stroke()
                        var qLbls=[{x:4,y:0},{x:(totalCols/2)*cellW+4,y:0},{x:4,y:(totalRows/2)*cellH},{x:(totalCols/2)*cellW+4,y:(totalRows/2)*cellH}]
                        var qLNames=["Q1 NW","Q2 NE","Q3 SW","Q4 SE"]
                        for (var ql=0;ql<Q;ql++){
                            var qs2=root.model.quadrantStats(ql),qv2=root.model.quadrantHealth(ql)
                            ctx.fillStyle=root.borderFromStats(qs2); ctx.font="bold 11px 'Segoe UI'"; ctx.textAlign="left"
                            ctx.fillText(qLNames[ql]+"  "+root.healthPct(qv2),qLbls[ql].x+2,qLbls[ql].y+15)
                        }
                        var lx=gridW+10
                        ctx.fillStyle=th.surface2; ctx.fillRect(lx,0,legendW,gridH)
                        ctx.strokeStyle=th.border; ctx.lineWidth=1; ctx.strokeRect(lx+0.5,0.5,legendW-1,gridH-1)
                        ctx.fillStyle=th.accent; ctx.font="bold 11px 'Segoe UI'"; ctx.textAlign="left"; ctx.fillText("STATUS",lx+10,18)
                        var bx4=lx+10,by4=28,bw4=20,bh4=16,bGap=6
                        var swatches=[[th.healthy,"HEALTHY"],[th.warning,"WARNING"],[th.critical,"CRITICAL"],[th.unknown,"NO DATA"]]
                        for(var si=0;si<4;si++){
                            ctx.fillStyle=swatches[si][0]; ctx.fillRect(bx4,by4+si*(bh4+bGap),bw4,bh4)
                            ctx.fillStyle=th.textDim; ctx.font="9px 'Segoe UI'"
                            ctx.fillText(swatches[si][1],bx4+bw4+4,by4+si*(bh4+bGap)+12)
                        }
                        var sy=by4+4*(bh4+bGap)+14
                        ctx.strokeStyle=th.border;ctx.beginPath();ctx.moveTo(lx+6,sy);ctx.lineTo(lx+legendW-6,sy);ctx.stroke(); sy+=10
                        ctx.fillStyle=th.accent;ctx.font="bold 10px 'Segoe UI'";ctx.fillText("TOTALS",bx4,sy); sy+=14
                        var tots=[[th.healthy,"H: "+root.model.healthyCount],[th.warning,"W: "+root.model.warningCount],[th.critical,"C: "+root.model.criticalCount],[th.unknown,"ND: "+root.model.noDataCount]]
                        for(var ti=0;ti<4;ti++){ctx.fillStyle=tots[ti][0];ctx.font="10px 'Segoe UI'";ctx.fillText(tots[ti][1],bx4,sy);sy+=14}
                        sy+=6; ctx.strokeStyle=th.border;ctx.beginPath();ctx.moveTo(lx+6,sy);ctx.lineTo(lx+legendW-6,sy);ctx.stroke(); sy+=10
                        ctx.fillStyle=th.accent;ctx.font="bold 9px 'Segoe UI'";ctx.fillText("THRESHOLDS",bx4,sy); sy+=12
                        ctx.fillStyle=th.textDim;ctx.font="9px 'Segoe UI'"
                        ctx.fillText("Pwr  >=43 dBm",bx4,sy);sy+=12
                        ctx.fillText("Temp <=50 deg C",bx4,sy);sy+=12
                        ctx.fillText("Curr <=2.0 A",bx4,sy)
                        if(allHov.hovGid>=0){
                            var hq2=Math.floor(allHov.hovGid/EPQ),he2=allHov.hovGid%EPQ
                            var hvSt=root.model.quadrantData(hq2)[he2]
                            var det=root.model.elementDetail(hq2,he2)
                            var detY=gridH-150
                            ctx.fillStyle=root.darkTheme?"#0a1a2e":"#e0eef8"
                            ctx.fillRect(lx+6,detY,legendW-12,144)
                            ctx.strokeStyle=th.accent;ctx.lineWidth=1;ctx.strokeRect(lx+6.5,detY+0.5,legendW-13,143)
                            ctx.fillStyle=th.accent;ctx.font="bold 10px 'Segoe UI'";ctx.fillText("ELEMENT  E"+allHov.hovGid,lx+12,detY+14)
                            ctx.fillStyle=th.text;ctx.font="10px 'Segoe UI'"
                            ctx.fillText("Quad:   Q"+(hq2+1),lx+12,detY+28)
                            ctx.fillText("Local:  #"+he2,lx+12,detY+42)
                            ctx.fillStyle=root.healthColor(det.powerStatus||0)
                            ctx.fillText("Power:  "+root.fmtPwr(det.power),lx+12,detY+60)
                            ctx.fillStyle=root.healthColor(det.tempStatus||0)
                            ctx.fillText("Temp:   "+root.fmtTmp(det.temperature),lx+12,detY+74)
                            ctx.fillStyle=root.healthColor(det.currStatus||0)
                            ctx.fillText("Curr:   "+root.fmtCur(det.current),lx+12,detY+88)
                            ctx.fillStyle=root.healthColor(hvSt);ctx.font="bold 11px 'Segoe UI'"
                            ctx.fillText(root.statusLabel(hvSt),lx+12,detY+110)
                        }
                        ctx.textAlign="left"
                    }
                }
                MouseArea {
                    id:allHov; anchors.fill:parent; hoverEnabled:true; property int hovGid:-1
                    cursorShape: Qt.PointingHandCursor
                    onPositionChanged: {
                        if(!root.model) return
                        var legendW=152,gridW=parent.width-legendW-10
                        if(mouse.x>=gridW){if(hovGid!==-1){hovGid=-1;allCanvas.requestPaint();root.clearHover()};return}
                        var EPQ=root.model.elementsPerQuadrant
                        var elemCols=16,elemRows=EPQ/16
                        var totalCols=elemCols*2,totalRows=elemRows*2
                        var cellW=gridW/totalCols,cellH=parent.height/totalRows
                        var gc2=Math.min(Math.floor(mouse.x/cellW),totalCols-1)
                        var gr2=Math.min(Math.floor(mouse.y/cellH),totalRows-1)
                        var q2=Math.floor(gr2/elemRows)*2+Math.floor(gc2/elemCols)
                        var e2=(gr2%elemRows)*elemCols+(gc2%elemCols)
                        var gid2=q2*EPQ+e2
                        if(gid2!==hovGid){hovGid=gid2;allCanvas.requestPaint()}
                        root.setHoverElement(gid2,root.model.quadrantData(q2)[e2],mouse.x,mouse.y)
                    }
                    onExited: { hovGid=-1; root.clearHover(); allCanvas.requestPaint() }
                    onClicked: {
                        if(!root.model) return
                        var legendW=152,gridW=parent.width-legendW-10
                        if(mouse.x>=gridW) return
                        var EPQ=root.model.elementsPerQuadrant
                        var elemCols=16,elemRows=EPQ/16
                        var totalCols=elemCols*2,totalRows=elemRows*2
                        var cellW=gridW/totalCols,cellH=parent.height/totalRows
                        var gc2=Math.min(Math.floor(mouse.x/cellW),totalCols-1)
                        var gr2=Math.min(Math.floor(mouse.y/cellH),totalRows-1)
                        var q2=Math.floor(gr2/elemRows)*2+Math.floor(gc2/elemCols)
                        var e2=(gr2%elemRows)*elemCols+(gc2%elemCols)
                        var gid2=q2*EPQ+e2
                        root.openControlPanel(gid2, q2, e2)
                    }
                }
            }
        }
    }

    // ════════════════════════════════════════════════════════════
    // TOOLTIP
    // ════════════════════════════════════════════════════════════
    Rectangle {
        id: hoverTooltip
        visible: root.hovType > 0
        opacity: visible ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation{duration:120} }
        width:  root.hovType===1?240:root.hovType===2?265:275
        height: root.hovType===1?130:root.hovType===2?165:220
        x: Math.min(root.hovMouseX+18, root.width-width-10)
        y: Math.min(root.hovMouseY+18, root.height-height-10)
        Behavior on x { NumberAnimation{duration:70;easing.type:Easing.OutQuad} }
        Behavior on y { NumberAnimation{duration:70;easing.type:Easing.OutQuad} }
        radius: 10
        color: th.ttBg
        border { color: th.ttBorder; width: 1 }
        z: 600

        property int   ttQ:       root.hovQuad>=0?root.hovQuad:0
        property var   ttQStats:  root.model?root.model.quadrantStats(ttQ):[0,0,0,0]
        property int   ttQHlth:   root.model?root.model.quadrantHealth(ttQ):-1
        property int   ttQStatus: ttQStats[2]>0 ? 3 : ttQStats[1]>0 ? 2 : ttQStats[0]>0 ? 1 : 0
        property int   ttC:       root.hovCluster>=0?root.hovCluster:0
        property var   ttCStats:  root.model?root.model.clusterStats(ttQ,ttC):[0,0,0,0]
        property int   ttCHlth:   root.model?root.model.clusterHealth(ttQ,ttC):-1
        property int   ttCStatus: ttCStats[2]>0 ? 3 : ttCStats[1]>0 ? 2 : ttCStats[0]>0 ? 1 : 0
        property int   ttCBase:   root.model?(ttQ*root.model.elementsPerQuadrant+ttC*root.model.elementsPerCluster):0
        property int   ttGid:     root.hovGlobalId>=0?root.hovGlobalId:0
        property int   ttEQ:      root.model?Math.floor(ttGid/root.model.elementsPerQuadrant):0
        property int   ttELoc:    root.model?(ttGid%root.model.elementsPerQuadrant):0
        property int   ttEClus:   root.model?Math.floor(ttELoc/root.model.elementsPerCluster):0
        property int   ttEIdx:    root.model?(ttELoc%root.model.elementsPerCluster):0
        property color ttStCol: {
            var v = root.hovType===1 ? ttQStatus : root.hovType===2 ? ttCStatus : root.hovValue
            return root.statusColor(v)
        }

        Rectangle {
            anchors{top:parent.top;left:parent.left;right:parent.right}
            height:3; radius:parent.radius; color:hoverTooltip.ttStCol
        }

        Column {
            anchors { fill: parent; margins: 14; topMargin: 18 }
            spacing: 0

            // LOD1 – Quadrant
            Column {
                visible:root.hovType===1; width:parent.width; spacing:4
                Row {
                    spacing:8
                    Rectangle { width:9;height:9;radius:4.5;color:hoverTooltip.ttStCol;anchors.verticalCenter:parent.verticalCenter }
                    Text {
                        text: root.hovQuad>=0 ? "Q"+(root.hovQuad+1)+" — "+["NW","NE","SW","SE"][root.hovQuad] : ""
                        font { family:"Segoe UI"; pixelSize:14; bold:true }
                        color: th.ttText; anchors.verticalCenter:parent.verticalCenter
                    }
                }
                Rectangle{width:parent.width;height:1;color:Qt.rgba(1,1,1,0.12)}
                Text { text:"Health:  " + root.healthPct(hoverTooltip.ttQHlth); font{family:"Segoe UI";pixelSize:12;bold:true}; color:hoverTooltip.ttStCol }
                Text { text:"Status:  " + root.statusLabel(hoverTooltip.ttQStatus); font{family:"Segoe UI";pixelSize:11}; color:th.ttText }
                Text { property var s:hoverTooltip.ttQStats; text:"H:"+s[0]+"  W:"+s[1]+"  C:"+s[2]+"  U:"+s[3]; font{family:"Segoe UI";pixelSize:11}; color:th.ttText }
                Text { text:"Elements: "+(root.model?root.model.elementsPerQuadrant:0)+"  |  Clusters: "+(root.model?root.model.clustersPerQuadrant:0); font{family:"Segoe UI";pixelSize:10;italic:true}; color:Qt.rgba(1,1,1,0.4) }
                Text { text:"Click -> clusters  |  Double-click / expand button -> expand"; font{family:"Segoe UI";pixelSize:10;italic:true}; color:th.accent }
            }

            // LOD2 – Cluster
            Column {
                visible:root.hovType===2; width:parent.width; spacing:4
                Row {
                    spacing:8
                    Rectangle { width:9;height:9;radius:4.5;color:hoverTooltip.ttStCol;anchors.verticalCenter:parent.verticalCenter }
                    Text { text:"Cluster "+(hoverTooltip.ttC+1); font{family:"Segoe UI";pixelSize:14;bold:true}; color:th.ttText; anchors.verticalCenter:parent.verticalCenter }
                }
                Rectangle{width:parent.width;height:1;color:Qt.rgba(1,1,1,0.12)}
                Text { text:"Quadrant: Q"+(hoverTooltip.ttQ+1)+(root.hovQuad>=0?"  ("+["NW","NE","SW","SE"][root.hovQuad]+")":""); font{family:"Segoe UI";pixelSize:11}; color:th.ttText }
                Text { text:"Elements: E"+hoverTooltip.ttCBase+" - E"+(hoverTooltip.ttCBase+(root.model?root.model.elementsPerCluster-1:15)); font{family:"Segoe UI";pixelSize:11}; color:th.ttText }
                Text { text:"Health:   "+root.healthPct(hoverTooltip.ttCHlth); font{family:"Segoe UI";pixelSize:12;bold:true}; color:hoverTooltip.ttStCol }
                Text { text:"Status:   "+root.statusLabel(hoverTooltip.ttCStatus); font{family:"Segoe UI";pixelSize:11}; color:th.ttText }
                Text { property var s:hoverTooltip.ttCStats; text:"H:"+s[0]+"  W:"+s[1]+"  C:"+s[2]+"  U:"+s[3]; font{family:"Segoe UI";pixelSize:11}; color:th.ttText }
                Text { text:"Click -> see individual elements"; font{family:"Segoe UI";pixelSize:10;italic:true}; color:th.accent }
            }

            // LOD3 / Full – Element
            Column {
                visible:root.hovType===3; width:parent.width; spacing:0
                Row {
                    spacing:8
                    Rectangle { width:9;height:9;radius:4.5;color:hoverTooltip.ttStCol;anchors.verticalCenter:parent.verticalCenter }
                    Text { text:"Element  "+hoverTooltip.ttGid; font{family:"Segoe UI";pixelSize:14;bold:true}; color:th.ttText; anchors.verticalCenter:parent.verticalCenter }
                }
                Item { height:6 }
                Text { text:"Q"+(hoverTooltip.ttEQ+1)+"  ·  Cluster "+(hoverTooltip.ttEClus+1)+"  ·  Local #"+hoverTooltip.ttEIdx; font{family:"Segoe UI";pixelSize:10}; color:Qt.rgba(1,1,1,0.42) }
                Item { height:8 }
                Rectangle { width:parent.width;height:1;color:Qt.rgba(1,1,1,0.12) }
                Item { height:8 }

                property var det: root.model ? root.model.elementDetail(hoverTooltip.ttEQ, hoverTooltip.ttELoc) : {}

                Row { width:parent.width;height:22;spacing:0
                    Text { text:"Power"; font{family:"Segoe UI";pixelSize:11}; color:Qt.rgba(1,1,1,0.42); width:76 }
                    Text { text:root.fmtPwr(parent.parent.det.power||-999); font{family:"Segoe UI";pixelSize:11;bold:true}; color:root.healthColor(parent.parent.det.powerStatus||0) }
                }
                Text { text:"   threshold >= 43 dBm"; font{family:"Segoe UI";pixelSize:9;italic:true}; color:Qt.rgba(1,1,1,0.28) }
                Item { height:4 }
                Row { width:parent.width;height:22;spacing:0
                    Text { text:"Temp"; font{family:"Segoe UI";pixelSize:11}; color:Qt.rgba(1,1,1,0.42); width:76 }
                    Text { text:root.fmtTmp(parent.parent.det.temperature||-999); font{family:"Segoe UI";pixelSize:11;bold:true}; color:root.healthColor(parent.parent.det.tempStatus||0) }
                }
                Text { text:"   threshold <= 50 deg C"; font{family:"Segoe UI";pixelSize:9;italic:true}; color:Qt.rgba(1,1,1,0.28) }
                Item { height:4 }
                Row { width:parent.width;height:22;spacing:0
                    Text { text:"Current"; font{family:"Segoe UI";pixelSize:11}; color:Qt.rgba(1,1,1,0.42); width:76 }
                    Text { text:root.fmtCur(parent.parent.det.current||-999); font{family:"Segoe UI";pixelSize:11;bold:true}; color:root.healthColor(parent.parent.det.currStatus||0) }
                }
                Text { text:"   threshold <= 2.0 A"; font{family:"Segoe UI";pixelSize:9;italic:true}; color:Qt.rgba(1,1,1,0.28) }
                Item { height:10 }
                Rectangle {
                    height:26;radius:13;width:ttPill.implicitWidth+28
                    color:Qt.rgba(hoverTooltip.ttStCol.r,hoverTooltip.ttStCol.g,hoverTooltip.ttStCol.b,0.20)
                    border{color:hoverTooltip.ttStCol;width:1}
                    Row { anchors.centerIn:parent;spacing:7
                        Rectangle { width:8;height:8;radius:4;color:hoverTooltip.ttStCol;anchors.verticalCenter:parent.verticalCenter }
                        Text { id:ttPill;text:root.statusLabel(root.hovValue);font{family:"Segoe UI";pixelSize:11;bold:true};color:hoverTooltip.ttStCol;anchors.verticalCenter:parent.verticalCenter }
                    }
                }
            }
        }
    }

    // ════════════════════════════════════════════════════════════
    // CONTROL PANEL OVERLAY
    // ════════════════════════════════════════════════════════════
    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: root.cpVisible ? 0.45 : 0.0
        Behavior on opacity { NumberAnimation { duration: 200 } }
        z: 700
        MouseArea {
            anchors.fill: parent
            enabled: root.cpVisible
            onClicked: root.closeControlPanel()
        }
    }

    ControlPanel {
        id: ctrlPanel
        anchors.fill: parent
        z: 800

        dark:       root.darkTheme
        globalId:   root.cpGlobalId
        quadrant:   root.cpQuadrant
        localElem:  root.cpLocalElem
        cluster:    root.cpCluster
        localIdx:   root.cpLocalIdx
        cmdSender:  root.cmdSender
        model:      root.model

        visible: opacity > 0
        onCloseRequested: root.closeControlPanel()
    }

    onCpVisibleChanged: {
        if (cpVisible) ctrlPanel.show()
        else           ctrlPanel.hide()
    }

    // Global mouse tracker for tooltip positioning
    MouseArea {
        anchors.fill:parent; propagateComposedEvents:true; hoverEnabled:true
        onPositionChanged: { root.hovMouseX=mouse.x; root.hovMouseY=mouse.y; mouse.accepted=false }
    }

    // ════════════════════════════════════════════════════════════
    // FOOTER
    // ════════════════════════════════════════════════════════════
    Rectangle {
        id: footer
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 24
        color:th.header
        Rectangle {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 1; color: th.accent; opacity: 0.3
        }
        RowLayout {
            anchors { fill: parent; leftMargin: 14; rightMargin: 14 }
            Text {
                text: "LOD1: click=clusters  |  double-click=expand  |  LOD2: click=elements  |  All: full view  |  Analytics  |  ESC: back"
                font { family: "Segoe UI"; pixelSize: 10 }
                color: Qt.rgba(1,1,1,0.30)
            }
            Item { Layout.fillWidth: true }
            Text {
                text: "PKT: "+(root.udpRecv?root.udpRecv.packetsReceived:0)+"  ·  "+root.model.antennaName+"  ·  "+(root.darkTheme?"Dark":"Light")
                font { family: "Segoe UI"; pixelSize: 10 }
                color: Qt.rgba(1,1,1,0.30)
            }
        }
    }

    // ── ESC key ──────────────────────────────────────────────────
    Item {
        focus: true
        Keys.onEscapePressed: {
            if (root.cpVisible)            root.closeControlPanel()
            else if (root.fullView)        root.hideFullView()
            else if (root.lodLevel>1)      root.backTo(root.lodLevel-1)
            else if (root.expandedQuad>=0) { root.expandedQuad=-1; lod1Canvas.requestPaint() }
        }
    }

    // ── Data connections ─────────────────────────────────────────
    Connections {
        target: antenna0
        function onQuadrantUpdated(q) {
            if (root.currentAntenna!==0) return
            if (root.fullView)                              allCanvas.requestPaint()
            else if (root.lodLevel===1)                     lod1Canvas.requestPaint()
            else if (root.lodLevel===2&&root.selQuad===q)   lod2Canvas.requestPaint()
            else if (root.lodLevel===3&&root.selQuad===q)   lod3Canvas.requestPaint()
        }
        function onStatsChanged() {
            if (root.currentAntenna!==0) return
            if (root.fullView||root.lodLevel===1) lod1Canvas.requestPaint()
        }
    }
    Connections {
        target: antenna1
        function onQuadrantUpdated(q) {
            if (root.currentAntenna!==1) return
            if (root.fullView)                              allCanvas.requestPaint()
            else if (root.lodLevel===1)                     lod1Canvas.requestPaint()
            else if (root.lodLevel===2&&root.selQuad===q)   lod2Canvas.requestPaint()
            else if (root.lodLevel===3&&root.selQuad===q)   lod3Canvas.requestPaint()
        }
        function onStatsChanged() {
            if (root.currentAntenna!==1) return
            if (root.fullView||root.lodLevel===1) lod1Canvas.requestPaint()
        }
    }
}
