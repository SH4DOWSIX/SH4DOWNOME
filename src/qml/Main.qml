import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import com.sh4downome 1.0

ApplicationWindow {
    id: root
    title: "SH4DOWNOME v" + appVersion
    visible: true
    minimumWidth: 360
    width: 360
    minimumHeight: 780
    height: 780

    flags: Qt.Window
             | Qt.WindowTitleHint
             | Qt.WindowSystemMenuHint
             | Qt.WindowMinimizeButtonHint
             | Qt.WindowMaximizeButtonHint
             | Qt.WindowCloseButtonHint
             | (controller.alwaysOnTop ? Qt.WindowStaysOnTopHint : 0)

    // Dark background
    color: "#303030"

    readonly property color panelColor: "#242424"
    readonly property color panelColor2: "#292929"
    readonly property color cardColor: "#1b1b1b"
    readonly property color cardBorder: "#3b3b3b"
    readonly property color buttonColor: "#222222"
    readonly property color buttonHoverColor: "#2c2c2c"
    readonly property color buttonPressedColor: "#191919"
    readonly property color buttonBorder: "#3f3f3f"
    readonly property color fieldColor: "#1f1f1f"
    readonly property color fieldBorder: "#171717"
    readonly property color mutedText: "#a8a8a8"

    // Keep native dialog accent colour in sync with app theme
    Binding { target: androidInput; property: "accentColor"; value: controller.accentColor }

    // ── Android native input dialog bridge ───────────────────────────────
    property string  _pendingInputType:    ""
    property var     _pendingNumCallback:  null
    property int     _pendingMin:          0
    property int     _pendingMax:          300
    property int     _pendingSectionIndex: -1
    property string  _pendingPresetName:   ""
    property string  _pendingPresetSource:  "" // "desktop" or "android"

    Connections {
        target: androidInput
        function onAccepted(text) {
            if (root._pendingInputType === "number") {
                var v = parseInt(text)
                if (!isNaN(v) && v >= root._pendingMin && v <= root._pendingMax && root._pendingNumCallback)
                    root._pendingNumCallback(v)
            } else if (root._pendingInputType === "label") {
                if (root._pendingSectionIndex >= 0)
                    controller.setSectionLabel(root._pendingSectionIndex, text)
            } else if (root._pendingInputType === "piece") {
                var name = text.trim()
                if (name.length > 0) {
                    if (controller.presetNameExists(name)) {
                        root._pendingPresetName = name
                        root._pendingPresetSource = "android"
                        duplicatePresetMsg.open()
                    }
                    else controller.savePreset(name)
                }
            } else if (root._pendingInputType === "bulkAdd") {
                var parts = text.split("|")
                if (parts.length === 2) {
                    var target = parseInt(parts[0])
                    var step   = parseInt(parts[1])
                    if (!isNaN(target) && !isNaN(step) && step > 0)
                        controller.addSectionRange(target, step)
                }
            }
            root._pendingInputType = ""
        }
        function onCancelled() { root._pendingInputType = "" }
    }

    function openNumberInput(label, val, minV, maxV, cb) {
        if (Qt.platform.os === "android") {
            root._pendingInputType = "number"
            root._pendingMin = minV; root._pendingMax = maxV; root._pendingNumCallback = cb
            androidInput.showNumber(label, val)
        } else {
            numberInputSheet.sheetLabel = label; numberInputSheet.currentVal = val
            numberInputSheet.minVal = minV; numberInputSheet.maxVal = maxV
            numberInputSheet.applyValue = cb; numberInputSheet.openWithFocus()
        }
    }
    function openLabelInput(idx, currentLabel) {
        if (Qt.platform.os === "android") {
            root._pendingInputType = "label"; root._pendingSectionIndex = idx
            androidInput.showText("Section Label", currentLabel)
        } else {
            labelInputSheet.sectionIndex = idx; labelField.text = currentLabel
            labelInputSheet.openWithFocus()
        }
    }
    function openPieceInput() {
        if (Qt.platform.os === "android") {
            root._pendingInputType = "piece"
            androidInput.showText(controller.terminology + " name", "")
        } else {
            newPieceDialog.openFresh()
        }
    }
    function openBulkAdd() {
        if (Qt.platform.os === "android") {
            root._pendingInputType = "bulkAdd"
            androidInput.showBulkAdd(controller.tempo)
        } else {
            bulkAddSheet.open()
        }
    }

    // ── Bottom-sheet dialogs ──────────────────────────────────────────────
    SubdivisionPickerSheet  { id: subSheet }
    TimeSignatureSheet      { id: timeSigSheet }
    PolyrhythmSheet         { id: polySheet }
    SettingsSheet           { id: settingsSheet }
    PresetPickerSheet       { id: pieceSheet }
    CustomSubdivisionSheet  { id: customSubSheet }
    BackupSheet             { id: backupSheet }

    Connections {
        target: settingsSheet
        function onOpenBackupRequested() {
            settingsSheet.close()
            backupSheet.open()
        }
    }

    Connections {
        target: backupSheet
        function onGoBack() {
            backupSheet.close()
            settingsSheet.open()
        }
    }

    // ── Shared dim backdrop for input strips ────────────────────────────
    Rectangle {
        id: inputBackdrop
        width: parent.width; height: parent.height
        color: "#80000000"
        z: 199
        visible: numberInputSheet.visible || labelInputSheet.visible || newPieceDialog.visible || bulkAddSheet.visible
        MouseArea {
            anchors.fill: parent
            onClicked: { numberInputSheet.close(); labelInputSheet.close(); newPieceDialog.close(); bulkAddSheet.close() }
        }
    }

    // ── Number input popup (centered, desktop) ────────────────────────
    Rectangle {
        id: numberInputSheet
        width: 300; height: 160
        anchors.centerIn: parent
        z: 200; visible: false
        color: "#1e1e1e"; border.color: "#555"; border.width: 1; radius: 6

        property string sheetLabel: "Value"
        property int minVal: 0
        property int maxVal: 300
        property int currentVal: 0
        property var applyValue: null

        ColumnLayout {
            anchors { fill: parent; margins: 16 }
            spacing: 12
            Text {
                text: numberInputSheet.sheetLabel
                color: "white"; font.pixelSize: 14; font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            TextField {
                id: numberField
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhDigitsOnly
                horizontalAlignment: TextInput.AlignHCenter
                font.pixelSize: 22; color: "white"
                selectionColor: controller.accentColor; selectedTextColor: "white"
                background: Rectangle { color: "#333"; radius: 4 }
                validator: IntValidator { bottom: numberInputSheet.minVal; top: numberInputSheet.maxVal }
                onAccepted: numberInputSheet.commitAndClose()
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Button {
                    text: "Cancel"; Layout.fillWidth: true; Layout.preferredHeight: 36
                    onClicked: numberInputSheet.close()
                    background: Rectangle { color: "#555"; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "OK"; Layout.fillWidth: true; Layout.preferredHeight: 36
                    onClicked: numberInputSheet.commitAndClose()
                    background: Rectangle { color: controller.accentColor; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        function close() { visible = false }
        function commitAndClose() {
            var v = parseInt(numberField.text)
            if (!isNaN(v) && v >= minVal && v <= maxVal && applyValue) applyValue(v)
            close()
        }
        Timer {
            id: numberKeyboardTimer; interval: 0
            onTriggered: { numberField.forceActiveFocus(Qt.MouseFocusReason) }
        }
        function openWithFocus() {
            numberField.text = String(currentVal)
            numberField.selectAll()
            visible = true
            numberKeyboardTimer.restart()
        }
    }

    // ── Label input popup (centered, desktop) ───────────────────────────
    Rectangle {
        id: labelInputSheet
        width: 300; height: 160
        anchors.centerIn: parent
        z: 200; visible: false
        color: "#1e1e1e"; border.color: "#555"; border.width: 1; radius: 6

        property int sectionIndex: -1

        ColumnLayout {
            anchors { fill: parent; margins: 16 }
            spacing: 12
            Text {
                text: "Section Label"
                color: "white"; font.pixelSize: 14; font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            TextField {
                id: labelField
                Layout.fillWidth: true
                font.pixelSize: 18; color: "white"
                selectionColor: controller.accentColor; selectedTextColor: "white"
                background: Rectangle { color: "#333"; radius: 4 }
                onAccepted: labelInputSheet.commitAndClose()
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Button {
                    text: "Cancel"; Layout.fillWidth: true; Layout.preferredHeight: 36
                    onClicked: labelInputSheet.close()
                    background: Rectangle { color: "#555"; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "OK"; Layout.fillWidth: true; Layout.preferredHeight: 36
                    onClicked: labelInputSheet.commitAndClose()
                    background: Rectangle { color: controller.accentColor; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        function close() { visible = false }
        function commitAndClose() {
            if (sectionIndex >= 0) controller.setSectionLabel(sectionIndex, labelField.text)
            close()
        }
        Timer {
            id: labelKeyboardTimer; interval: 0
            onTriggered: { labelField.forceActiveFocus(Qt.MouseFocusReason) }
        }
        function openWithFocus() {
            labelField.selectAll()
            visible = true
            labelKeyboardTimer.restart()
        }
    }

    Connections {
        target: controller
        function onCustomEditorReady() { customSubSheet.open() }
        function onRunningChanged() {
            if (controller.beatWindowAuto) {
                if (controller.running) root.beatWindowOpen = true
                else root.beatWindowOpen = false
            }
        }
    }

    // Beat Window (full-screen overlay)
    property bool beatWindowOpen: false

    BeatWindow {
        id: beatWindow
        anchors.fill: parent
        z: 100
        visible: beatWindowOpen
        onCloseRequested: root.beatWindowOpen = false
    }

    // ── New piece name popup ─────────────────────────────────────────────
    Rectangle {
        id: newPieceDialog
        width: 300; height: 160
        anchors.centerIn: parent
        z: 200; visible: false
        color: "#1e1e1e"; border.color: "#555"; border.width: 1; radius: 6

        ColumnLayout {
            anchors { fill: parent; margins: 16 }
            spacing: 12
            Text {
                text: "New " + controller.terminology
                color: "white"; font.pixelSize: 14; font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            TextField {
                id: saveNameField
                Layout.fillWidth: true
                font.pixelSize: 15; color: "white"
                selectionColor: controller.accentColor; selectedTextColor: "white"
                background: Rectangle { color: "#333"; radius: 4 }
                onAccepted: newPieceDialog.doAccept()
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Button {
                    text: "Cancel"; Layout.fillWidth: true
                    onClicked: newPieceDialog.close()
                    background: Rectangle { color: "#555"; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "OK"; Layout.fillWidth: true
                    onClicked: newPieceDialog.doAccept()
                    background: Rectangle { color: controller.accentColor; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        function close() { visible = false; Qt.inputMethod.hide() }
        function doAccept() {
            Qt.inputMethod.commit()
            var name = saveNameField.text.trim()
            if (name.length === 0) return
            if (controller.presetNameExists(name)) {
                root._pendingPresetName = name
                root._pendingPresetSource = "desktop"
                duplicatePresetMsg.open()
                return
            }
            controller.savePreset(name)
            close()
        }
        Timer {
            id: pieceKeyboardTimer; interval: 0
            onTriggered: { saveNameField.forceActiveFocus(Qt.MouseFocusReason); Qt.inputMethod.show() }
        }
        function openFresh() {
            saveNameField.text = ""
            visible = true
            pieceKeyboardTimer.restart()
        }
    }
    // ── Duplicate preset name popup ───────────────────────────────────────
    Popup {
        id: duplicatePresetMsg
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
                text: "\"" + root._pendingPresetName + "\" already exists.\nRename it or auto-append a number."
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
                    Text { anchors.centerIn: parent; text: "Go Back"; color: "#aaa"; font.pixelSize: 14 }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            duplicatePresetMsg.close()
                            if (root._pendingPresetSource === "android") {
                                root._pendingInputType = "piece"
                                androidInput.showText("Save " + controller.terminology, root._pendingPresetName)
                            } else {
                                // desktop — dialog is still open, just bring focus back
                                saveNameField.forceActiveFocus()
                                saveNameField.selectAll()
                            }
                        }
                    }
                }

                Rectangle { width: 1; height: 44; color: "#333" }

                Rectangle {
                    Layout.fillWidth: true; height: 44
                    color: "#252525"
                    Text { anchors.centerIn: parent; text: "Auto-rename"; color: controller.accentColor; font.pixelSize: 14; font.bold: true }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            duplicatePresetMsg.close()
                            var base = root._pendingPresetName
                            var candidate = base
                            var n = 2
                            while (controller.presetNameExists(candidate))
                                candidate = base + " " + (n++)
                            controller.savePreset(candidate)
                        }
                    }
                }
            }
        }
    }

    // ── Bulk add section range popup ─────────────────────────────────────
    Rectangle {
        id: bulkAddSheet
        width: 300; height: 230
        anchors.centerIn: parent
        z: 200; visible: false
        color: "#1e1e1e"; border.color: "#555"; border.width: 1; radius: 6

        property int fromTempo: 0

        function open() {
            fromTempo = controller.tempo
            bulkTargetField.text = ""
            bulkStepInput.text = "5"
            visible = true
        }
        function close() { visible = false; Qt.inputMethod.hide() }
        function commitAndClose() {
            var target = parseInt(bulkTargetField.text)
            var step   = parseInt(bulkStepInput.text)
            if (!isNaN(target) && !isNaN(step) && step > 0 && target >= 20 && target <= 300)
                controller.addSectionRange(target, step)
            close()
        }

        ColumnLayout {
            anchors { fill: parent; margins: 16 }
            spacing: 10
            Text {
                text: "Add Section Range"
                color: "white"; font.pixelSize: 14; font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                text: "From: " + bulkAddSheet.fromTempo + " BPM"
                color: "#aaa"; font.pixelSize: 12
                Layout.alignment: Qt.AlignHCenter
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text { text: "Target BPM"; color: "white"; font.pixelSize: 12; Layout.preferredWidth: 72 }
                TextField {
                    id: bulkTargetField
                    Layout.fillWidth: true
                    inputMethodHints: Qt.ImhDigitsOnly
                    horizontalAlignment: TextInput.AlignHCenter
                    font.pixelSize: 18; color: "white"
                    selectionColor: controller.accentColor; selectedTextColor: "white"
                    background: Rectangle { color: "#333"; radius: 4 }
                    validator: IntValidator { bottom: 20; top: 300 }
                    onAccepted: bulkStepInput.forceActiveFocus(Qt.TabFocusReason)
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text { text: "BPM Step"; color: "white"; font.pixelSize: 12; Layout.preferredWidth: 72 }
                TextField {
                    id: bulkStepInput
                    Layout.fillWidth: true
                    inputMethodHints: Qt.ImhDigitsOnly
                    horizontalAlignment: TextInput.AlignHCenter
                    font.pixelSize: 18; color: "white"
                    selectionColor: controller.accentColor; selectedTextColor: "white"
                    background: Rectangle { color: "#333"; radius: 4 }
                    validator: IntValidator { bottom: 1; top: 300 }
                    onAccepted: bulkAddSheet.commitAndClose()
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Button {
                    text: "Cancel"; Layout.fillWidth: true; Layout.preferredHeight: 36
                    onClicked: bulkAddSheet.close()
                    background: Rectangle { color: "#555"; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "Add"; Layout.fillWidth: true; Layout.preferredHeight: 36
                    onClicked: bulkAddSheet.commitAndClose()
                    background: Rectangle { color: controller.accentColor; radius: 4 }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }

    // ── Main layout ────────────────────────────────────────────────────────
    ColumnLayout {
        anchors { left: parent.left; right: parent.right; top: parent.top; bottom: parent.bottom }
        anchors.margins: 6
        spacing: 4

        // ── Preset row ──────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Button {
                Layout.fillWidth: true; Layout.preferredHeight: 40
                onClicked: pieceSheet.open()
                background: InsetPanel { radius: 8 }
                contentItem: Item {
                    Text {
                        anchors { left: parent.left; leftMargin: 6; verticalCenter: parent.verticalCenter; right: pickerIcon.left; rightMargin: 4 }
                        text: controller.presetName
                        color: "white"; font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    Image {
                        id: pickerIcon
                        anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }
                        source: "qrc:/resources/svg/section_down.svg"
                        sourceSize: Qt.size(512,512); width: 16; height: 16
                        fillMode: Image.PreserveAspectFit
                        smooth: true; mipmap: true
                        opacity: 0.6
                    }
                }
            }

            Button {
                id: newPieceBtn
                Layout.preferredWidth: 40; Layout.preferredHeight: 40
                onClicked: root.openPieceInput()
                background: InsetPanel {
                    radius: 20
                    color: newPieceBtn.pressed ? "#171717" : root.fieldColor
                    border.color: newPieceBtn.pressed ? "#0f0f0f" : root.fieldBorder
                }
                contentItem: Image { source: "qrc:/resources/svg/new_piece.svg"; sourceSize: Qt.size(512,512); width: 30; height: 30; fillMode: Image.PreserveAspectFit; anchors.centerIn: parent; smooth: true; mipmap: true; opacity: newPieceBtn.pressed ? 0.65 : 1.0 }
            }

        }

        // ── Section management row ─────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                spacing: 4

                ToolIconButton {
                    Layout.fillWidth: true
                    enabled: controller.sectionTableEnabled
                    iconSource: "qrc:/resources/svg/section_up.svg"
                    onClicked: controller.moveSectionUp()
                }
                ToolIconButton {
                    Layout.fillWidth: true
                    enabled: controller.sectionTableEnabled
                    iconSource: "qrc:/resources/svg/section_down.svg"
                    onClicked: controller.moveSectionDown()
                }
                ToolIconButton {
                    Layout.fillWidth: true
                    enabled: controller.sectionTableEnabled
                    iconSource: "qrc:/resources/svg/add_section.svg"
                    iconSize: 22
                    onClicked: controller.addSection()
                }
                ToolIconButton {
                    Layout.fillWidth: true
                    enabled: controller.sectionTableEnabled
                    iconSource: "qrc:/resources/svg/addadd_section.svg"
                    iconSize: 22
                    onClicked: root.openBulkAdd()
                }
                ToolIconButton {
                    Layout.fillWidth: true
                    enabled: controller.sectionTableEnabled
                    iconSource: "qrc:/resources/svg/remove_section.svg"
                    iconSize: 22
                    onClicked: controller.removeSection()
                }
                ToolIconButton {
                    Layout.fillWidth: true
                    iconSource: "qrc:/resources/svg/settings.svg"
                    iconSize: 22
                    onClicked: settingsSheet.open()
                }
            }
        }

        // ── Section list ────────────────────────────────────────────────
        Rectangle {
            id: sectionPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 124
            color: "#1f1f1f"
            radius: 8
            border.color: "#171717"
            border.width: 1
            clip: true

            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                radius: sectionPanel.radius - 1
                color: "transparent"
                border.color: Qt.rgba(1,1,1,0.035)
                border.width: 1
            }

            ColumnLayout {
                anchors { fill: parent; margins: 3 }
                spacing: 0

                // Column headers
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 21
                    color: "transparent"
                    RowLayout {
                        anchors { fill: parent; leftMargin: 35; rightMargin: 5 }
                        spacing: 0
                        DarkHeaderLabel {
                            text: "Section"
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignLeft
                            leftPadding: 4
                        }
                        DarkHeaderLabel { text: "Time Sig";  Layout.preferredWidth: 58 }
                        DarkHeaderLabel { text: "Sub/Poly";  Layout.preferredWidth: 72 }
                        DarkHeaderLabel {
                            text: "Tempo"
                            Layout.preferredWidth: 44
                            horizontalAlignment: Text.AlignRight
                            rightPadding: 3
                        }
                    }
                }

                // Section list
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 100
                    clip: true

                    ListView {
                        id: sectionList
                        anchors { fill: parent; topMargin: 1; bottomMargin: 3 }
                        model: controller.sectionModel
                        clip: true
                        interactive: true

                        Connections {
                            target: controller
                            function onCurrentSectionIndexChanged() {
                                sectionList.positionViewAtIndex(controller.currentSectionIndex, ListView.Contain)
                            }
                        }

                        delegate: Rectangle {
                            id: delegateRoot
                            readonly property bool selected: index === controller.currentSectionIndex
                            width: ListView.view.width
                            height: 31
                            color: "transparent"

                            Rectangle {
                                anchors {
                                    fill: parent
                                    leftMargin: delegateRoot.selected ? 1 : 0
                                    rightMargin: delegateRoot.selected ? 1 : 0
                                    topMargin: delegateRoot.selected ? 2 : 0
                                    bottomMargin: delegateRoot.selected ? 2 : 0
                                }
                                radius: delegateRoot.selected ? 5 : 0
                                color: delegateRoot.selected
                                       ? controller.accentColor.darker(2.35)
                                       : (index % 2 === 0 ? "#262626" : "#222222")
                            }

                            Rectangle {
                                width: 4
                                anchors {
                                    left: parent.left
                                    leftMargin: delegateRoot.selected ? 1 : 0
                                    top: parent.top
                                    topMargin: delegateRoot.selected ? 3 : 0
                                    bottom: parent.bottom
                                    bottomMargin: delegateRoot.selected ? 3 : 0
                                }
                                radius: 2
                                color: controller.accentColor
                                visible: delegateRoot.selected
                            }

                            Rectangle {
                                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                                height: 1
                                color: delegateRoot.selected ? Qt.rgba(1,1,1,0.08) : Qt.rgba(1,1,1,0.025)
                            }

                            MouseArea {
                                anchors.fill: parent
                                z: 1
                                onClicked: controller.selectSection(index)
                                onDoubleClicked: {
                                    controller.selectSection(index)
                                    root.openLabelInput(index, sectionLabel)
                                }
                                onPressAndHold: {
                                    controller.selectSection(index)
                                    root.openLabelInput(index, sectionLabel)
                                }
                            }

                            RowLayout {
                                anchors { fill: parent; leftMargin: 7; rightMargin: 5 }
                                spacing: 0

                                // Row number
                                Rectangle {
                                    Layout.preferredWidth: 18
                                    Layout.preferredHeight: 18
                                    Layout.alignment: Qt.AlignVCenter
                                    radius: 4
                                    color: delegateRoot.selected ? Qt.rgba(1,1,1,0.18) : Qt.rgba(0,0,0,0.28)
                                    border.color: delegateRoot.selected ? Qt.rgba(1,1,1,0.30) : Qt.rgba(1,1,1,0.04)
                                    border.width: 1
                                    Text {
                                        anchors.fill: parent
                                        text: rowNumber
                                        color: delegateRoot.selected ? "white" : "#9a9a9a"
                                        font.pixelSize: 9
                                        font.bold: delegateRoot.selected
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }

                                // Label (tap+hold to edit)
                                Text {
                                    id: labelDisplay
                                    Layout.fillWidth: true
                                    leftPadding: 6
                                    text: sectionLabel
                                    color: delegateRoot.selected ? "white" : "#dddddd"
                                    font.pixelSize: 12
                                    font.bold: delegateRoot.selected
                                    clip: true
                                    verticalAlignment: Text.AlignVCenter
                                }

                                Text { text: timeSig;      color: delegateRoot.selected ? "white" : "#b8b8b8"; font.pixelSize: 11; Layout.preferredWidth: 58; horizontalAlignment: Text.AlignHCenter }
                                // Sub/Poly cell
                                Item {
                                    Layout.preferredWidth: 72
                                    height: delegateRoot.height - 4
                                    // Note image (non-poly)
                                    Image {
                                        anchors.fill: parent
                                        visible: !isPoly
                                        source: isPoly ? "" : ("image://notes/section_" + index + "_" + controller.subdivisionRevision)
                                        sourceSize.width: parent.width
                                        sourceSize.height: parent.height
                                        fillMode: Image.Pad
                                        smooth: true
                                        opacity: delegateRoot.selected ? 1.0 : 0.62
                                    }
                                    // Fraction (poly)
                                    Text {
                                        anchors.centerIn: parent
                                        visible: isPoly
                                        text: subPoly
                                        color: delegateRoot.selected ? "white" : "#b8b8b8"; font.pixelSize: 11; font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }
                                Text {
                                    text: sectionTempo
                                    color: delegateRoot.selected ? "white" : "#c0c0c0"
                                    font.pixelSize: 11
                                    font.bold: delegateRoot.selected
                                    Layout.preferredWidth: 44
                                    rightPadding: 3
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                        }
                    }
                }
            }

        }

        // ── Below list: Time Sig | Subdivision | Tempo marking ──
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 6

            // Time Signature box
            InsetPanel {
                width: 52; height: 56
                enabled: controller.sectionTableEnabled
                Text {
                    text: "Time Sig"; color: root.mutedText; font.pixelSize: 9
                    anchors { top: parent.top; topMargin: 3; horizontalCenter: parent.horizontalCenter }
                }
                ColumnLayout {
                    anchors.centerIn: parent; anchors.verticalCenterOffset: 4; spacing: -2
                    Text { Layout.fillWidth: true; text: controller.numerator;   color: "white"; font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                    Text { Layout.fillWidth: true; text: controller.denominator; color: "white"; font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: timeSigSheet.open() }
            }

            // Subdivision box
            InsetPanel {
                Layout.fillWidth: true; height: 56
                enabled: controller.sectionTableEnabled
                Text {
                    text: controller.polyrhythmEnabled ? "Polyrhythm" : "Subdivision"
                    color: root.mutedText; font.pixelSize: 9
                    anchors { top: parent.top; topMargin: 3; horizontalCenter: parent.horizontalCenter }
                }
                Image {
                    anchors { fill: parent; topMargin: 14; bottomMargin: 4; leftMargin: 4; rightMargin: 4 }
                    source: controller.polyrhythmEnabled ? ""
                            : ("image://notes/current_" + controller.subdivisionRevision)
                    sourceSize.width: width
                    sourceSize.height: height
                    fillMode: Image.Pad
                    verticalAlignment: Image.AlignBottom
                    horizontalAlignment: Image.AlignHCenter
                    visible: !controller.polyrhythmEnabled
                    smooth: true
                }
                ColumnLayout {
                    anchors.centerIn: parent; anchors.verticalCenterOffset: 4; spacing: -2
                    visible: controller.polyrhythmEnabled
                    Text { Layout.fillWidth: true; text: controller.polyPrimaryBeats;   color: "white"; font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                    Text { Layout.fillWidth: true; text: controller.polySecondaryBeats; color: "white"; font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: controller.polyrhythmEnabled ? polySheet.open() : subSheet.open()
                }
            }

            // Poly / Count In / Tap buttons
            ModeButton {
                Layout.preferredHeight: 56; implicitWidth: 50
                enabled: controller.sectionTableEnabled
                text: "Poly"
                active: controller.polyrhythmEnabled
                onClicked: controller.togglePolyrhythm()
            }
            ModeButton {
                Layout.preferredHeight: 56; implicitWidth: 56
                text: "Count In"
                active: controller.countInEnabled
                onClicked: controller.toggleCountIn()
            }
            ModeButton {
                Layout.preferredHeight: 56; implicitWidth: 50
                text: "Tap"
                onClicked: controller.tapTempo()
            }

        }

        // ── Tempo slider + spinbox + marking ──────────────────
        RowLayout {
            Layout.fillWidth: true; spacing: 6
            Image { source: "qrc:/resources/svg/tempo.svg"; sourceSize: Qt.size(20,20); width: 20; height: 20; Layout.preferredWidth: 20; Layout.preferredHeight: 20; fillMode: Image.PreserveAspectFit; smooth: false; mipmap: false }
            Slider {
                id: tempoSlider
                Layout.fillWidth: true
                implicitHeight: 20
                from: 1; to: 300; value: controller.tempo
                enabled: controller.sectionTableEnabled
                onMoved: controller.tempo = Math.round(value)
                Connections {
                    target: controller
                    function onTempoChanged() { if (!tempoSlider.pressed) tempoSlider.value = controller.tempo }
                }
                background: Rectangle {
                    x: tempoSlider.leftPadding
                    y: tempoSlider.topPadding + tempoSlider.availableHeight / 2 - height / 2
                    width: tempoSlider.availableWidth; height: 4; color: "#555"; radius: 2
                    Rectangle { width: tempoSlider.visualPosition * parent.width; height: parent.height; color: controller.accentColor; radius: 2 }
                }
                handle: Rectangle {
                    x: tempoSlider.leftPadding + tempoSlider.visualPosition * (tempoSlider.availableWidth - width)
                    y: tempoSlider.topPadding + tempoSlider.availableHeight / 2 - height / 2
                    width: 16; height: 16; radius: 8
                    color: tempoSlider.pressed ? controller.accentColor : "#888"; border.color: "#555"
                }
            }
            Rectangle {
                id: tempoValueBtn
                Layout.preferredWidth: 64; Layout.preferredHeight: 32
                color: root.fieldColor; radius: 8
                border.color: root.fieldBorder
                border.width: 1
                enabled: controller.sectionTableEnabled
                opacity: enabled ? 1.0 : 0.5
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: parent.radius - 1
                    color: "transparent"
                    border.color: Qt.rgba(1,1,1,0.035)
                    border.width: 1
                }
                Text {
                    anchors.centerIn: parent
                    text: controller.tempo
                    color: "white"; font.pixelSize: 14
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.openNumberInput("Tempo", controller.tempo, 1, 300,
                                   function(v) { controller.tempo = v })
                }
            }
        }

        // ── Tempo marking ──────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 4
            spacing: 0
            Text {
                text: {
                    var lines = controller.startStopLabel.split("\n").filter(function(l) { return l !== "Start" && l !== "Stop" })
                    return lines.join("  ")
                }
                color: "#cccccc"; font.pixelSize: 11
                visible: text.length > 0
            }
            Item { Layout.fillWidth: true }
            Text {
                text: controller.tempoMarkings
                color: "#cccccc"; font.pixelSize: 11
                horizontalAlignment: Text.AlignRight
            }
        }

        // ── Beat indicator ─────────────────────────────────────
        BeatIndicatorItem {
            id: beatIndicator
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            beats:         controller.biBeats
            subdivisions:  controller.biSubdivisions
            currentBeat:   controller.biCurrentBeat
            currentSub:    controller.biCurrentSub
            mode:          controller.biMode
            gridHighlight: controller.biGridHighlight
            polyMain:      controller.biPolyMain
            polySecondary: controller.biPolySecondary
            accentColor:   controller.accentColor
            backgroundColor: root.color
            circleBorderColor: "#101010"
            circleBorderWidth: 2
        }

        // ── Volume row ─────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Image { source: "qrc:/resources/svg/volume.svg"; sourceSize: Qt.size(20,20); width: 20; height: 20; Layout.preferredWidth: 20; Layout.preferredHeight: 20; fillMode: Image.PreserveAspectFit; smooth: false; mipmap: false }
            Slider {
                id: volSlider
                Layout.fillWidth: true
                implicitHeight: 20
                from: 0; to: 100; value: controller.volume
                onMoved: controller.volume = Math.round(value)
                background: Rectangle {
                    x: volSlider.leftPadding; y: volSlider.topPadding + volSlider.availableHeight / 2 - height / 2
                    width: volSlider.availableWidth; height: 4; color: "#555"; radius: 2
                    Rectangle { width: volSlider.visualPosition * parent.width; height: parent.height; color: controller.accentColor; radius: 2 }
                }
                handle: Rectangle {
                    x: volSlider.leftPadding + volSlider.visualPosition * (volSlider.availableWidth - width)
                    y: volSlider.topPadding + volSlider.availableHeight / 2 - height / 2
                    width: 14; height: 14; radius: 7; color: volSlider.pressed ? controller.accentColor : "#888"; border.color: "#555"
                }
                Connections {
                    target: controller
                    function onVolumeChanged() { if (!volSlider.pressed) volSlider.value = controller.volume }
                }
            }
            Rectangle {
                id: volValueBtn
                Layout.preferredWidth: 64; Layout.preferredHeight: 32
                color: root.fieldColor; radius: 8
                border.color: root.fieldBorder
                border.width: 1
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: parent.radius - 1
                    color: "transparent"
                    border.color: Qt.rgba(1,1,1,0.035)
                    border.width: 1
                }
                Text {
                    anchors.centerIn: parent
                    text: controller.volume
                    color: "white"; font.pixelSize: 14
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.openNumberInput("Volume", controller.volume, 0, 100,
                                   function(v) { controller.volume = v })
                }
            }
        }

        // ── Timer row ────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true; spacing: 6
            ModeButton {
                id: timerBtn
                Layout.preferredWidth: 46; Layout.preferredHeight: 36
                text: "Timer"
                active: controller.timerEnabled
                onClicked: controller.toggleTimer()
            }
            // Timer min display
            Rectangle {
                id: timerMinBtn
                Layout.preferredWidth: 52; Layout.preferredHeight: 32
                color: root.fieldColor; radius: 8
                border.color: root.fieldBorder
                border.width: 1
                enabled: !controller.running; opacity: enabled ? 1.0 : 0.5
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: parent.radius - 1
                    color: "transparent"
                    border.color: Qt.rgba(1,1,1,0.035)
                    border.width: 1
                }
                Text {
                    anchors.centerIn: parent
                    text: String(Math.floor(controller.timerTotalSeconds / 60)).padStart(2, '0')
                    color: "white"; font.pixelSize: 14
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.openNumberInput("Minutes",
                                   Math.floor(controller.timerTotalSeconds / 60), 0, 99,
                                   function(v) { controller.timerTotalSeconds = v * 60 + (controller.timerTotalSeconds % 60) })
                }
            }
            Text { text: ":"; color: "white"; font.pixelSize: 14 }
            Rectangle {
                id: timerSecBtn
                Layout.preferredWidth: 52; Layout.preferredHeight: 32
                color: root.fieldColor; radius: 8
                border.color: root.fieldBorder
                border.width: 1
                enabled: !controller.running; opacity: enabled ? 1.0 : 0.5
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: parent.radius - 1
                    color: "transparent"
                    border.color: Qt.rgba(1,1,1,0.035)
                    border.width: 1
                }
                Text {
                    anchors.centerIn: parent
                    text: String(controller.timerTotalSeconds % 60).padStart(2, '0')
                    color: "white"; font.pixelSize: 14
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.openNumberInput("Seconds",
                                   controller.timerTotalSeconds % 60, 0, 59,
                                   function(v) { controller.timerTotalSeconds = Math.floor(controller.timerTotalSeconds / 60) * 60 + v })
                }
            }
            Text {
                visible: controller.timerEnabled && controller.running
                text: controller.timerRemainingString
                color: "white"; font.pixelSize: 11; font.bold: true
            }
            Item { Layout.fillWidth: true }
        }

        // ── Speed row ────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true; spacing: 4
            ModeButton {
                id: speedBtn
                Layout.preferredWidth: 46; Layout.preferredHeight: 36
                text: "Speed"
                active: controller.speedEnabled
                onClicked: controller.toggleSpeed()
            }
            Text { text: "Bars"; color: root.mutedText; font.pixelSize: 11 }
            Rectangle {
                Layout.preferredWidth: 52; Layout.preferredHeight: 32
                color: root.fieldColor; radius: 8
                border.color: root.fieldBorder
                border.width: 1
                enabled: !controller.running; opacity: enabled ? 1.0 : 0.5
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: parent.radius - 1
                    color: "transparent"
                    border.color: Qt.rgba(1,1,1,0.035)
                    border.width: 1
                }
                Text { anchors.centerIn: parent; text: controller.speedBarsPerStep; color: "white"; font.pixelSize: 14 }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.openNumberInput("Bars per Step", controller.speedBarsPerStep, 1, 99,
                                   function(v) { controller.speedBarsPerStep = v })
                }
            }
            Text { text: "BPM+"; color: root.mutedText; font.pixelSize: 11 }
            Rectangle {
                Layout.preferredWidth: 52; Layout.preferredHeight: 32
                color: root.fieldColor; radius: 8
                border.color: root.fieldBorder
                border.width: 1
                enabled: !controller.running; opacity: enabled ? 1.0 : 0.5
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: parent.radius - 1
                    color: "transparent"
                    border.color: Qt.rgba(1,1,1,0.035)
                    border.width: 1
                }
                Text { anchors.centerIn: parent; text: controller.speedTempoStep; color: "white"; font.pixelSize: 14 }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.openNumberInput("BPM Step", controller.speedTempoStep, 1, 50,
                                   function(v) { controller.speedTempoStep = v })
                }
            }
            Text { text: "Max"; color: root.mutedText; font.pixelSize: 11 }
            Rectangle {
                Layout.preferredWidth: 52; Layout.preferredHeight: 32
                color: root.fieldColor; radius: 8
                border.color: root.fieldBorder
                border.width: 1
                enabled: !controller.running; opacity: enabled ? 1.0 : 0.5
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: parent.radius - 1
                    color: "transparent"
                    border.color: Qt.rgba(1,1,1,0.035)
                    border.width: 1
                }
                Text { anchors.centerIn: parent; text: controller.speedMaxTempo; color: "white"; font.pixelSize: 14 }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.openNumberInput("Max Tempo", controller.speedMaxTempo, 1, 300,
                                   function(v) { controller.speedMaxTempo = v })
                }
            }
            Item { Layout.fillWidth: true }
        }

        Item { Layout.fillWidth: true; Layout.preferredHeight: 8 }

        // ── Controls row ─────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true; Layout.preferredHeight: 53; spacing: 4

            Button {
                id: startStopBtn
                Layout.fillWidth: true; Layout.preferredHeight: 53
                onClicked: controller.startStop()
                background: InsetPanel {
                    radius: 8
                    color: controller.running
                           ? (startStopBtn.pressed ? "#531010" : "#681313")
                           : (startStopBtn.pressed ? "#00490b" : "#005d0d")
                    border.color: controller.running ? "#240606" : "#002f07"
                    innerBorderColor: controller.running ? Qt.rgba(1,0.45,0.45,0.16) : Qt.rgba(0.45,1,0.45,0.14)
                }
                contentItem: Item {
                    Text {
                        anchors.centerIn: parent
                        text: controller.running ? "STOP" : "START"
                        color: "white"; font.pixelSize: 26; font.bold: true
                    }
                }
            }

            Button {
                id: beatWindowBtn
                Layout.preferredWidth: 53; Layout.preferredHeight: 53
                onClicked: root.beatWindowOpen = true
                background: InsetPanel {
                    radius: 8
                    color: beatWindowBtn.pressed ? "#171717" : root.fieldColor
                }
                contentItem: Image {
                    source: "qrc:/resources/svg/section_up.svg"
                    sourceSize: Qt.size(512,512); width: 24; height: 24
                    fillMode: Image.PreserveAspectFit
                    anchors.centerIn: parent
                    smooth: true; mipmap: true
                }
            }
        }
    }

    // ── Keyboard shortcuts (Shortcut works at app level regardless of focus) ──
    Shortcut { sequence: " ";          onActivated: controller.startStop() }
    Shortcut { sequence: "Up";         onActivated: { var r = controller.currentSectionIndex; if (r > 0) controller.selectSection(r - 1) } }
    Shortcut { sequence: "Down";       onActivated: { controller.selectSection(controller.currentSectionIndex + 1) } }
    Shortcut { sequence: "Ctrl+Up";    enabled: controller.sectionTableEnabled; onActivated: controller.moveSectionUp() }
    Shortcut { sequence: "Ctrl+Down";  enabled: controller.sectionTableEnabled; onActivated: controller.moveSectionDown() }

    // ── Reusable inline components ─────────────────────────────────────────
    component DarkButton: Button {
        background: Rectangle { color: "#555"; radius: 3; border.color: "#333" }
        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
    }

    component ToolIconButton: Button {
        id: toolButton
        property string iconSource: ""
        property int iconSize: 21
        Layout.fillHeight: true
        implicitHeight: 32
        opacity: enabled ? 1.0 : 0.38
        background: InsetPanel {
            radius: 7
            color: toolButton.pressed ? "#171717"
                 : toolButton.hovered ? "#242424"
                 : root.fieldColor
            border.color: toolButton.hovered ? "#101010" : root.fieldBorder
        }
        contentItem: Image {
            source: toolButton.iconSource
            sourceSize: Qt.size(512,512)
            width: toolButton.iconSize
            height: toolButton.iconSize
            fillMode: Image.PreserveAspectFit
            anchors.centerIn: parent
            smooth: true
            mipmap: true
            opacity: toolButton.pressed ? 0.72 : 0.92
        }
    }

    component ModeButton: Button {
        id: modeButton
        property bool active: false
        opacity: enabled ? 1.0 : 0.45
        background: InsetPanel {
            radius: 8
            color: modeButton.active
                   ? Qt.rgba(controller.accentColor.r * 0.44, controller.accentColor.g * 0.44, controller.accentColor.b * 0.44, modeButton.pressed ? 0.90 : 0.78)
                   : (modeButton.pressed ? "#171717"
                      : modeButton.hovered ? "#242424"
                      : root.fieldColor)
            border.color: modeButton.active ? controller.accentColor
                         : modeButton.hovered ? "#101010"
                         : root.fieldBorder
            innerBorderColor: modeButton.active ? Qt.rgba(1,1,1,0.08) : Qt.rgba(1,1,1,0.035)
        }
        contentItem: Text {
            text: modeButton.text
            color: modeButton.active ? "white" : "#e2e2e2"
            font.pixelSize: 11
            font.bold: modeButton.active
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    component InsetPanel: Rectangle {
        id: insetPanel
        property color innerBorderColor: Qt.rgba(1,1,1,0.035)
        color: root.fieldColor
        radius: 8
        border.color: root.fieldBorder
        border.width: 1

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Math.max(insetPanel.radius - 1, 0)
            color: "transparent"
            border.color: insetPanel.innerBorderColor
            border.width: 1
        }
    }

    component DarkHeaderLabel: Text {
        color: "#969696"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter
        height: 22; verticalAlignment: Text.AlignVCenter
    }

    component MessageDialog: Dialog {
        property string message: ""
        modal: true; width: 320
        anchors.centerIn: parent
        standardButtons: Dialog.Ok
        Label { text: message; color: "white"; wrapMode: Text.Wrap; width: parent.width }
    }
}
