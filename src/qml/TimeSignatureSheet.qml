import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Drawer {
    id: root
    edge: Qt.BottomEdge
    width: parent.width
    height: 340
    dragMargin: 0
    background: Rectangle { color: "#2b2b2b" }

    // Valid denominator values
    readonly property var denomValues: [2, 4, 8, 16]

    property int pendingNumerator:   4
    property int denomIndex:         1   // default = 4
    property int pendingDenominator: denomValues[denomIndex]

    onOpened: {
        pendingNumerator = controller.numerator
        var i = denomValues.indexOf(controller.denominator)
        denomIndex = i >= 0 ? i : 1
    }

    // Preset helper
    function applyPreset(num, den) {
        pendingNumerator = num
        var i = denomValues.indexOf(den)
        if (i >= 0) denomIndex = i
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
                    text: "Select Time Signature"; color: "white"
                    font.pixelSize: 14; font.bold: true; Layout.fillWidth: true
                }
                Item {
                    width: 36; height: 36
                    Rectangle { width: 18; height: 2; color: "#aaa"; anchors.centerIn: parent; rotation: 45 }
                    Rectangle { width: 18; height: 2; color: "#aaa"; anchors.centerIn: parent; rotation: -45 }
                    MouseArea { anchors.fill: parent; onClicked: root.close() }
                }
            }
        }

        // ── Body ─────────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            spacing: 6

            // Centred heading
            Text {
                Layout.fillWidth: true
                text: "Time Signature"
                color: "white"; font.pixelSize: 18; font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            // ── Beats per Bar ─────────────────────────────────────────
            Text {
                Layout.fillWidth: true
                text: "Beats per Bar"
                color: "white"; font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16; Layout.rightMargin: 16
                spacing: 0

                // Minus
                Button {
                    text: "-"
                    implicitWidth: 80; implicitHeight: 36
                    enabled: root.pendingNumerator > 1
                    onClicked: if (root.pendingNumerator > 1) root.pendingNumerator--
                    background: Rectangle { color: "#3a3a3a"; radius: 3; border.color: "#555" }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                // Big number
                Text {
                    Layout.fillWidth: true
                    text: root.pendingNumerator
                    color: "white"; font.pixelSize: 38; font.bold: true
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }

                // Plus
                Button {
                    text: "+"
                    implicitWidth: 80; implicitHeight: 36
                    enabled: root.pendingNumerator < 32
                    onClicked: if (root.pendingNumerator < 32) root.pendingNumerator++
                    background: Rectangle { color: "#3a3a3a"; radius: 3; border.color: "#555" }
                    contentItem: Text { text: parent.text; color: "#c84040"; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }

            // ── Note Value ────────────────────────────────────────────
            Text {
                Layout.fillWidth: true
                text: "Note Value"
                color: "white"; font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16; Layout.rightMargin: 16
                spacing: 0

                Button {
                    text: "-"
                    implicitWidth: 80; implicitHeight: 36
                    enabled: root.denomIndex > 0
                    onClicked: if (root.denomIndex > 0) root.denomIndex--
                    background: Rectangle { color: "#3a3a3a"; radius: 3; border.color: "#555" }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }

                Text {
                    Layout.fillWidth: true
                    text: root.denomValues[root.denomIndex]
                    color: "white"; font.pixelSize: 38; font.bold: true
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }

                Button {
                    text: "+"
                    implicitWidth: 80; implicitHeight: 36
                    enabled: root.denomIndex < root.denomValues.length - 1
                    onClicked: if (root.denomIndex < root.denomValues.length - 1) root.denomIndex++
                    background: Rectangle { color: "#3a3a3a"; radius: 3; border.color: "#555" }
                    contentItem: Text { text: parent.text; color: "#c84040"; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }

            // ── Preset shortcuts ──────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 10; Layout.rightMargin: 10
                spacing: 4

                Repeater {
                    model: [
                        { n: 2, d: 4 }, { n: 3, d: 4 }, { n: 4, d: 4 },
                        { n: 5, d: 4 }, { n: 3, d: 8 }, { n: 6, d: 8 }
                    ]
                    delegate: Button {
                        text: modelData.n + "/" + modelData.d
                        Layout.fillWidth: true
                        implicitHeight: 30
                        onClicked: root.applyPreset(modelData.n, modelData.d)
                        background: Rectangle {
                            color: (root.pendingNumerator === modelData.n && root.denomValues[root.denomIndex] === modelData.d)
                                   ? "#555" : "#3a3a3a"
                            radius: 3; border.color: "#666"
                        }
                        contentItem: Text {
                            text: parent.text; color: "white"; font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // ── OK button ─────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.rightMargin: 14
                spacing: 8
                Button {
                    text: "OK"
                    Layout.fillWidth: true; Layout.preferredHeight: 40
                    onClicked: {
                        controller.requestTimeSignature(root.pendingNumerator,
                                                        root.denomValues[root.denomIndex])
                        root.close()
                    }
                    background: Rectangle { color: controller.accentColor; radius: 3 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 15; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }
}
