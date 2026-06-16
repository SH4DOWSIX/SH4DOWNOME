import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Drawer {
    id: root
    edge: Qt.BottomEdge
    width: parent.width
    dragMargin: 0
    background: Rectangle { color: "#252525" }

    // Animate height transition between settings and colour picker panels
    height: showingColorPicker ? 380 : 470
    Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

    signal openBackupRequested()

    property var soundSets: ["Default", "Bongo", "Cowbell", "Digital", "Drum", "Hihat", "Metal", "Wooden", "Wooden 2", "Wooden 3"]

    property bool showingColorPicker: false

    // Pending settings values
    property string pendingSoundSet:    controller.soundSet
    property color  pendingAccentColor: controller.accentColor
    property bool   pendingAlwaysOnTop:    controller.alwaysOnTop
    property bool   pendingBeatWindowAuto: controller.beatWindowAuto
    property string pendingTerminology: controller.terminology

    function displaySoundSetName(name) {
        if (name === "Woodblock")
            return "Wooden"
        if (name === "Woodblock 2")
            return "Wooden 3"
        return name
    }

    // Color picker working values (HSV)
    property real cpHue: 0.0
    property real cpSat: 1.0
    property real cpVal: 1.0
    property color cpColor: Qt.hsva(cpHue, cpSat, cpVal, 1.0)

    readonly property var presetColors: [
        "#960000", "#c84040", "#00669c", "#007a3d",
        "#cc6600", "#6a0dad", "#b8860b", "#3a3a3a"
    ]

    // Reusable custom slider — MouseArea-based, immune to Drawer gesture stealing
    component ColorSlider: Item {
        id: cs
        property real value: 0.0   // 0..1, read/write
        property alias trackGradient: csTrack.gradient
        signal moved(real v)
        height: 30
        Layout.fillWidth: true

        Rectangle {
            id: csTrack
            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
            height: 8; radius: 4
        }
        Rectangle {
            id: csHandle
            width: 20; height: 20; radius: 10
            anchors.verticalCenter: parent.verticalCenter
            x: Math.max(0, Math.min(cs.width - width, cs.value * (cs.width - width)))
            color: "white"; border.color: "#333"
        }
        MouseArea {
            anchors.fill: parent
            preventStealing: true
            function updateFromX(mx) {
                var v = Math.max(0.0, Math.min(1.0, mx / width))
                cs.value = v
                cs.moved(v)
            }
            onPressed:         (mouse) => updateFromX(mouse.x)
            onPositionChanged: (mouse) => { if (pressed) updateFromX(mouse.x) }
        }
    }

    onOpened: {
        showingColorPicker = false
        pendingSoundSet    = displaySoundSetName(controller.soundSet)
        pendingAccentColor = controller.accentColor
        pendingAlwaysOnTop = controller.alwaysOnTop
        pendingBeatWindowAuto = controller.beatWindowAuto
        pendingTerminology = controller.terminology
        var i = soundSets.indexOf(pendingSoundSet)
        soundSetCombo.currentIndex = i >= 0 ? i : 0
        var ti = ["Piece", "Song", "Preset"].indexOf(pendingTerminology)
        terminologyCombo.currentIndex = ti >= 0 ? ti : 0
        topCheck.checked = pendingAlwaysOnTop
        beatWindowAutoCheck.checked = pendingBeatWindowAuto
    }

    function openColorPicker() {
        var c = pendingAccentColor
        cpHue = c.hsvHue < 0 ? 0 : c.hsvHue
        cpSat = c.hsvSaturation
        cpVal = c.hsvValue
        hueSlider.value = cpHue
        satSlider.value = cpSat
        briSlider.value = cpVal
        showingColorPicker = true
    }

    function applyPreset(hex) {
        var c = Qt.color(hex)
        cpHue = c.hsvHue < 0 ? 0 : c.hsvHue
        cpSat = c.hsvSaturation
        cpVal = c.hsvValue
        hueSlider.value = cpHue
        satSlider.value = cpSat
        briSlider.value = cpVal
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Title bar ────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 44
            color: "#1e1e1e"
            RowLayout {
                anchors { fill: parent; leftMargin: 14; rightMargin: 6 }
                ToolButton {
                    visible: root.showingColorPicker
                    implicitWidth: 36; implicitHeight: 36
                    onClicked: root.showingColorPicker = false
                    contentItem: Image {
                        source: "qrc:/resources/svg/section_down.svg"
                        sourceSize: Qt.size(512,512); width: 20; height: 20
                        fillMode: Image.PreserveAspectFit
                        smooth: true; mipmap: true
                        rotation: 90
                        anchors.centerIn: parent
                    }
                }
                Text {
                    text: root.showingColorPicker ? "Accent Color" : "Settings"
                    color: "white"; font.pixelSize: 14; font.bold: true
                    Layout.fillWidth: true
                }
                Item {
                    width: 36; height: 36
                    Rectangle { width: 18; height: 2; color: "#aaa"; anchors.centerIn: parent; rotation: 45 }
                    Rectangle { width: 18; height: 2; color: "#aaa"; anchors.centerIn: parent; rotation: -45 }
                    MouseArea { anchors.fill: parent; onClicked: root.close() }
                }
            }
        }

        // ── Settings panel ───────────────────────────────────────────────
        ColumnLayout {
            visible: !root.showingColorPicker
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Text { text: "Sound Set:"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                ComboBox {
                    id: soundSetCombo
                    model: root.soundSets
                    Layout.preferredWidth: 140
                    Layout.preferredHeight: 38
                    background: Rectangle { color: "#2a2a2a"; radius: 3; border.color: "#555" }
                    contentItem: Text {
                        text: soundSetCombo.displayText; color: "white"; font.pixelSize: 15
                        leftPadding: 6; verticalAlignment: Text.AlignVCenter
                    }
                    popup: Popup {
                        y: soundSetCombo.height + 2
                        width: soundSetCombo.width
                        padding: 0
                        background: Rectangle { color: "#1e1e1e"; border.color: "#555"; radius: 3 }
                        contentItem: ListView {
                            implicitHeight: contentHeight
                            model: soundSetCombo.delegateModel
                            clip: true
                            ScrollIndicator.vertical: ScrollIndicator {}
                        }
                    }
                    delegate: ItemDelegate {
                        width: soundSetCombo.width
                        highlighted: soundSetCombo.highlightedIndex === index
                        background: Rectangle {
                            color: highlighted ? controller.accentColor : (hovered ? "#2e2e2e" : "#1e1e1e")
                        }
                        contentItem: Text {
                            text: modelData
                            color: "white"
                            font.pixelSize: 15
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 6
                        }
                    }
                    onActivated: root.pendingSoundSet = currentText
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Text { text: "Terminology:"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                ComboBox {
                    id: terminologyCombo
                    model: ["Piece", "Song", "Preset"]
                    Layout.preferredWidth: 140
                    Layout.preferredHeight: 38
                    background: Rectangle { color: "#2a2a2a"; radius: 3; border.color: "#555" }
                    contentItem: Text {
                        text: terminologyCombo.displayText; color: "white"; font.pixelSize: 15
                        leftPadding: 6; verticalAlignment: Text.AlignVCenter
                    }
                    popup: Popup {
                        y: terminologyCombo.height + 2
                        width: terminologyCombo.width
                        padding: 0
                        background: Rectangle { color: "#1e1e1e"; border.color: "#555"; radius: 3 }
                        contentItem: ListView {
                            implicitHeight: contentHeight
                            model: terminologyCombo.delegateModel
                            clip: true
                            ScrollIndicator.vertical: ScrollIndicator {}
                        }
                    }
                    delegate: ItemDelegate {
                        width: terminologyCombo.width
                        highlighted: terminologyCombo.highlightedIndex === index
                        background: Rectangle {
                            color: highlighted ? controller.accentColor : (hovered ? "#2e2e2e" : "#1e1e1e")
                        }
                        contentItem: Text {
                            text: modelData
                            color: "white"
                            font.pixelSize: 15
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 6
                        }
                    }
                    onActivated: root.pendingTerminology = currentText
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Text { text: "Accent Color:"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                Rectangle {
                    width: 60; height: 36
                    color: root.pendingAccentColor
                    radius: 3; border.color: "#666"
                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: root.openColorPicker()
                    }
                }
            }

            CheckBox {
                id: beatWindowAutoCheck
                text: "Auto Beat Window on play"
                onCheckedChanged: root.pendingBeatWindowAuto = checked
                contentItem: Text {
                    text: parent.text; color: "white"; font.pixelSize: 15
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: parent.indicator.width + parent.spacing
                }
            }

            CheckBox {
                id: topCheck
                visible: Qt.platform.os !== "android"
                text: "Always on top"
                onCheckedChanged: root.pendingAlwaysOnTop = checked
                contentItem: Text {
                    text: parent.text; color: "white"; font.pixelSize: 15
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: parent.indicator.width + parent.spacing
                }
            }

            Item { Layout.fillHeight: true }

            Button {
                text: "Check for Updates"
                Layout.fillWidth: true; Layout.preferredHeight: 36
                onClicked: controller.checkForUpdates()
                background: Rectangle { color: "#333"; radius: 3; border.color: "#555" }
                contentItem: Text {
                    text: parent.text; color: "white"; font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                text: "Backup & Restore"
                Layout.fillWidth: true; Layout.preferredHeight: 36
                onClicked: root.openBackupRequested()
                background: Rectangle { color: "#333"; radius: 3; border.color: "#555" }
                contentItem: Text {
                    text: parent.text; color: "white"; font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 8
                Button {
                    text: "Cancel"; onClicked: root.close()
                    Layout.fillWidth: true; Layout.preferredHeight: 38
                    background: Rectangle { color: "#444"; radius: 3 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "OK"
                    Layout.fillWidth: true; Layout.preferredHeight: 38
                    onClicked: {
                        controller.applySettings(root.pendingSoundSet, root.pendingAccentColor,
                                                  root.pendingAlwaysOnTop, root.pendingBeatWindowAuto,
                                                  root.pendingTerminology)
                        root.close()
                    }
                    background: Rectangle { color: controller.accentColor; radius: 3 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        // ── Color picker panel ───────────────────────────────────────────
        ColumnLayout {
            visible: root.showingColorPicker
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 14; spacing: 10

            // Live preview
            Rectangle {
                Layout.fillWidth: true; height: 30; radius: 4
                color: root.cpColor; border.color: "#555"
            }

            // Preset swatches (1 row of 8)
            Grid {
                id: presetGrid
                Layout.fillWidth: true
                columns: 8; spacing: 6
                Repeater {
                    model: root.presetColors
                    delegate: Rectangle {
                        width:  (presetGrid.width - 7 * presetGrid.spacing) / 8
                        height: width; radius: 3; color: modelData
                        border.color: "white"
                        border.width: root.cpColor.toString().toLowerCase() === modelData.toLowerCase() ? 2 : 0
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: root.applyPreset(modelData)
                        }
                    }
                }
            }

            // Hue
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                Text { text: "Hue"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 26 }
                ColorSlider {
                    id: hueSlider
                    onMoved: (v) => root.cpHue = v
                    trackGradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.000; color: "#ff0000" }
                        GradientStop { position: 0.167; color: "#ffff00" }
                        GradientStop { position: 0.333; color: "#00ff00" }
                        GradientStop { position: 0.500; color: "#00ffff" }
                        GradientStop { position: 0.667; color: "#0000ff" }
                        GradientStop { position: 0.833; color: "#ff00ff" }
                        GradientStop { position: 1.000; color: "#ff0000" }
                    }
                }
            }

            // Saturation
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                Text { text: "Sat"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 26 }
                ColorSlider {
                    id: satSlider
                    onMoved: (v) => root.cpSat = v
                    trackGradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: Qt.hsva(root.cpHue, 0, root.cpVal, 1) }
                        GradientStop { position: 1; color: Qt.hsva(root.cpHue, 1, root.cpVal, 1) }
                    }
                }
            }

            // Brightness
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                Text { text: "Bri"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 26 }
                ColorSlider {
                    id: briSlider
                    onMoved: (v) => root.cpVal = v
                    trackGradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: "black" }
                        GradientStop { position: 1; color: Qt.hsva(root.cpHue, root.cpSat, 1, 1) }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true; spacing: 8
                Button {
                    Layout.fillWidth: true; Layout.preferredHeight: 40
                    text: "Cancel"; onClicked: root.showingColorPicker = false
                    background: Rectangle { color: "#444"; radius: 3 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    Layout.fillWidth: true; Layout.preferredHeight: 40
                    text: "OK"
                    onClicked: { root.pendingAccentColor = root.cpColor; root.showingColorPicker = false }
                    background: Rectangle { color: root.cpColor; radius: 3 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }
}
