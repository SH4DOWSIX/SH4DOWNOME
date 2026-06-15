import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Inline time-signature dialog: shown as a child of the ApplicationWindow.
// Callers set it active by setting its `active` Loader to true.
Dialog {
    id: root
    title: "Time Signature"
    modal: true
    width: 240
    anchors.centerIn: Overlay.overlay

    property int initialNumerator: 4
    property int initialDenominator: 4
    property int selectedNumerator: numeratorSpin.value
    property int selectedDenominator: denominatorSpin.value

    standardButtons: Dialog.Ok | Dialog.Cancel
    onOpened:   { numeratorSpin.value = initialNumerator; denominatorSpin.value = initialDenominator }

    background: Rectangle { color: "#353535"; radius: 4; border.color: "#555" }

    ColumnLayout {
        width: parent.width
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Label { text: "Beats per bar:"; color: "white"; Layout.fillWidth: true }
            SpinBox {
                id: numeratorSpin
                from: 1; to: 32; value: initialNumerator
                contentItem: TextInput {
                    text: numeratorSpin.textFromValue(numeratorSpin.value, numeratorSpin.locale)
                    color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    readOnly: !numeratorSpin.editable; validator: numeratorSpin.validator; inputMethodHints: Qt.ImhFormattedNumbersOnly
                }
                background: Rectangle { color: "#2a2a2a"; border.color: "#555"; radius: 3 }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label { text: "Beat value:"; color: "white"; Layout.fillWidth: true }
            ComboBox {
                id: denominatorSpin
                property int value: parseInt(currentText)
                model: ["2", "4", "8", "16"]
                currentIndex: {
                    var denoms = [2, 4, 8, 16]
                    return denoms.indexOf(initialDenominator)
                }
                background: Rectangle { color: "#2a2a2a"; radius: 3; border.color: "#555" }
                contentItem: Text { text: denominatorSpin.displayText; color: "white"; leftPadding: 6; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                Layout.preferredWidth: 72
            }
        }
    }
}
