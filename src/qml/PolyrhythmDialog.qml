import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: root
    title: "Polyrhythm"
    modal: true
    width: 260
    anchors.centerIn: Overlay.overlay

    property int initialPrimary:   3
    property int initialSecondary: 2
    property int selectedPrimary:   primarySpin.value
    property int selectedSecondary: secondarySpin.value

    standardButtons: Dialog.Ok | Dialog.Cancel
    onOpened:   { primarySpin.value = initialPrimary; secondarySpin.value = initialSecondary }

    background: Rectangle { color: "#353535"; radius: 4; border.color: "#555" }

    ColumnLayout {
        width: parent.width
        spacing: 10

        Label {
            text: "Set the polyrhythm ratio.\nMain beats against secondary beats."
            color: "#ccc"; font.pixelSize: 11; wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Label { text: "Main beats:"; color: "white"; Layout.fillWidth: true }
            SpinBox {
                id: primarySpin
                from: 2; to: 16; value: initialPrimary
                contentItem: TextInput {
                    text: primarySpin.textFromValue(primarySpin.value, primarySpin.locale)
                    color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    readOnly: !primarySpin.editable; validator: primarySpin.validator; inputMethodHints: Qt.ImhFormattedNumbersOnly
                }
                background: Rectangle { color: "#2a2a2a"; border.color: "#555"; radius: 3 }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label { text: "Secondary beats:"; color: "white"; Layout.fillWidth: true }
            SpinBox {
                id: secondarySpin
                from: 2; to: 16; value: initialSecondary
                contentItem: TextInput {
                    text: secondarySpin.textFromValue(secondarySpin.value, secondarySpin.locale)
                    color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    readOnly: !secondarySpin.editable; validator: secondarySpin.validator; inputMethodHints: Qt.ImhFormattedNumbersOnly
                }
                background: Rectangle { color: "#2a2a2a"; border.color: "#555"; radius: 3 }
            }
        }

        // Common presets
        Text { text: "Common presets:"; color: "#aaa"; font.pixelSize: 11 }
        RowLayout {
            spacing: 4
            Repeater {
                model: [Qt.point(3,2), Qt.point(4,3), Qt.point(5,4), Qt.point(5,3), Qt.point(7,4)]
                delegate: Button {
                    text: modelData.x + "/" + modelData.y
                    onClicked: { primarySpin.value = modelData.x; secondarySpin.value = modelData.y }
                    background: Rectangle { color: "#555"; radius: 3 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    Layout.preferredWidth: 40
                }
            }
        }
    }
}
