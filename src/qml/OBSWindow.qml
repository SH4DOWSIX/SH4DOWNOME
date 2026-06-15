import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import com.sh4downome 1.0

Window {
    id: obsWindow
    title: "OBS Beat Indicator"
    width: 420
    height: 220
    minimumWidth: 280
    minimumHeight: 140
    color: "#000000"
    flags: Qt.Window | Qt.WindowStaysOnTopHint

    // Pulse flash overlay
    Rectangle {
        anchors.fill: parent
        color: controller.accentColor
        opacity: controller.obsPulse ? 0.3 : 0.0
        Behavior on opacity { NumberAnimation { duration: 60 } }
        z: 10
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        // BPM display
        Text {
            Layout.fillWidth: true
            text: controller.tempo + " BPM"
            color: "white"
            font.pixelSize: 36
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        // Beat indicator
        BeatIndicatorItem {
            Layout.fillWidth: true
            Layout.fillHeight: true
            beats:         controller.biBeats
            subdivisions:  controller.biSubdivisions
            currentBeat:   controller.biCurrentBeat
            currentSub:    controller.biCurrentSub
            mode:          controller.biMode
            gridHighlight: controller.biGridHighlight
            polyMain:      controller.biPolyMain
            polySecondary: controller.biPolySecondary
            accentColor:   controller.accentColor
        }
    }
}
