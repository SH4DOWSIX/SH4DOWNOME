import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Drawer {
    id: root
    edge: Qt.BottomEdge
    width: parent.width
    height: 360
    dragMargin: 0
    background: Rectangle { color: "#252525" }

    property int _primaryVal:   2
    property int _secondaryVal: 2
    property bool _perBeatVal: true
    property var _pendingCallback: null

    onOpened: {
        _primaryVal   = controller.polyPrimaryBeats
        _secondaryVal = controller.polySecondaryBeats
        _perBeatVal   = controller.polyrhythmPerBeat
    }

    Connections {
        target: androidInput
        function onAccepted(text) {
            if (!root._pendingCallback) return
            var v = parseInt(text)
            if (!isNaN(v)) root._pendingCallback(v)
            root._pendingCallback = null
        }
        function onCancelled() { root._pendingCallback = null }
    }

    function _openNumInput(label, val, cb) {
        if (Qt.platform.os === "android") {
            root._pendingCallback = cb
            androidInput.showNumber(label, val)
        } else {
            numPopup._label    = label
            numPopup._callback = cb
            numPopupField.text = String(val)
            numPopupField.selectAll()
            numPopup.visible   = true
            numPopupFocusTimer.restart()
        }
    }

    // ── Local number input popup ──────────────────────────────────────────
    Rectangle {
        id: numPopup
        anchors.centerIn: parent
        width: 260; height: 150
        z: 10; visible: false
        color: "#1e1e1e"; border.color: "#555"; border.width: 1; radius: 6

        property string _label: ""
        property var    _callback: null

        // eat all clicks so nothing beneath fires
        MouseArea { anchors.fill: parent }

        ColumnLayout {
            anchors { fill: parent; margins: 14 }
            spacing: 10
            Text {
                text: numPopup._label
                color: "white"; font.pixelSize: 13; font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            TextField {
                id: numPopupField
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhDigitsOnly
                horizontalAlignment: TextInput.AlignHCenter
                font.pixelSize: 20; color: "white"
                selectionColor: controller.accentColor; selectedTextColor: "white"
                background: Rectangle { color: "#333"; radius: 4 }
                validator: IntValidator { bottom: 2; top: 16 }
                onAccepted: numPopup.commit()
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                Button {
                    text: "Cancel"; Layout.fillWidth: true; Layout.preferredHeight: 34
                    onClicked: numPopup.visible = false
                    background: Rectangle { color: "#555"; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "OK"; Layout.fillWidth: true; Layout.preferredHeight: 34
                    onClicked: numPopup.commit()
                    background: Rectangle { color: controller.accentColor; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        function commit() {
            var v = parseInt(numPopupField.text)
            if (!isNaN(v) && _callback) _callback(v)
            visible = false
        }
    }
    Timer {
        id: numPopupFocusTimer; interval: 0
        onTriggered: numPopupField.forceActiveFocus(Qt.MouseFocusReason)
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
                Text { text: "Polyrhythm"; color: "white"; font.pixelSize: 14; font.bold: true; Layout.fillWidth: true }
                Item {
                    width: 36; height: 36
                    Rectangle { width: 18; height: 2; color: "#aaa"; anchors.centerIn: parent; rotation: 45 }
                    Rectangle { width: 18; height: 2; color: "#aaa"; anchors.centerIn: parent; rotation: -45 }
                    MouseArea { anchors.fill: parent; onClicked: root.close() }
                }
            }
        }

        // ── Form ─────────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 14
            spacing: 12

            Text {
                text: "Set the polyrhythm ratio and how it fits the meter."
                color: "#ccc"; font.pixelSize: 11; wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                Text { text: "Main beats:"; color: "white"; font.pixelSize: 12; Layout.fillWidth: true }
                Rectangle {
                    Layout.preferredWidth: 90; Layout.preferredHeight: 36
                    color: "#2a2a2a"; border.color: "#555"; radius: 3
                    Text {
                        anchors.centerIn: parent
                        text: root._primaryVal
                        color: "white"; font.pixelSize: 14
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root._openNumInput("Main beats", root._primaryVal, function(v) { root._primaryVal = v })
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Text { text: "Secondary beats:"; color: "white"; font.pixelSize: 12; Layout.fillWidth: true }
                Rectangle {
                    Layout.preferredWidth: 90; Layout.preferredHeight: 36
                    color: "#2a2a2a"; border.color: "#555"; radius: 3
                    Text {
                        anchors.centerIn: parent
                        text: root._secondaryVal
                        color: "white"; font.pixelSize: 14
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root._openNumInput("Secondary beats", root._secondaryVal, function(v) { root._secondaryVal = v })
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Text { text: "Scope:"; color: "white"; font.pixelSize: 12; Layout.fillWidth: true }

                Button {
                    text: "Per beat"
                    Layout.preferredWidth: 86; Layout.preferredHeight: 32
                    onClicked: root._perBeatVal = true
                    background: Rectangle {
                        color: root._perBeatVal ? controller.accentColor : "#555"
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text; color: "white"; font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    text: "Per bar"
                    Layout.preferredWidth: 86; Layout.preferredHeight: 32
                    onClicked: root._perBeatVal = false
                    background: Rectangle {
                        color: !root._perBeatVal ? controller.accentColor : "#555"
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text; color: "white"; font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // Presets
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Text { text: "Presets:"; color: "#aaa"; font.pixelSize: 11 }

                Repeater {
                    model: [Qt.point(3,2), Qt.point(4,3), Qt.point(5,4), Qt.point(5,3), Qt.point(7,4)]
                    delegate: Button {
                        text: modelData.x + "/" + modelData.y
                        onClicked: { root._primaryVal = modelData.x; root._secondaryVal = modelData.y }
                        background: Rectangle { color: "#555"; radius: 3 }
                        contentItem: Text {
                            text: parent.text; color: "white"; font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        }
                        implicitWidth: 42; implicitHeight: 28
                    }
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
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
                    onClicked: {
                        controller.setPolyrhythm(root._primaryVal, root._secondaryVal, root._perBeatVal)
                        root.close()
                    }
                    background: Rectangle { color: controller.accentColor; radius: 3 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }
}
