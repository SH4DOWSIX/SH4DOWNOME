import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Drawer {
    id: root
    edge: Qt.BottomEdge
    width: parent.width
    height: Math.min(parent.height * 0.92, 560)
    dragMargin: 0
    interactive: false   // prevent swipe-to-close conflicting with inner Flickable
    background: Rectangle { color: "#252525" }

    property var editor: controller.patternEditor
    property bool _awaitingNameInput: false
    property string _pendingName: ""

    // ── Duplicate name warning popup ──────────────────────────────────────
    Popup {
        id: dupNamePopup
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 300)
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 0
        background: Rectangle { color: "#1e1e1e"; radius: 6; border.color: "#555" }

        ColumnLayout {
            width: parent.width
            spacing: 0

            Text {
                Layout.fillWidth: true
                Layout.margins: 16
                text: "\"" + root._pendingName + "\" already exists.\nRename it or auto-append a number."
                color: "white"; font.pixelSize: 13; wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true; height: 44
                    color: "#252525"
                    radius: 0
                    Text { anchors.centerIn: parent; text: "Go Back"; color: "#aaa"; font.pixelSize: 14 }
                    MouseArea { anchors.fill: parent; onClicked: dupNamePopup.close() }
                }

                Rectangle { width: 1; height: 44; color: "#333" }

                Rectangle {
                    Layout.fillWidth: true; height: 44
                    color: "#252525"
                    Text { anchors.centerIn: parent; text: "Auto-rename"; color: controller.accentColor; font.pixelSize: 14; font.bold: true }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            dupNamePopup.close()
                            // Find a free name by appending incrementing numbers
                            var base = root._pendingName
                            var candidate = base
                            var n = 2
                            while (controller.customPatternNameExists(candidate))
                                candidate = base + " " + (n++)
                            editor.patternName = candidate
                            nameField.text = candidate
                            editor.stopPreview()
                            controller.commitCustomPattern()
                            root.close()
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: androidInput
        function onAccepted(text) {
            if (!root._awaitingNameInput) return
            root._awaitingNameInput = false
            editor.patternName = text
            nameField.text = text
        }
        function onCancelled() { root._awaitingNameInput = false }
    }

    onOpened: {
        nameField.text = editor.patternName
        presetCombo.currentIndex = 0
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
                Text {
                    text: "Custom Subdivision Pattern"
                    color: "white"; font.pixelSize: 13; font.bold: true
                    Layout.fillWidth: true
                }
                Item {
                    width: 36; height: 36
                    Rectangle { width: 18; height: 2; color: "#aaa"; anchors.centerIn: parent; rotation: 45 }
                    Rectangle { width: 18; height: 2; color: "#aaa"; anchors.centerIn: parent; rotation: -45 }
                    MouseArea { anchors.fill: parent; onClicked: { editor.stopPreview(); root.close() } }
                }
            }
        }

        // ── Scrollable content ───────────────────────────────────────────
        Flickable {
            id: contentFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: contentCol.implicitHeight
            clip: true
            interactive: contentHeight > height

            ColumnLayout {
                id: contentCol
                width: parent.width
                spacing: 6

                // ── Name ────────────────────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.topMargin: 8
                    spacing: 6
                    Text { text: "Name:"; color: "#ccc"; font.pixelSize: 11; Layout.alignment: Qt.AlignVCenter }
                    TextField {
                        id: nameField
                        Layout.fillWidth: true
                        visible: Qt.platform.os !== "android"
                        background: Rectangle { color: "#2a2a2a"; radius: 3; border.color: "#555" }
                        color: "white"; font.pixelSize: 12
                        onTextChanged: editor.patternName = text
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        visible: Qt.platform.os === "android"
                        color: "#2a2a2a"; radius: 3; border.color: "#555"
                        Text {
                            anchors { fill: parent; leftMargin: 8; rightMargin: 8 }
                            text: editor.patternName.length > 0 ? editor.patternName : "Tap to enter name"
                            color: editor.patternName.length > 0 ? "white" : "#888"
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                androidInput.showText("Pattern Name", editor.patternName)
                                root._awaitingNameInput = true
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#444"; Layout.leftMargin: 10; Layout.rightMargin: 10 }

                // ── Pulse strip ──────────────────────────────────────────
                Text {
                    text: editor.pulses.length > 0
                          ? "Pattern  (" + editor.pulses.length + " pulse" + (editor.pulses.length !== 1 ? "s" : "") + ")"
                          : "Pattern  — empty"
                    color: "#888"; font.pixelSize: 10
                    Layout.leftMargin: 10
                    Layout.topMargin: 2
                }

                ListView {
                    id: pulseList
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    height: 68
                    orientation: ListView.Horizontal
                    spacing: 3
                    clip: true
                    model: editor.pulses

                    onCountChanged: {
                        if (editor.selectedPulseIndex >= 0)
                            positionViewAtIndex(editor.selectedPulseIndex, ListView.Contain)
                    }

                    delegate: Rectangle {
                        width: 54; height: 68
                        radius: 3
                        color: editor.selectedPulseIndex === index ? controller.accentColor.darker(1.3) : "#2e2e2e"

                        Image {
                            anchors.centerIn: parent
                            width: 40; height: 52
                            source: "image://notes/editpulse_" + index + "_" + editor.pulseRevision
                            sourceSize: Qt.size(width, height)
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }

                        // Accent marker
                        Rectangle {
                            visible: modelData && modelData.accent === true
                            width: 6; height: 6; radius: 3
                            color: "#f0c020"
                            anchors { top: parent.top; right: parent.right; margins: 3 }
                        }

                        MouseArea { anchors.fill: parent; onClicked: editor.selectPulse(index) }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#444"; Layout.leftMargin: 10; Layout.rightMargin: 10 }

                // ── Note tile picker ─────────────────────────────────────
                Text { text: "Note type:"; color: "#888"; font.pixelSize: 10; Layout.leftMargin: 10 }

                Row {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 2

                    Repeater {
                        model: editor.tileCount()
                        delegate: Rectangle {
                            property int tileW: Math.floor((340 - (editor.tileCount() - 1) * 2) / editor.tileCount())
                            width: tileW; height: 56; radius: 3
                            color: editor.tileIndex === index ? controller.accentColor.darker(1.3) : "#333"
                            Image {
                                anchors.centerIn: parent
                                width: parent.width - 6; height: parent.height - 8
                                source: "image://notes/edittile_" + index
                                sourceSize: Qt.size(width, height)
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                            }
                            MouseArea { anchors.fill: parent; onClicked: editor.setTile(index) }
                        }
                    }
                }

                // ── Tuplet buttons ───────────────────────────────────────
                RowLayout {
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 3

                    Repeater {
                        model: [
                            { label: "None", n: 0 },
                            { label: "2",    n: 2 },
                            { label: "3",    n: 3 },
                            { label: "4",    n: 4 },
                            { label: "5",    n: 5 },
                            { label: "6",    n: 6 },
                            { label: "7",    n: 7 },
                            { label: "8",    n: 8 },
                            { label: "9",    n: 9 }
                        ]
                        delegate: Rectangle {
                            property bool shown: modelData.n === 0
                                || (editor.tupletVisibility[modelData.n] === true)
                            Layout.preferredWidth: shown ? (modelData.n === 0 ? 40 : 30) : 0
                            height: 28; radius: 3
                            visible: shown
                            color: editor.tuplet === modelData.n ? controller.accentColor.darker(1.3) : "#333"
                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                color: "white"; font.pixelSize: 10
                            }
                            MouseArea { anchors.fill: parent; onClicked: editor.setTuplet(modelData.n) }
                        }
                    }
                }

                // ── Type toggles ─────────────────────────────────────────
                RowLayout {
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 4

                    Rectangle {
                        width: 54; height: 28; radius: 3
                        color: !editor.isRest ? controller.accentColor.darker(1.3) : "#333"
                        Text { anchors.centerIn: parent; text: "Note"; color: "white"; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; onClicked: editor.setIsRest(false) }
                    }
                    Rectangle {
                        width: 54; height: 28; radius: 3
                        color: editor.isRest ? controller.accentColor.darker(1.3) : "#333"
                        Text { anchors.centerIn: parent; text: "Rest"; color: "white"; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; onClicked: editor.setIsRest(true) }
                    }
                    Item { width: 6; height: 1 }
                    Rectangle {
                        width: 60; height: 28; radius: 3
                        opacity: editor.canDot ? 1.0 : 0.3
                        color: editor.dottedActive ? controller.accentColor.darker(1.3) : "#333"
                        Text { anchors.centerIn: parent; text: "Dotted"; color: "white"; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; enabled: editor.canDot; onClicked: editor.setDotted(!editor.dottedActive) }
                    }
                    Rectangle {
                        width: 68; height: 28; radius: 3
                        color: editor.accent ? controller.accentColor.darker(1.3) : "#333"
                        Text { anchors.centerIn: parent; text: "Accented"; color: "white"; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; onClicked: editor.setAccent(!editor.accent) }
                    }
                }

                // ── Action row ───────────────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 4

                    Rectangle {
                        width: 56; height: 28; radius: 3
                        color: "#006000"
                        Text { anchors.centerIn: parent; text: "Add"; color: "white"; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; onClicked: editor.addPulse() }
                    }
                    Rectangle {
                        width: 64; height: 28; radius: 3
                        color: editor.selectedPulseIndex >= 0 ? "#3a1515" : "#2a2a2a"
                        Text { anchors.centerIn: parent; text: "Remove"; color: editor.selectedPulseIndex >= 0 ? "white" : "#666"; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; enabled: editor.selectedPulseIndex >= 0; onClicked: editor.removeSelected() }
                    }
                    Rectangle {
                        width: 50; height: 28; radius: 3
                        color: "#333"
                        Text { anchors.centerIn: parent; text: "Clear"; color: "white"; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; onClicked: editor.clearAll() }
                    }
                    Rectangle {
                        width: 46; height: 28; radius: 3
                        color: "#333"
                        Text { anchors.centerIn: parent; text: "Undo"; color: "white"; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; onClicked: editor.undo() }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: editor.totalDuration
                        color: editor.totalDurationColor
                        font.pixelSize: 9
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignRight
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#444"; Layout.leftMargin: 10; Layout.rightMargin: 10 }

                // ── Presets ──────────────────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 6
                    Text { text: "Insert preset:"; color: "#aaa"; font.pixelSize: 11; Layout.alignment: Qt.AlignVCenter }

                    ComboBox {
                        id: presetCombo
                        Layout.fillWidth: true
                        model: editor.presetNames
                        background: Rectangle { color: "#2a2a2a"; radius: 3; border.color: "#555" }
                        contentItem: Text {
                            text: presetCombo.displayText
                            color: "white"; font.pixelSize: 11
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 6
                            elide: Text.ElideRight
                        }
                        popup.background: Rectangle { color: "#2a2a2a"; border.color: "#555" }
                    }

                    Rectangle {
                        width: 56; height: 28; radius: 3
                        color: "#333"
                        Text { anchors.centerIn: parent; text: "Apply"; color: "white"; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; onClicked: editor.applyPreset(presetCombo.currentIndex) }
                    }
                }

                // ── Preview ──────────────────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 6

                    Rectangle {
                        Layout.fillWidth: true
                        height: 52; radius: 3
                        color: "#1a1a1a"; border.color: "#444"
                        clip: true

                        Image {
                            anchors.centerIn: parent
                            height: parent.height - 8
                            width: parent.width - 8
                            visible: editor.pulses.length > 0
                            source: editor.pulses.length > 0
                                    ? "image://notes/editpreview_" + editor.pulseRevision
                                    : ""
                            sourceSize: Qt.size(parent.width, parent.height)
                            fillMode: Image.Pad
                            smooth: true
                        }
                        Text {
                            anchors.centerIn: parent
                            visible: editor.pulses.length === 0
                            text: "(empty)"; color: "#555"; font.pixelSize: 11
                        }
                    }

                    Rectangle {
                        width: 58; height: 52; radius: 3
                        visible: editor.pulses.length > 0
                        color: editor.isPreviewing ? controller.accentColor.darker(1.3) : "#333"
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 1
                            Text { Layout.alignment: Qt.AlignHCenter; text: editor.isPreviewing ? "■" : "▶"; color: "white"; font.pixelSize: 14 }
                            Text { Layout.alignment: Qt.AlignHCenter; text: editor.isPreviewing ? "Stop" : "Preview"; color: "#ccc"; font.pixelSize: 9 }
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: editor.isPreviewing ? editor.stopPreview() : editor.startPreview()
                        }
                    }
                }

                Item { height: 4 }
            }
        }

        // ── Footer ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#1e1e1e"
            RowLayout {
                anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true; height: 40; radius: 3
                    color: "#444"
                    Text { anchors.centerIn: parent; text: "Cancel"; color: "white"; font.pixelSize: 15 }
                    MouseArea { anchors.fill: parent; onClicked: { editor.stopPreview(); root.close() } }
                }

                Rectangle {
                    Layout.fillWidth: true; height: 40; radius: 3
                    color: editor.canAccept ? controller.accentColor : "#252525"
                    Text {
                        anchors.centerIn: parent
                        text: "OK"
                        color: editor.canAccept ? "white" : "#555"
                        font.pixelSize: 15; font.bold: editor.canAccept
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: editor.canAccept
                        onClicked: {
                            Qt.inputMethod.commit()
                            editor.patternName = nameField.text
                            var name = editor.patternName.trim().length > 0
                                ? editor.patternName.trim()
                                : "Custom " + (controller.pickerPatternCount(3) + 1)
                            if (controller.customPatternNameExists(name)) {
                                root._pendingName = name
                                dupNamePopup.open()
                                return
                            }
                            editor.stopPreview()
                            controller.commitCustomPattern()
                            root.close()
                        }
                    }
                }
            }
        }
    }
}
