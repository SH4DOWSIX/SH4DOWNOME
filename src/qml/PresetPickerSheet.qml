import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Drawer {
    id: root
    edge: Qt.BottomEdge
    width: parent.width
    height: Math.min(parent.height * 0.65, 520)
    dragMargin: 0
    background: Rectangle { color: "#252525" }

    property bool _awaitingRename: false

    Connections {
        target: androidInput
        function onAccepted(text) {
            if (!root._awaitingRename) return
            root._awaitingRename = false
            var n = text.trim()
            if (n.length > 0 && n !== controller.presetName)
                controller.renamePreset(controller.presetName, n)
        }
        function onCancelled() { root._awaitingRename = false }
    }

    // ── Rename popup ─────────────────────────────────────────────────────
    Rectangle {
        id: renameDialog
        width: 300; height: 160
        anchors.centerIn: parent
        z: 10; visible: false
        color: "#1e1e1e"; border.color: "#555"; border.width: 1; radius: 6

        ColumnLayout {
            anchors { fill: parent; margins: 16 }
            spacing: 12
            Text {
                text: "Rename " + controller.terminology
                color: "white"; font.pixelSize: 14; font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            TextField {
                id: renameField
                Layout.fillWidth: true
                font.pixelSize: 15; color: "white"
                selectionColor: controller.accentColor; selectedTextColor: "white"
                background: Rectangle { color: "#333"; radius: 4 }
                onAccepted: renameDialog.doAccept()
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Button {
                    text: "Cancel"; Layout.fillWidth: true
                    onClicked: renameDialog.close()
                    background: Rectangle { color: "#555"; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "OK"; Layout.fillWidth: true
                    onClicked: renameDialog.doAccept()
                    background: Rectangle { color: controller.accentColor; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        function open() { renameField.text = controller.presetName; renameField.selectAll(); visible = true; renameField.forceActiveFocus() }
        function close() { visible = false; Qt.inputMethod.hide() }
        function doAccept() {
            Qt.inputMethod.commit()
            var n = renameField.text.trim()
            if (n.length === 0 || n === controller.presetName) { close(); return }
            controller.renamePreset(controller.presetName, n)
            close()
        }
    }

    // ── Delete confirm popup ──────────────────────────────────────────────
    Rectangle {
        id: deleteDialog
        width: 300; height: 160
        anchors.centerIn: parent
        z: 10; visible: false
        color: "#1e1e1e"; border.color: "#555"; border.width: 1; radius: 6

        ColumnLayout {
            anchors { fill: parent; margins: 16 }
            spacing: 12
            Text {
                text: "Delete " + controller.terminology
                color: "white"; font.pixelSize: 14; font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                text: "Delete \"" + controller.presetName + "\"?\nThis cannot be undone."
                color: "#ccc"; font.pixelSize: 13; wrapMode: Text.Wrap
                Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Button {
                    text: "No"; Layout.fillWidth: true
                    onClicked: deleteDialog.close()
                    background: Rectangle { color: "#555"; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "Yes"; Layout.fillWidth: true
                    onClicked: { controller.deletePreset(controller.presetName); deleteDialog.close(); root.close() }
                    background: Rectangle { color: controller.accentColor.darker(1.5); radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        function open() { visible = true }
        function close() { visible = false }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Title bar ─────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 44
            color: "#1e1e1e"
            RowLayout {
                anchors { fill: parent; leftMargin: 14; rightMargin: 6 }
                Text {
                    text: "Select " + controller.terminology
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

        // ── Preset list ───────────────────────────────────────────────────
        ListView {
            id: presetList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: controller.presetNames
            clip: true
            ScrollBar.vertical: ScrollBar {}

            delegate: Rectangle {
                width: ListView.view.width
                height: 46
                color: (modelData === controller.presetName)
                       ? controller.accentColor.darker(1.4)
                       : (index % 2 === 0 ? "#2e2e2e" : "#282828")

                RowLayout {
                    anchors { fill: parent; leftMargin: 16; rightMargin: 12 }
                    Text {
                        text: modelData
                        color: "white"; font.pixelSize: 13
                        font.bold: (modelData === controller.presetName)
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Text {
                        visible: (modelData === controller.presetName)
                        text: "✓"
                        color: controller.accentColor; font.pixelSize: 14; font.bold: true
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: { controller.loadPreset(modelData); root.close() }
                }
            }
        }

        // ── Divider ───────────────────────────────────────────────────────
        Rectangle { Layout.fillWidth: true; height: 1; color: "#444" }

        // ── Bottom actions ────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 8

            Button {
                text: "Rename"
                Layout.fillWidth: true
                onClicked: {
                    if (Qt.platform.os === "android") {
                        androidInput.showText("Rename " + controller.terminology, controller.presetName)
                        root._awaitingRename = true
                    } else {
                        renameDialog.open()
                    }
                }
                background: Rectangle { color: "#333"; radius: 4 }
                contentItem: Text {
                    text: parent.text; color: "white"; font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
            }
            Button {
                text: "Delete"
                Layout.fillWidth: true
                onClicked: deleteDialog.open()
                background: Rectangle { color: controller.accentColor.darker(1.5); radius: 4 }
                contentItem: Text {
                    text: parent.text; color: "white"; font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
