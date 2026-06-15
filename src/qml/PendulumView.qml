import QtQuick 2.15

Item {
    id: root

    property int  _dir:      -1
    property real _maxAngle: 22.0
    property bool _dragging: false

    // ─ Layout geometry ─
    readonly property real cx:       width  / 2
    readonly property real bodyBot:  height - 8
    readonly property real baseH:    14
    readonly property real bodyH:    Math.min(height * 0.70, 340)
    readonly property real bodyTop:  bodyBot - bodyH
    readonly property real bodyW:    Math.min(width  * 0.68, 210)
    readonly property real bodyTopW: bodyW * 0.28
    readonly property real spireH:   Math.min(height * 0.11, 52)
    readonly property real spireTop: bodyTop - spireH
    readonly property real faceInset: 7

    // Face panel extents
    readonly property real faceTop:  bodyTop + 4
    readonly property real faceBot:  bodyBot - baseH - 4
    readonly property real faceH:    faceBot - faceTop

    // Scale strip (white, centered in face)
    // Strip ends at 78% of face height — leaves a solid black lower panel (like a real metronome)
    readonly property real scaleW:   16
    readonly property real scaleTop: faceTop + 4
    readonly property real scaleBot: faceTop + faceH * 0.78
    readonly property real scaleH:   scaleBot - scaleTop

    // Pivot: 72 % down the scale strip
    readonly property real pivotY:   scaleTop + scaleH * 1

    // Weight y inside assembly (assembly starts at rodTop, so offset scene coords)
    readonly property real tNorm:        Math.max(0, Math.min(1, (controller.tempo - 40) / 168.0))
    readonly property real weightLocalY: (scaleTop + 4 + tNorm * Math.max(0, pivotY - scaleTop - 30)) - (spireTop - 10)

    // ─ Body Canvas ─
    Canvas {
        id: bodyCanvas
        anchors.fill: parent
        onWidthChanged:  requestPaint()
        onHeightChanged: requestPaint()
        Component.onCompleted: Qt.callLater(requestPaint)

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var cx    = root.cx
            var bW    = root.bodyW
            var tW    = root.bodyTopW
            var bTop  = root.bodyTop
            var bBot  = root.bodyBot
            var bH    = root.bodyH
            var bsH   = root.baseH
            var spH   = root.spireH
            var spTop = root.spireTop
            var fi    = root.faceInset

            // Helper: half-width of body trapezoid at scene-y
            function bodyHalfW(y) {
                var frac = (y - bTop) / bH
                return tW / 2 + (bW / 2 - tW / 2) * frac
            }

            // ─ Wood gradient (light oak) ─
            var woodG = ctx.createLinearGradient(cx - bW / 2, 0, cx + bW / 2, 0)
            woodG.addColorStop(0,    "#7A4D1E")
            woodG.addColorStop(0.07, "#B07030")
            woodG.addColorStop(0.20, "#D49040")
            woodG.addColorStop(0.38, "#EDB050")
            woodG.addColorStop(0.50, "#F5C060")
            woodG.addColorStop(0.62, "#EDB050")
            woodG.addColorStop(0.80, "#D49040")
            woodG.addColorStop(0.93, "#B07030")
            woodG.addColorStop(1,    "#7A4D1E")

            // ─ Spire ─
            ctx.beginPath()
            ctx.moveTo(cx,          spTop + 6)   // peak (rounded below)
            ctx.lineTo(cx + tW / 2, bTop)
            ctx.lineTo(cx - tW / 2, bTop)
            ctx.closePath()
            ctx.fillStyle = woodG
            ctx.fill()
            ctx.strokeStyle = "#5A3010"; ctx.lineWidth = 1.5; ctx.stroke()

            // Spire cap circle
            ctx.beginPath()
            ctx.arc(cx, spTop + 5, 5, 0, Math.PI * 2)
            ctx.fillStyle = "#2A1005"
            ctx.fill()

            // ─ Main body (trapezoid) ─
            ctx.beginPath()
            ctx.moveTo(cx - tW / 2, bTop)
            ctx.lineTo(cx + tW / 2, bTop)
            ctx.lineTo(cx + bW / 2, bBot)
            ctx.lineTo(cx - bW / 2, bBot)
            ctx.closePath()
            ctx.fillStyle = woodG
            ctx.fill()
            ctx.strokeStyle = "#5A3010"; ctx.lineWidth = 1.5; ctx.stroke()

            // ─ Wood grain lines ─
            ctx.save()
            ctx.beginPath()
            ctx.moveTo(cx - tW / 2, bTop)
            ctx.lineTo(cx + tW / 2, bTop)
            ctx.lineTo(cx + bW / 2, bBot)
            ctx.lineTo(cx - bW / 2, bBot)
            ctx.closePath()
            ctx.clip()
            var grainStep = 10
            for (var gx = cx - bW / 2 - 5; gx < cx + bW / 2 + 5; gx += grainStep) {
                ctx.beginPath()
                ctx.moveTo(gx - 3, bTop - 4)
                ctx.bezierCurveTo(gx + 3, bTop + bH * 0.25,
                                  gx - 2, bTop + bH * 0.65,
                                  gx + 4, bBot + 4)
                ctx.strokeStyle = "rgba(80,35,5,0.10)"
                ctx.lineWidth = 1
                ctx.stroke()
            }
            ctx.restore()

            // ─ Edge highlights (left bright / right shadow) ─
            var hiL = ctx.createLinearGradient(cx - bW / 2, 0, cx - bW / 2 + 20, 0)
            hiL.addColorStop(0, "rgba(255,230,140,0.30)")
            hiL.addColorStop(1, "rgba(255,230,140,0.00)")
            ctx.beginPath()
            ctx.moveTo(cx - tW / 2,      bTop)
            ctx.lineTo(cx - tW / 2 + 14, bTop)
            ctx.lineTo(cx - bW / 2 + 20, bBot)
            ctx.lineTo(cx - bW / 2,      bBot)
            ctx.closePath()
            ctx.fillStyle = hiL; ctx.fill()

            var hiR = ctx.createLinearGradient(cx + bW / 2, 0, cx + bW / 2 - 20, 0)
            hiR.addColorStop(0, "rgba(40,15,0,0.28)")
            hiR.addColorStop(1, "rgba(40,15,0,0.00)")
            ctx.beginPath()
            ctx.moveTo(cx + tW / 2,      bTop)
            ctx.lineTo(cx + tW / 2 - 14, bTop)
            ctx.lineTo(cx + bW / 2 - 20, bBot)
            ctx.lineTo(cx + bW / 2,      bBot)
            ctx.closePath()
            ctx.fillStyle = hiR; ctx.fill()

            // ─ Dark base ─
            ctx.beginPath()
            ctx.moveTo(cx - bW / 2,     bBot - bsH)
            ctx.lineTo(cx + bW / 2,     bBot - bsH)
            ctx.lineTo(cx + bW / 2 + 7, bBot)
            ctx.lineTo(cx - bW / 2 - 7, bBot)
            ctx.closePath()
            ctx.fillStyle = "#151008"
            ctx.fill()
            ctx.strokeStyle = "#080503"; ctx.lineWidth = 1; ctx.stroke()

            // -- Wooden face panel (same gradient as body) ----------------
            var ft = root.faceTop, fb = root.faceBot
            ctx.beginPath()
            ctx.moveTo(cx - bodyHalfW(ft) + fi, ft)
            ctx.lineTo(cx + bodyHalfW(ft) - fi, ft)
            ctx.lineTo(cx + bodyHalfW(fb) - fi, fb)
            ctx.lineTo(cx - bodyHalfW(fb) + fi, fb)
            ctx.closePath()
            ctx.fillStyle = woodG
            ctx.fill()
            ctx.strokeStyle = "#5A3010"; ctx.lineWidth = 0.8; ctx.stroke()
            var scT  = root.scaleTop
            var scB  = root.scaleBot
            var scH  = root.scaleH
            var scHW = root.scaleW / 2
            ctx.beginPath()
            ctx.rect(cx - scHW, scT, root.scaleW, scB - scT)
            ctx.fillStyle = "#EDE5D5"
            ctx.fill()
            ctx.strokeStyle = "#BBA888"; ctx.lineWidth = 0.5; ctx.stroke()

            // ─ Scale tick marks & numbers ─
            var marks = [40,42,44,46,48,50,52,54,56,58,60,63,66,69,72,76,
                         80,84,88,92,96,100,104,108,112,116,120,126,132,
                         138,144,152,160,168,176,184,192,200,208]
            var majorSet = { 40:1, 60:1, 80:1, 100:1, 120:1, 140:1, 160:1, 180:1, 200:1, 208:1 }

            for (var i = 0; i < marks.length; i++) {
                var bpm   = marks[i]
                var n     = (bpm - 40) / 168.0
                var ty    = scT + n * scH
                var major = majorSet[bpm] === 1
                var tLen  = major ? 7 : 4

                ctx.beginPath()
                ctx.moveTo(cx - scHW,        ty)
                ctx.lineTo(cx - scHW - tLen, ty)
                ctx.strokeStyle = major ? "#555" : "#999"
                ctx.lineWidth   = major ? 0.8   : 0.5
                ctx.stroke()

                ctx.beginPath()
                ctx.moveTo(cx + scHW,        ty)
                ctx.lineTo(cx + scHW + tLen, ty)
                ctx.stroke()

                if (major) {
                    ctx.fillStyle  = "#444"
                    ctx.font       = "bold 6px sans-serif"
                    ctx.textAlign  = "right"
                    ctx.textBaseline = "middle"
                    ctx.fillText(String(bpm), cx - scHW - 9, ty)
                    ctx.textAlign  = "left"
                    ctx.fillText(String(bpm), cx + scHW + 9, ty)
                }
            }
        }
    }

    // ─ Pendulum assembly ─
    Item {
        id: assembly
        // Start just above the spire tip — rod doesn't fill the whole screen
        readonly property real rodTop: root.spireTop - 10
        width: 40
        height: root.pivotY - rodTop
        x: root.cx - width / 2
        y: rodTop
        transformOrigin: Item.Bottom   // rotates about pivot point

        // Rod (thin metallic grey)
        Rectangle {
            width: 4
            height: parent.height + 14   // extends slightly past pivot
            x: (parent.width - width) / 2
            y: 0
            radius: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#353535" }
                GradientStop { position: 0.3; color: "#787878" }
                GradientStop { position: 0.7; color: "#686868" }
                GradientStop { position: 1.0; color: "#303030" }
            }
        }

        // Sliding weight (draggable)
        Rectangle {
            id: weightRect
            property real _dragY: 0
            width: 34; height: 20; radius: 2
            x: (parent.width - width) / 2
            y: root._dragging ? _dragY : root.weightLocalY

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: root._dragging ? "#888" : "#666" }
                GradientStop { position: 0.3; color: root._dragging ? "#DDD" : "#B4B4B4" }
                GradientStop { position: 0.7; color: root._dragging ? "#D4D4D4" : "#AAAAAA" }
                GradientStop { position: 1.0; color: root._dragging ? "#777" : "#555" }
            }
            border.color: "#3A3A3A"; border.width: 1

            Behavior on y { enabled: !root._dragging; NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }

            // Screw details
            Rectangle {
                width: 5; height: 5; radius: 2.5; color: "#3C3C3C"
                anchors { left: parent.left; leftMargin: 5; verticalCenter: parent.verticalCenter }
            }
            Rectangle {
                width: 5; height: 5; radius: 2.5; color: "#3C3C3C"
                anchors { right: parent.right; rightMargin: 5; verticalCenter: parent.verticalCenter }
            }

            MouseArea {
                anchors { fill: parent; margins: -8 }
                cursorShape: Qt.SizeVerCursor
                property real _pressY:       0
                property real _pressWeightY: 0
                onPressed: function(mouse) {
                    root._dragging    = true
                    weightRect._dragY = weightRect.y
                    _pressY           = mouse.y
                    _pressWeightY     = weightRect.y
                }
                onPositionChanged: function(mouse) {
                    if (!pressed) return
                    var rodTop = root.spireTop - 10
                    var minY  = (root.scaleTop + 4) - rodTop
                    var maxY  = (root.pivotY - 30) - rodTop
                    var newY  = Math.max(minY, Math.min(maxY,
                                    _pressWeightY + (mouse.y - _pressY)))
                    weightRect._dragY = newY
                    var norm  = (newY - minY) / Math.max(1, maxY - minY)
                    controller.tempo = Math.max(40, Math.min(208, Math.round(40 + norm * 168)))
                }
                onReleased: root._dragging = false
            }
        }
    }

    // -- Lower wooden overlay: hides rod below scale strip ---------------
    Canvas {
        id: lowerFaceCanvas
        anchors.fill: parent
        onWidthChanged:  requestPaint()
        onHeightChanged: requestPaint()
        Connections {
            target: root
            function onScaleBotChanged() { lowerFaceCanvas.requestPaint() }
            function onFaceBotChanged()  { lowerFaceCanvas.requestPaint() }
        }
        Component.onCompleted: Qt.callLater(requestPaint)
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            var cx   = root.cx
            var bW   = root.bodyW
            var tW   = root.bodyTopW
            var bTop = root.bodyTop
            var bH   = root.bodyH
            var fi   = root.faceInset
            var scB  = root.scaleBot
            var fB   = root.faceBot
            function bodyHalfW(y) {
                var frac = (y - bTop) / bH
                return tW / 2 + (bW / 2 - tW / 2) * frac
            }
            var woodG = ctx.createLinearGradient(cx - bW / 2, 0, cx + bW / 2, 0)
            woodG.addColorStop(0,    "#7A4D1E")
            woodG.addColorStop(0.07, "#B07030")
            woodG.addColorStop(0.20, "#D49040")
            woodG.addColorStop(0.38, "#EDB050")
            woodG.addColorStop(0.50, "#F5C060")
            woodG.addColorStop(0.62, "#EDB050")
            woodG.addColorStop(0.80, "#D49040")
            woodG.addColorStop(0.93, "#B07030")
            woodG.addColorStop(1,    "#7A4D1E")
            ctx.beginPath()
            ctx.moveTo(cx - bodyHalfW((scB + 6)) + fi, (scB + 6))
            ctx.lineTo(cx + bodyHalfW((scB + 6)) - fi, (scB + 6))
            ctx.lineTo(cx + bodyHalfW(fB)  - fi, fB)
            ctx.lineTo(cx - bodyHalfW(fB)  + fi, fB)
            ctx.closePath()
            ctx.fillStyle = woodG
            ctx.fill()
            ctx.strokeStyle = "#5A3010"; ctx.lineWidth = 0.8; ctx.stroke()
            ctx.save()
            ctx.beginPath()
            ctx.moveTo(cx - bodyHalfW((scB + 6)) + fi, (scB + 6))
            ctx.lineTo(cx + bodyHalfW((scB + 6)) - fi, (scB + 6))
            ctx.lineTo(cx + bodyHalfW(fB)  - fi, fB)
            ctx.lineTo(cx - bodyHalfW(fB)  + fi, fB)
            ctx.closePath()
            ctx.clip()
            var panH = fB - scB
            for (var gx = cx - bW / 2; gx < cx + bW / 2; gx += 10) {
                ctx.beginPath()
                ctx.moveTo(gx - 2, scB)
                ctx.bezierCurveTo(gx + 2, scB + panH * 0.4, gx - 1, scB + panH * 0.7, gx + 3, fB)
                ctx.strokeStyle = "rgba(80,35,5,0.10)"
                ctx.lineWidth = 1
                ctx.stroke()
            }
            ctx.restore()
        }
    }

    // ─ BPM label (on body, above base) ─
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.faceBot - 20
        text: controller.tempo + " BPM"
        color: Qt.rgba(1, 1, 1, 0.75)
        font.pixelSize: 12; font.bold: true
        style: Text.Outline; styleColor: "#000"
    }

    // ─ Animations ─
    NumberAnimation {
        id: swingAnim
        target: assembly; property: "rotation"
        easing.type: Easing.InOutSine
        duration: controller.running
                  ? Math.max(100, Math.round(60000 / controller.tempo))
                  : 400
    }

    NumberAnimation {
        id: returnAnim
        target: assembly; property: "rotation"
        to: 0; easing.type: Easing.OutCubic; duration: 600
    }

    // ─ Beat sync ─
    Connections {
        target: controller
        function onRunningChanged() {
            if (controller.running) {
                root._dir = -1
            } else {
                swingAnim.stop()
                returnAnim.start()
            }
        }
        function onBiCurrentBeatChanged() {
            if (!controller.running) return
            root._dir = -root._dir
            swingAnim.to = root._dir * root._maxAngle
            swingAnim.restart()
        }
    }

    onVisibleChanged: {
        if (!visible) {
            swingAnim.stop()
            assembly.rotation = 0
        }
    }
}

