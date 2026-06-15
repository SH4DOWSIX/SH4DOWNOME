import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Drawer {
    id: root
    edge: Qt.BottomEdge
    width: parent.width
    height: root._editingRests ? 400 : Math.min(parent.height * 0.82, 540)
    Behavior on height { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
    dragMargin: 0
    property bool _editingRests: false
    onClosed: root._editingRests = false
    background: Rectangle { color: "#252525" }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // -- Title bar
        Rectangle {
            Layout.fillWidth: true
            height: 44
            color: "#1e1e1e"
            RowLayout {
                anchors { fill: parent; leftMargin: 14; rightMargin: 6 }
                ToolButton {
                    visible: root._editingRests
                    implicitWidth: 36; implicitHeight: 36
                    onClicked: root._editingRests = false
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
                    text: root._editingRests ? controller.stagedPatternName : "Choose Subdivision"
                    color: "white"; font.pixelSize: 14; font.bold: true
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Item {
                    width: 36; height: 36
                    Rectangle { width: 18; height: 2; color: "#aaa"; anchors.centerIn: parent; rotation: 45 }
                    Rectangle { width: 18; height: 2; color: "#aaa"; anchors.centerIn: parent; rotation: -45 }
                    MouseArea { anchors.fill: parent; onClicked: root.close() }
                }
            }
        }

        // -- Page 0: Picker grid (hidden when editing rests)
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            // picker page
            ColumnLayout {
                anchors.fill: parent
                opacity: root._editingRests ? 0 : 1
                visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.InOutQuad } }
            spacing: 0

            TabBar {
                id: tabBar
                Layout.fillWidth: true
                background: Rectangle { color: "#2a2a2a" }
                currentIndex: 0

                TabButton {
                    text: "Standard"
                    background: Rectangle { color: parent.checked ? "#3a3a3a" : "transparent" }
                    contentItem: Text { text: parent.text; color: parent.checked ? "white" : "#888"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                TabButton {
                    text: "Composite"
                    background: Rectangle { color: parent.checked ? "#3a3a3a" : "transparent" }
                    contentItem: Text { text: parent.text; color: parent.checked ? "white" : "#888"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                TabButton {
                    text: "Tuplets"
                    background: Rectangle { color: parent.checked ? "#3a3a3a" : "transparent" }
                    contentItem: Text { text: parent.text; color: parent.checked ? "white" : "#888"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                TabButton {
                    text: "Custom"
                    background: Rectangle { color: parent.checked ? "#3a3a3a" : "transparent" }
                    contentItem: Text { text: parent.text; color: parent.checked ? "white" : "#888"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }

            GridView {
                id: patternGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                leftMargin: 6; topMargin: 6; rightMargin: 6; bottomMargin: 6

                property int currentCat: tabBar.currentIndex

                cellWidth: Math.floor((width - 12) / 2)
                cellHeight: tabBar.currentIndex === 3 ? 108 : 88

                model: controller.pickerPatternCount(currentCat)

                onCurrentCatChanged: {
                    model = controller.pickerPatternCount(currentCat)
                    positionViewAtBeginning()
                }

                delegate: Item {
                    width: patternGrid.cellWidth
                    height: patternGrid.cellHeight

                    property bool isCurrent: controller.subdivisionRevision >= 0
                                             && controller.isCurrentPickerPattern(patternGrid.currentCat, index)

                    Rectangle {
                        anchors { fill: parent; margins: 4 }
                        radius: 6
                        color: isCurrent ? controller.accentColor.darker(1.3) : "#333"
                        border.color: isCurrent ? controller.accentColor : "#555"
                        border.width: isCurrent ? 2 : 1

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (patternGrid.currentCat === 3) {
                                    controller.applyPickerPattern(3, index)
                                    root.close()
                                } else {
                                    controller.preparePickerPattern(patternGrid.currentCat, index)
                                    root._editingRests = true
                                }
                            }
                        }

                        ColumnLayout {
                            anchors { fill: parent; margins: 5 }
                            spacing: 3

                            Image {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                source: "image://notes/picker_"
                                        + patternGrid.currentCat + "_"
                                        + index + "_"
                                        + controller.pickerRevision
                                sourceSize.width: 80
                                sourceSize.height: 80
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                            }

                            Text {
                                Layout.fillWidth: true
                                text: controller.pickerRevision >= 0
                                      ? controller.pickerPatternName(patternGrid.currentCat, index)
                                      : ""
                                color: isCurrent ? "white" : "#ccc"
                                font.pixelSize: 9
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }

                            RowLayout {
                                visible: patternGrid.currentCat === 3
                                Layout.fillWidth: true
                                spacing: 3
                                Button {
                                    Layout.fillWidth: true
                                    text: "Edit"
                                    font.pixelSize: 9; padding: 2
                                    background: Rectangle { color: "#555"; radius: 3 }
                                    contentItem: Text { text: parent.text; color: "white"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                    onClicked: controller.openEditCustomPattern(index)
                                }
                                Button {
                                    Layout.fillWidth: true
                                    text: "Del"
                                    font.pixelSize: 9; padding: 2
                                    background: Rectangle { color: controller.accentColor.darker(1.5); radius: 3 }
                                    contentItem: Text { text: parent.text; color: "white"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                    onClicked: controller.deleteCustomPattern(index)
                                }
                            }
                        }
                    }
                }

                Connections {
                    target: controller
                    function onPickerPatternsChanged() {
                        patternGrid.model = controller.pickerPatternCount(patternGrid.currentCat)
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: patternGrid.count === 0
                    text: tabBar.currentIndex === 3
                          ? "No custom patterns yet.\nTap 'New Pattern' below to create one."
                          : ""
                    color: "#777"; font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    width: patternGrid.width * 0.7
                }
            }

            // New Pattern button -- Custom tab only
            Rectangle {
                visible: tabBar.currentIndex === 3
                Layout.fillWidth: true
                height: 40
                color: "#1e1e1e"
                Button {
                    anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                    text: "New Pattern"
                    font.pixelSize: 12; padding: 6
                    background: Rectangle { color: controller.accentColor; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: controller.openNewCustomPattern()
                }
            }
        }

            // rest editor page
            ColumnLayout {
                anchors.fill: parent
                opacity: root._editingRests ? 1 : 0
                visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.InOutQuad } }
            spacing: 0

            // Live subdivision preview — same card style as the picker grid cells
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Item {
                    anchors.centerIn: parent
                    width: Math.floor((root.width - 12) / 2)
                    height: 88

                    Rectangle {
                        anchors { fill: parent; margins: 4 }
                        radius: 6
                        color: controller.accentColor.darker(1.3)
                        border.color: controller.accentColor
                        border.width: 2

                        ColumnLayout {
                            anchors { fill: parent; margins: 5 }
                            spacing: 3

                            Image {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                source: root._editingRests
                                        ? "image://notes/staged_" + controller.stagedRevision
                                        : ""
                                sourceSize.width: 80
                                sourceSize.height: 80
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                            }

                            Text {
                                Layout.fillWidth: true
                                text: controller.stagedPatternName
                                color: "white"
                                font.pixelSize: 9
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

            // Pulse toggle row
            Rectangle {
                Layout.fillWidth: true
                height: 62
                color: "#1e1e1e"
                Column {
                    anchors { fill: parent; leftMargin: 12; rightMargin: 12; topMargin: 6; bottomMargin: 6 }
                    spacing: 4
                    Text {
                        text: "Tap a note to silence it:"
                        color: "#888"; font.pixelSize: 10
                    }
                    Flickable {
                        width: parent.width
                        height: 34
                        contentWidth: pulseRow.implicitWidth
                        clip: true
                        interactive: pulseRow.implicitWidth > width
                        Row {
                            id: pulseRow
                            spacing: 4
                            Repeater {
                                model: root._editingRests ? controller.stagedPulseCount() : 0
                                delegate: Rectangle {
                                    property bool isRest: controller.stagedRevision >= 0
                                                          && controller.stagedPulseIsRest(index)
                                    width: 34; height: 34; radius: 3
                                    color: isRest ? "#2a1414" : "#2e2e2e"
                                    border.color: isRest ? "#7a3030" : "#555"
                                    Text {
                                        anchors.centerIn: parent
                                        text: isRest ? "\u2014" : String(index + 1)
                                        color: isRest ? "#cc6666" : "#aaa"
                                        font.pixelSize: 11
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: controller.toggleStagedPulseRest(index)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

            // Accent toggle row
            Rectangle {
                Layout.fillWidth: true
                height: 62
                color: "#1e1e1e"
                Column {
                    anchors { fill: parent; leftMargin: 12; rightMargin: 12; topMargin: 6; bottomMargin: 6 }
                    spacing: 4
                    Text {
                        text: "Tap a beat to toggle accent:"
                        color: "#888"; font.pixelSize: 10
                    }
                    Flickable {
                        width: parent.width
                        height: 34
                        contentWidth: accentRow.implicitWidth
                        clip: true
                        interactive: accentRow.implicitWidth > width
                        Row {
                            id: accentRow
                            spacing: 4
                            Repeater {
                                model: controller.accents.length
                                delegate: Rectangle {
                                    property bool accentOn: controller.accents[index] === true
                                    width: 34; height: 34; radius: 3
                                    color:        accentOn ? controller.accentColor : "#2e2e2e"
                                    border.color: accentOn ? controller.accentColor.darker(1.3) : "#555"
                                    Text {
                                        anchors.centerIn: parent
                                        text: String(index + 1)
                                        color: "white"
                                        font.pixelSize: 11
                                        font.bold: accentOn
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: controller.setAccent(index, !accentOn)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Footer: Cancel / OK
            Rectangle {
                Layout.fillWidth: true
                height: 60
                color: "#1e1e1e"
                RowLayout {
                    anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
                    spacing: 8
                    Button {
                        text: "Cancel"
                        Layout.fillWidth: true; Layout.preferredHeight: 40
                        onClicked: root.close()
                        background: Rectangle { color: "#444"; radius: 3 }
                        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    }
                    Button {
                        text: "OK"
                        Layout.fillWidth: true; Layout.preferredHeight: 40
                        onClicked: { controller.commitStagedPattern(); root.close() }
                        background: Rectangle { color: controller.accentColor; radius: 3 }
                        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    }
                }
            }
        }
    }
}
}