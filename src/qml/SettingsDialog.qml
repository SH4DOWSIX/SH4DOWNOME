import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs

Dialog {
    id: root
    title: "Settings"
    modal: true
    width: 300
    anchors.centerIn: Overlay.overlay

    property string initialSoundSet:     "Default"
    property color  initialAccentColor:  "#960000"
    property bool   initialObsEnabled:   false
    property bool   initialAlwaysOnTop:  false

    property string selectedSoundSet:    soundSetCombo.currentText
    property color  selectedAccentColor: colorRect.currentColor
    property bool   selectedObsEnabled:  obsCheck.checked
    property bool   selectedAlwaysOnTop: topCheck.checked

    standardButtons: Dialog.Ok | Dialog.Cancel
    function displaySoundSetName(name) {
        if (name === "Woodblock")
            return "Wooden"
        if (name === "Woodblock 2")
            return "Wooden 3"
        return name
    }

    onOpened: {
        var i = soundSets.indexOf(displaySoundSetName(initialSoundSet))
        soundSetCombo.currentIndex = i >= 0 ? i : 0
        colorRect.currentColor = initialAccentColor
        obsCheck.checked        = initialObsEnabled
        topCheck.checked        = initialAlwaysOnTop
    }

    background: Rectangle { color: "#353535"; radius: 4; border.color: "#555" }

    property var soundSets: ["Default", "Bongo", "Cowbell", "Digital", "Drum", "Hihat", "Metal", "Wooden", "Wooden 2", "Wooden 3"]

    ColumnLayout {
        width: parent.width
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Label { text: "Sound Set:"; color: "white"; Layout.fillWidth: true }
            ComboBox {
                id: soundSetCombo
                model: root.soundSets
                background: Rectangle { color: "#2a2a2a"; radius: 3; border.color: "#555" }
                contentItem: Text { text: soundSetCombo.displayText; color: "white"; leftPadding: 6; verticalAlignment: Text.AlignVCenter }
                Layout.preferredWidth: 110
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label { text: "Accent Color:"; color: "white"; Layout.fillWidth: true }
            Rectangle {
                id: colorRect
                property color currentColor: initialAccentColor
                width: 40; height: 24; color: currentColor
                radius: 3; border.color: "#555"
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: colorDialog.open()
                }
            }
            ColorDialog {
                id: colorDialog
                title: "Choose Accent Color"
                selectedColor: colorRect.currentColor
                onAccepted: colorRect.currentColor = selectedColor
            }
        }

        CheckBox {
            id: obsCheck
            text: "Enable OBS Beat Window"
            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter; leftPadding: parent.indicator.width + parent.spacing }
        }

        CheckBox {
            id: topCheck
            text: "Always on top (requires restart)"
            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter; leftPadding: parent.indicator.width + parent.spacing; wrapMode: Text.Wrap }
        }
    }
}
