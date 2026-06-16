import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import com.sh4downome 1.0

Item {
    id: beatWindow

    signal closeRequested()

    property int beatStyle: 0
    property bool _pickerOpen: false
    property real pulseKick: 0
    property real stageFlash: 0
    readonly property var styleNames: ["Classic", "Pulse", "Sweep", "LCD", "Stage", "Polyrhythm"]
    readonly property var availableStyleIndexes: controller.polyrhythmEnabled ? [0, 5] : [0, 1, 2, 3, 4]

    function storedStyleForCurrentMode() {
        return controller.polyrhythmEnabled
                ? controller.beatWindowPolyrhythmStyle
                : controller.beatWindowSubdivisionStyle
    }

    function applyStoredStyleForCurrentMode() {
        var stored = storedStyleForCurrentMode()
        beatStyle = availableStyleIndexes.indexOf(stored) >= 0
                ? stored
                : (controller.polyrhythmEnabled ? 5 : 0)
    }

    Component.onCompleted: applyStoredStyleForCurrentMode()
    onVisibleChanged: if (visible) applyStoredStyleForCurrentMode()

    onAvailableStyleIndexesChanged: {
        applyStoredStyleForCurrentMode()
    }

    function lcm(a, b) {
        a = Math.max(1, a)
        b = Math.max(1, b)
        var x = a
        var y = b
        while (y !== 0) {
            var t = y
            y = x % y
            x = t
        }
        return Math.max(1, (a / x) * b)
    }

    function polyHit(col, hits, columns) {
        hits = Math.max(1, hits)
        columns = Math.max(1, columns)
        for (var i = 0; i < hits; ++i) {
            if (Math.floor((i * columns) / hits) === col)
                return true
        }
        return false
    }

    MouseArea { anchors.fill: parent; onClicked: {} }

    Rectangle { anchors.fill: parent; color: "#000000" }

    Connections {
        target: controller
        function onBeatIndicatorChanged() {
            pulseAnim.restart()
            stageAnim.restart()
        }
    }

    SequentialAnimation {
        id: pulseAnim
        running: false
        NumberAnimation { target: beatWindow; property: "pulseKick"; from: 1.0; to: 0.0; duration: 260; easing.type: Easing.OutCubic }
    }

    SequentialAnimation {
        id: stageAnim
        running: false
        NumberAnimation { target: beatWindow; property: "stageFlash"; from: 0.42; to: 0.0; duration: 180; easing.type: Easing.OutCubic }
    }

    Button {
        id: styleBtn
        width: 36; height: 36
        anchors { top: parent.top; right: parent.right; topMargin: 6; rightMargin: 6 }
        z: 5
        onClicked: beatWindow._pickerOpen = !beatWindow._pickerOpen
        background: Rectangle {
            color: styleBtn.pressed ? Qt.rgba(1,1,1,0.18) : (styleBtn.hovered ? Qt.rgba(1,1,1,0.08) : "transparent")
            radius: 4
        }
        contentItem: Image {
            source: "qrc:/resources/svg/settings.svg"
            sourceSize: Qt.size(512,512); width: 20; height: 20
            fillMode: Image.PreserveAspectFit; anchors.centerIn: parent
            smooth: true; mipmap: true
        }
    }

    Item {
        id: bottomBar
        height: 65
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }

        RowLayout {
            anchors { fill: parent; leftMargin: 6; rightMargin: 6; topMargin: 6; bottomMargin: 6 }
            spacing: 4

            Button {
                Layout.fillWidth: true; Layout.fillHeight: true
                onClicked: controller.startStop()
                background: Rectangle {
                    color: controller.running ? "#6a0000" : "#006000"
                    radius: 4; border.color: "#222"
                }
                contentItem: Item {
                    Text {
                        anchors.centerIn: parent
                        text: controller.running ? "STOP" : "START"
                        color: "white"; font.pixelSize: 26; font.bold: true
                    }
                }
            }

            Button {
                id: closeBtn
                Layout.preferredWidth: 53; Layout.fillHeight: true
                onClicked: beatWindow.closeRequested()
                background: Rectangle {
                    color: closeBtn.pressed ? Qt.rgba(1,1,1,0.18) : (closeBtn.hovered ? Qt.rgba(1,1,1,0.08) : "transparent")
                    radius: 4
                }
                contentItem: Image {
                    source: "qrc:/resources/svg/section_down.svg"
                    sourceSize: Qt.size(512,512); width: 24; height: 24
                    fillMode: Image.PreserveAspectFit; anchors.centerIn: parent
                    smooth: true; mipmap: true
                }
            }
        }
    }

    Item {
        id: contentArea
        anchors {
            top: parent.top; topMargin: 8
            bottom: bottomBar.top; bottomMargin: 4
            left: parent.left; right: parent.right
        }

        ClassicStyle { anchors.fill: parent; visible: beatWindow.beatStyle === 0 }
        PulseStyle { anchors.fill: parent; visible: beatWindow.beatStyle === 1 }
        SweepStyle { anchors.fill: parent; visible: beatWindow.beatStyle === 2 }
        LCDStyle { anchors.fill: parent; visible: beatWindow.beatStyle === 3 }
        StageStyle { anchors.fill: parent; visible: beatWindow.beatStyle === 4 }
        PolyStyle { anchors.fill: parent; visible: beatWindow.beatStyle === 5 }
    }

    Rectangle {
        visible: beatWindow._pickerOpen
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.86)
        z: 20

        MouseArea { anchors.fill: parent; onClicked: beatWindow._pickerOpen = false }

        Column {
            anchors.centerIn: parent
            spacing: 10
            width: Math.min(parent.width - 40, 270)

            Text {
                width: parent.width
                text: "Beat Window Style"
                color: "white"; font.pixelSize: 16; font.bold: true
                horizontalAlignment: Text.AlignHCenter
                bottomPadding: 4
            }

            Repeater {
                model: beatWindow.availableStyleIndexes
                delegate: Button {
                    width: parent.width; height: 44
                    text: beatWindow.styleNames[modelData]
                    onClicked: {
                        beatWindow.beatStyle = modelData
                        controller.setBeatWindowStyleForMode(controller.polyrhythmEnabled, modelData)
                        beatWindow._pickerOpen = false
                    }
                    background: Rectangle {
                        color: beatWindow.beatStyle === modelData ? controller.accentColor : "#2a2a2a"
                        radius: 4
                        border.color: beatWindow.beatStyle === modelData ? Qt.lighter(controller.accentColor, 1.3) : "#555"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"; font.pixelSize: 15
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    component ClassicStyle: Item {
        BeatIndicatorItem {
            id: classicBeatIndicator
            width: parent.width; height: 120
            anchors.centerIn: parent
            backgroundColor: "#000000"
            circleBorderColor: "#1e1e1e"
            circleBorderWidth: 2
            beats: controller.biBeats
            subdivisions: controller.biSubdivisions
            currentBeat: controller.biCurrentBeat
            currentSub: controller.biCurrentSub
            mode: controller.biMode
            gridHighlight: controller.biGridHighlight
            polyMain: controller.biPolyMain
            polySecondary: controller.biPolySecondary
            accentColor: controller.accentColor
        }

        Image {
            width: parent.width; height: 80
            anchors { horizontalCenter: parent.horizontalCenter; bottom: classicBeatIndicator.top; bottomMargin: 12 }
            source: controller.polyrhythmEnabled ? "" : ("image://notes/current_" + controller.subdivisionRevision)
            sourceSize.width: width; sourceSize.height: height
            fillMode: Image.Pad
            verticalAlignment: Image.AlignVCenter
            horizontalAlignment: Image.AlignHCenter
            visible: !controller.polyrhythmEnabled
            smooth: true
        }

        PatternLane {
            id: classicPatternLane
            laneWidth: Math.min(300, parent.width * 0.7)
            anchors { horizontalCenter: parent.horizontalCenter; top: classicBeatIndicator.bottom; topMargin: 10 }
        }

        TimerTextBlock {
            blockWidth: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; top: classicPatternLane.bottom; topMargin: 8 }
        }
    }

    component PulseStyle: Item {
        Item {
            id: pulseFrame
            width: Math.min(parent.width, parent.height) * 0.56
            height: width
            anchors.centerIn: parent
        }

        Rectangle {
            id: outerPulse
            width: pulseFrame.width * (0.82 + beatWindow.pulseKick * 0.18)
            height: width
            radius: width / 2
            anchors.centerIn: pulseFrame
            color: "transparent"
            border.color: Qt.rgba(controller.accentColor.r, controller.accentColor.g, controller.accentColor.b, 0.45 + beatWindow.pulseKick * 0.45)
            border.width: 4 + beatWindow.pulseKick * 8
        }

        Rectangle {
            width: outerPulse.width * (0.56 + beatWindow.pulseKick * 0.18)
            height: width
            radius: width / 2
            anchors.centerIn: outerPulse
            color: Qt.rgba(controller.accentColor.r, controller.accentColor.g, controller.accentColor.b, 0.18 + beatWindow.pulseKick * 0.46)
            border.color: Qt.rgba(1, 1, 1, 0.16)
            border.width: 1
        }

        Text {
            anchors.centerIn: outerPulse
            text: Math.max(1, controller.biCurrentBeat + 1)
            color: "white"; font.pixelSize: Math.min(92, outerPulse.width * 0.38); font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        PatternLane {
            id: pulseSubs
            laneWidth: Math.min(260, parent.width * 0.62)
            anchors { horizontalCenter: parent.horizontalCenter; top: pulseFrame.bottom; topMargin: 12 }
        }

        TimerTextBlock {
            blockWidth: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; top: pulseSubs.bottom; topMargin: 10 }
        }
    }

    component SweepStyle: Item {
        readonly property int totalSteps: Math.max(1, controller.biBeats * controller.biSubdivisions)
        readonly property int activeStep: Math.max(0, controller.biCurrentBeat * controller.biSubdivisions + controller.biCurrentSub)

        Rectangle {
            id: sweepTrack
            width: Math.min(parent.width - 28, 620)
            height: 84
            radius: 4
            anchors.centerIn: parent
            color: "#101010"
            border.color: "#333"
            border.width: 1

            Row {
                anchors { fill: parent; margins: 8 }
                spacing: 3
                Repeater {
                    model: totalSteps
                    delegate: Rectangle {
                        width: Math.max(3, (parent.width - parent.spacing * (totalSteps - 1)) / totalSteps)
                        height: parent.height
                        radius: 2
                        color: index <= activeStep ? Qt.rgba(controller.accentColor.r, controller.accentColor.g, controller.accentColor.b, index === activeStep ? 1.0 : 0.34) : "#2b2b2b"
                    }
                }
            }

            Rectangle {
                width: 3
                height: parent.height + 18
                y: -9
                radius: 1
                color: "white"
                x: 8 + ((sweepTrack.width - 16) * (activeStep + 0.5) / totalSteps) - width / 2
            }
        }

        Text {
            width: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; bottom: sweepTrack.top; bottomMargin: 14 }
            text: "Beat " + Math.max(1, controller.biCurrentBeat + 1) + " / " + Math.max(1, controller.biBeats)
                  + "   Sub " + Math.max(1, controller.biCurrentSub + 1) + " / " + Math.max(1, controller.biSubdivisions)
            color: "#cccccc"; font.pixelSize: 18; font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        TimerTextBlock {
            id: sweepTimerBlock
            blockWidth: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; top: sweepTrack.bottom; topMargin: 14 }
        }

        PatternLane {
            laneWidth: Math.min(parent.width - 28, 620)
            anchors { horizontalCenter: parent.horizontalCenter; top: sweepTimerBlock.bottom; topMargin: 10 }
        }
    }

    component LCDStyle: Item {
        Rectangle {
            id: lcdPanel
            width: Math.min(parent.width - 40, 430)
            height: Math.min(parent.height - 34, 250)
            anchors.centerIn: parent
            radius: 6
            color: "#1d2420"
            border.color: "#6f7d6d"
            border.width: 2

            Rectangle {
                anchors { fill: parent; margins: 12 }
                radius: 4
                color: "#9aa88e"
                opacity: 0.92
            }

            Text {
                anchors { top: parent.top; left: parent.left; topMargin: 22; leftMargin: 24 }
                text: controller.tempo
                color: "#1c241d"; font.pixelSize: 58; font.bold: true
            }

            Text {
                anchors { top: parent.top; right: parent.right; topMargin: 30; rightMargin: 28 }
                text: "BPM"
                color: "#1c241d"; font.pixelSize: 18; font.bold: true
            }

            Text {
                anchors { left: parent.left; leftMargin: 26; top: parent.top; topMargin: 92 }
                text: "BEAT " + Math.max(1, controller.biCurrentBeat + 1) + " / " + Math.max(1, controller.biBeats)
                color: "#263025"; font.pixelSize: 18; font.bold: true
            }

            Text {
                anchors { right: parent.right; rightMargin: 28; top: parent.top; topMargin: 94 }
                text: "SUB " + Math.max(1, controller.biCurrentSub + 1) + "/" + Math.max(1, controller.biSubdivisions)
                color: "#263025"; font.pixelSize: 16; font.bold: true
            }

            Row {
                id: lcdTicks
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: 24; rightMargin: 24; bottomMargin: 34 }
                height: 62
                spacing: 4
                Repeater {
                    model: Math.max(1, controller.biBeats)
                    delegate: Rectangle {
                        width: Math.max(7, (lcdTicks.width - lcdTicks.spacing * (Math.max(1, controller.biBeats) - 1)) / Math.max(1, controller.biBeats))
                        height: parent.height
                        color: "transparent"
                        Rectangle {
                            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                            height: parent.height * (index === controller.biCurrentBeat ? 1 : 0.35)
                            color: index === controller.biCurrentBeat ? "#1d241e" : "#51604d"
                        }
                    }
                }
            }

            PatternLane {
                laneWidth: parent.width - 52
                anchors { left: parent.left; leftMargin: 26; bottom: parent.bottom; bottomMargin: 14 }
                darkMode: true
            }

            Text {
                anchors { right: parent.right; rightMargin: 26; top: parent.top; topMargin: 124 }
                visible: controller.timerEnabled
                text: controller.timerRemainingString
                color: "#1c241d"; font.pixelSize: 18; font.bold: true
            }
        }
    }

    component StageStyle: Item {
        Rectangle {
            anchors.fill: parent
            color: controller.accentColor
            opacity: beatWindow.stageFlash
        }

        Text {
            id: stageBeat
            anchors.centerIn: parent
            text: Math.max(1, controller.biCurrentBeat + 1)
            color: "white"; font.pixelSize: Math.min(parent.width * 0.36, parent.height * 0.58); font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            width: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; bottom: stageBeat.top; bottomMargin: 6 }
            text: controller.tempo + " BPM"
            color: "#d6d6d6"; font.pixelSize: 30; font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Column {
            width: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; top: stageBeat.bottom; topMargin: 8 }
            spacing: 6

            Text {
                width: parent.width
                visible: controller.timerEnabled
                text: controller.timerRemainingString
                color: "#cccccc"; font.pixelSize: 24; font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            PatternLane {
                laneWidth: Math.min(320, parent.width * 0.68)
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    component PolyStyle: Item {
        readonly property int columns: beatWindow.lcm(controller.biPolyMain, controller.biPolySecondary)

        Column {
            id: polyLanes
            width: Math.min(parent.width - 28, 640)
            anchors.centerIn: parent
            spacing: 8

            Repeater {
                model: [
                    { label: "SECONDARY", hits: controller.biPolySecondary, upper: true },
                    { label: "MAIN", hits: controller.biPolyMain, upper: false }
                ]
                delegate: Row {
                    id: laneRow
                    property int laneHits: modelData.hits
                    property bool laneUpper: modelData.upper

                    width: polyLanes.width
                    height: 48
                    spacing: 4

                    Repeater {
                        model: columns
                        delegate: Rectangle {
                            readonly property bool isHit: beatWindow.polyHit(index, laneRow.laneHits, columns)
                            readonly property bool isActive: index === controller.biGridHighlight && isHit
                            width: Math.max(4, (polyLanes.width - 4 * (columns - 1)) / columns)
                            height: parent.height
                            radius: 3
                            color: isActive ? "white"
                                  : isHit ? (laneRow.laneUpper ? Qt.darker(controller.accentColor, 1.15) : controller.accentColor)
                                  : "#242424"
                            border.color: "#111"
                            border.width: 1
                        }
                    }
                }
            }
        }

        Text {
            width: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; bottom: polyLanes.top; bottomMargin: 14 }
            text: controller.biPolyMain + " over " + controller.biPolySecondary
            color: "white"; font.pixelSize: 28; font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        TimerTextBlock {
            blockWidth: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; top: polyLanes.bottom; topMargin: 14 }
        }
    }

    component TimerTextBlock: Column {
        property real blockWidth: 0
        width: blockWidth
        spacing: 2

        Text {
            width: parent.width
            text: controller.tempo + " BPM"
            color: "white"; font.pixelSize: 48; font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            width: parent.width
            visible: controller.timerEnabled
            text: controller.timerRemainingString
            color: "#cccccc"; font.pixelSize: 20; font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }
    }

    component PatternLane: Item {
        property real laneWidth: 180
        property bool darkMode: false
        readonly property var pulses: controller.currentSubdivisionPulses.length > 0
                                      ? controller.currentSubdivisionPulses
                                      : [{ rest: false, accent: false }]
        readonly property int pulseCount: Math.max(1, pulses.length)
        readonly property bool compressed: pulseCount > 32
        readonly property bool compact: pulseCount > 12
        readonly property real gap: compact ? (width < pulseCount * 5 ? 1 : 2) : 5
        readonly property real cellWidth: Math.max(compact ? 1 : 2, (width - Math.max(0, pulseCount - 1) * gap) / pulseCount)
        width: laneWidth
        height: compressed ? 24 : (compact ? 18 : 22)

        onPulsesChanged: compressedRail.requestPaint()
        onPulseCountChanged: compressedRail.requestPaint()
        onWidthChanged: compressedRail.requestPaint()
        onDarkModeChanged: compressedRail.requestPaint()

        Connections {
            target: controller
            function onBeatIndicatorChanged() { compressedRail.requestPaint() }
            function onAccentColorChanged() { compressedRail.requestPaint() }
            function onSubdivisionChanged() { compressedRail.requestPaint() }
        }

        Repeater {
            model: compressed ? 0 : pulses
            delegate: Rectangle {
                readonly property bool activePulse: index === controller.biCurrentSub
                readonly property bool restPulse: modelData.rest === true
                readonly property bool accentPulse: modelData.accent === true
                width: cellWidth
                height: restPulse ? (compact ? 7 : 10) : (accentPulse ? parent.height : (compact ? 13 : 16))
                x: index * (cellWidth + gap)
                anchors.verticalCenter: parent.verticalCenter
                radius: compact ? 1 : 4
                color: activePulse ? "white"
                      : restPulse ? (darkMode ? "#6f7868" : "#303030")
                      : accentPulse ? Qt.lighter(controller.accentColor, 1.15)
                      : (darkMode ? "#263025" : "#555")
                opacity: restPulse && !activePulse ? 0.72 : 1.0
                border.color: activePulse ? Qt.lighter(controller.accentColor, 1.35)
                              : accentPulse ? controller.accentColor
                              : (darkMode ? "#1c241d" : "#222")
                border.width: 1
            }
        }

        Canvas {
            id: compressedRail
            anchors.fill: parent
            visible: compressed

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.globalAlpha = 1.0

                var centerY = height / 2
                ctx.fillStyle = darkMode ? "#1c241d" : "#202020"
                ctx.fillRect(0, centerY - 2, width, 4)

                var normalColor = darkMode ? "#263025" : "#555555"
                var restColor = darkMode ? "#6f7868" : "#303030"
                var accentColor = "rgba("
                        + Math.round(controller.accentColor.r * 255) + ","
                        + Math.round(controller.accentColor.g * 255) + ","
                        + Math.round(controller.accentColor.b * 255) + ",1)"
                var activeColor = "#ffffff"

                for (var i = 0; i < pulseCount; ++i) {
                    var pulse = pulses[i]
                    var x0 = Math.floor(i * width / pulseCount)
                    var x1 = Math.max(x0 + 1, Math.floor((i + 1) * width / pulseCount))
                    var isRest = pulse.rest === true
                    var isAccent = pulse.accent === true
                    var barHeight = isRest ? 7 : (isAccent ? height : 14)

                    ctx.fillStyle = isRest ? restColor : (isAccent ? accentColor : normalColor)
                    ctx.globalAlpha = isRest ? 0.72 : 1.0
                    ctx.fillRect(x0, centerY - barHeight / 2, Math.max(1, x1 - x0), barHeight)
                }

                var active = Math.max(0, Math.min(pulseCount - 1, controller.biCurrentSub))
                var activeX = Math.floor((active + 0.5) * width / pulseCount)
                ctx.globalAlpha = 1.0
                ctx.fillStyle = activeColor
                ctx.fillRect(Math.max(0, activeX - 1), 0, 3, height)
            }
        }
    }
}
