import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1 as Platform

Drawer {
    id: root
    edge: Qt.BottomEdge
    width: parent.width
    height: parent.height * 0.72
    dragMargin: 0
    background: Rectangle { color: "#252525" }

    // 0 = main menu, 1 = export select, 2 = import select
    property int    panel:          0
    property string statusText:     ""
    property bool   statusOk:       true
    property var    exportChecked:  ({})
    property var    importChecked:  ({})
    property string importFilePath: ""
    property var    importAvailable: []
    property int    importCustomCount: 0

    onOpened: { panel = 0 }
    signal goBack()

    // ── File dialogs ──────────────────────────────────────────────────────
    Platform.FileDialog {
        id: exportDialog
        title: "Save Backup"
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: ["Metronome backup (*.json)", "All files (*)"]
        defaultSuffix: "json"
        onAccepted: {
            var names = Object.keys(root.exportChecked).filter(function(k) { return root.exportChecked[k] })
            var ok = controller.exportPresetsToFile(names, file.toString())
            root.statusText = ok
                ? "Exported " + names.length + " preset(s) successfully."
                : "Export failed \u2014 could not write file."
            root.statusOk = ok
            root.panel = 0
        }
    }

    Platform.FileDialog {
        id: importDialog
        title: "Open Backup"
        fileMode: Platform.FileDialog.OpenFile
        nameFilters: ["Metronome backup (*.json)", "All files (*)"]
        onAccepted: {
            root.importFilePath = file.toString()
            var names = controller.presetsInFile(root.importFilePath)
            var custCount = controller.customPatternsInFile(root.importFilePath)
            if (names.length === 0 && custCount === 0) {
                root.statusText = "No presets or custom subdivisions found in the selected file."
                root.statusOk = false
                root.panel = 0
            } else {
                root.importAvailable = names
                root.importCustomCount = custCount
                var checked = {}
                for (var i = 0; i < names.length; i++) checked[names[i]] = true
                root.importChecked = checked
                root.panel = 2
            }
        }
    }

    // ── Helper: checked-count label ───────────────────────────────────────
    function checkedCount(map) {
        return Object.keys(map).filter(function(k) { return map[k] }).length
    }

    // ── Layout ────────────────────────────────────────────────────────────
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
                ToolButton {
                    visible: root.panel !== 0
                    implicitWidth: 36; implicitHeight: 36
                    onClicked: root.panel = 0
                    contentItem: Image {
                        source: "qrc:/resources/svg/section_down.svg"
                        sourceSize: Qt.size(512,512); width: 20; height: 20
                        fillMode: Image.PreserveAspectFit
                        smooth: true; mipmap: true
                        rotation: 90
                        anchors.centerIn: parent
                    }
                }
                ToolButton {
                    visible: root.panel === 0
                    implicitWidth: 36; implicitHeight: 36
                    onClicked: { root.close(); root.goBack() }
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
                    text: root.panel === 1 ? "Select Presets to Export"
                        : root.panel === 2 ? "Select Presets to Import"
                        : "Backup & Restore"
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

        // ── Content area ──────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ── Panel 0: Main ─────────────────────────────────────────────
            ColumnLayout {
                visible: root.panel === 0
                anchors { fill: parent; margins: 16 }
                spacing: 12

                Text {
                    visible: root.statusText.length > 0
                    text: root.statusText
                    color: root.statusOk ? "#66cc66" : "#ff5555"
                    font.pixelSize: 13; wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text: "Export or import presets and their custom subdivisions."
                    color: "#888"; font.pixelSize: 13; wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Item { Layout.fillHeight: true }

                Button {
                    text: "Export Presets\u2026"
                    Layout.fillWidth: true; Layout.preferredHeight: 46
                    onClicked: {
                        root.statusText = ""
                        var names = controller.presetNames
                        var checked = {}
                        for (var i = 0; i < names.length; i++) checked[names[i]] = true
                        root.exportChecked = checked
                        root.panel = 1
                    }
                    background: Rectangle { color: "#333"; radius: 4; border.color: "#555" }
                    contentItem: Text {
                        text: parent.text; color: "white"; font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    text: "Import Presets\u2026"
                    Layout.fillWidth: true; Layout.preferredHeight: 46
                    onClicked: { root.statusText = ""; importDialog.currentFile = ""; importDialog.open() }
                    background: Rectangle { color: "#333"; radius: 4; border.color: "#555" }
                    contentItem: Text {
                        text: parent.text; color: "white"; font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // ── Panel 1: Export select ────────────────────────────────────
            ColumnLayout {
                visible: root.panel === 1
                anchors.fill: parent
                spacing: 0

                // Select All / None toolbar
                Rectangle {
                    Layout.fillWidth: true; height: 36; color: "#1e1e1e"
                    RowLayout {
                        anchors { fill: parent; leftMargin: 14; rightMargin: 14 }
                        Text {
                            text: root.checkedCount(root.exportChecked) + " selected"
                            color: "#aaa"; font.pixelSize: 12; Layout.fillWidth: true
                        }
                        Text {
                            text: "All"; color: controller.accentColor; font.pixelSize: 12
                            MouseArea { anchors.fill: parent; onClicked: {
                                var c = {}; var n = controller.presetNames
                                for (var i = 0; i < n.length; i++) c[n[i]] = true
                                root.exportChecked = c
                            }}
                        }
                        Text { text: "  |  "; color: "#444"; font.pixelSize: 12 }
                        Text {
                            text: "None"; color: controller.accentColor; font.pixelSize: 12
                            MouseArea { anchors.fill: parent; onClicked: {
                                var c = {}; var n = controller.presetNames
                                for (var i = 0; i < n.length; i++) c[n[i]] = false
                                root.exportChecked = c
                            }}
                        }
                    }
                }

                // Preset list
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    model: controller.presetNames
                    clip: true
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Rectangle {
                        width: ListView.view.width; height: 40
                        color: index % 2 === 0 ? "#2e2e2e" : "#282828"
                        RowLayout {
                            anchors { fill: parent; leftMargin: 10; rightMargin: 14 }
                            spacing: 8
                            CheckBox {
                                checked: root.exportChecked[modelData] === true
                                onToggled: {
                                    var c = Object.assign({}, root.exportChecked)
                                    c[modelData] = checked
                                    root.exportChecked = c
                                }
                            }
                            Text {
                                text: modelData; color: "white"; font.pixelSize: 13
                                Layout.fillWidth: true; elide: Text.ElideRight
                            }
                        }
                        MouseArea {
                            anchors.fill: parent; z: -1
                            onClicked: {
                                var c = Object.assign({}, root.exportChecked)
                                c[modelData] = !c[modelData]
                                root.exportChecked = c
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#444" }

                // Footer buttons
                Rectangle {
                    Layout.fillWidth: true; height: 62; color: "#1e1e1e"
                    RowLayout {
                        anchors { fill: parent; leftMargin: 14; rightMargin: 14; topMargin: 10; bottomMargin: 10 }
                        spacing: 8
                        Button {
                            text: "Cancel"; Layout.fillWidth: true; Layout.fillHeight: true
                            onClicked: root.panel = 0
                            background: Rectangle { color: "#444"; radius: 3 }
                            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        Button {
                            text: "Export Selected"
                            Layout.fillWidth: true; Layout.fillHeight: true
                            enabled: root.checkedCount(root.exportChecked) > 0
                            onClicked: { exportDialog.currentFile = ""; exportDialog.open() }
                            background: Rectangle { color: enabled ? controller.accentColor : "#555"; radius: 3 }
                            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                }
            }

            // ── Panel 2: Import select ────────────────────────────────────
            ColumnLayout {
                visible: root.panel === 2
                anchors.fill: parent
                spacing: 0

                // Select All / None toolbar
                Rectangle {
                    Layout.fillWidth: true; height: 36; color: "#1e1e1e"
                    RowLayout {
                        anchors { fill: parent; leftMargin: 14; rightMargin: 14 }
                        Text {
                            text: root.checkedCount(root.importChecked) + " preset(s) selected"
                                + (root.importCustomCount > 0 ? " + " + root.importCustomCount + " custom subdivision(s)" : "")
                            color: "#aaa"; font.pixelSize: 12; Layout.fillWidth: true
                        }
                        Text {
                            text: "All"; color: controller.accentColor; font.pixelSize: 12
                            MouseArea { anchors.fill: parent; onClicked: {
                                var c = {}; var n = root.importAvailable
                                for (var i = 0; i < n.length; i++) c[n[i]] = true
                                root.importChecked = c
                            }}
                        }
                        Text { text: "  |  "; color: "#444"; font.pixelSize: 12 }
                        Text {
                            text: "None"; color: controller.accentColor; font.pixelSize: 12
                            MouseArea { anchors.fill: parent; onClicked: {
                                var c = {}; var n = root.importAvailable
                                for (var i = 0; i < n.length; i++) c[n[i]] = false
                                root.importChecked = c
                            }}
                        }
                    }
                }

                // Preset list from file
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    model: root.importAvailable
                    clip: true
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Rectangle {
                        width: ListView.view.width; height: 40
                        color: index % 2 === 0 ? "#2e2e2e" : "#282828"
                        RowLayout {
                            anchors { fill: parent; leftMargin: 10; rightMargin: 14 }
                            spacing: 8
                            CheckBox {
                                checked: root.importChecked[modelData] === true
                                onToggled: {
                                    var c = Object.assign({}, root.importChecked)
                                    c[modelData] = checked
                                    root.importChecked = c
                                }
                            }
                            Text {
                                text: modelData; color: "white"; font.pixelSize: 13
                                Layout.fillWidth: true; elide: Text.ElideRight
                            }
                            // Warn if name already exists
                            Text {
                                visible: controller.presetNameExists(modelData)
                                text: "overwrites"
                                color: "#f90"; font.pixelSize: 11
                            }
                        }
                        MouseArea {
                            anchors.fill: parent; z: -1
                            onClicked: {
                                var c = Object.assign({}, root.importChecked)
                                c[modelData] = !c[modelData]
                                root.importChecked = c
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#444" }

                // Footer buttons
                Rectangle {
                    Layout.fillWidth: true; height: 62; color: "#1e1e1e"
                    RowLayout {
                        anchors { fill: parent; leftMargin: 14; rightMargin: 14; topMargin: 10; bottomMargin: 10 }
                        spacing: 8
                        Button {
                            text: "Cancel"; Layout.fillWidth: true; Layout.fillHeight: true
                            onClicked: root.panel = 0
                            background: Rectangle { color: "#444"; radius: 3 }
                            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        Button {
                            text: "Import Selected"
                            Layout.fillWidth: true; Layout.fillHeight: true
                            enabled: root.checkedCount(root.importChecked) > 0 || root.importCustomCount > 0
                            onClicked: {
                                var names = Object.keys(root.importChecked).filter(function(k) { return root.importChecked[k] })
                                var ok = controller.importPresetsFromFile(names, root.importFilePath)
                                var presetCount = names.length
                                var custCount = root.importCustomCount
                                root.statusText = ok
                                    ? (presetCount > 0 && custCount > 0
                                        ? "Imported " + presetCount + " preset(s) and " + custCount + " custom subdivision(s)."
                                        : presetCount > 0
                                            ? "Imported " + presetCount + " preset(s) successfully."
                                            : "Imported " + custCount + " custom subdivision(s) successfully.")
                                    : "Import failed \u2014 could not read file."
                                root.statusOk = ok
                                root.panel = 0
                            }
                            background: Rectangle { color: enabled ? controller.accentColor : "#555"; radius: 3 }
                            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                }
            }
        }
    }
}
