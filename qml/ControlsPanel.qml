import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root
    property var app
    Layout.fillHeight: true
    Layout.preferredWidth: 340
    Layout.minimumWidth: 300
    background: Rectangle { color: "#121c25"; border.color: "#263946" }

    Dialog {
        id: safetyDialog
        modal: true
        title: qsTr("Confirm high-risk CAT action")
        anchors.centerIn: Overlay.overlay
        width: 500
        standardButtons: Dialog.Cancel
        visible: app.safetyConfirmation.length > 0
        onRejected: app.cancelSafety()
        contentItem: ColumnLayout {
            spacing: 10
            Label { text: qsTr("The server prepared a one-time action confirmation."); color: "#e8edf2"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: qsTr("Action digest:") + " " + app.safetyDigest; color: "#f3c969"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: qsTr("Expires at:") + " " + app.safetyExpiresAt; color: "#b9c4cf"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Button { text: qsTr("Confirm and send once"); Layout.alignment: Qt.AlignRight; onClicked: { app.confirmSafety(); safetyDialog.close() } }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        TabBar {
            id: tabs
            Layout.fillWidth: true
            TabButton { text: qsTr("Radio") }
            TabButton { text: qsTr("CAT") }
            TabButton { text: qsTr("Audio") }
        }

        StackLayout {
            currentIndex: tabs.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                spacing: 10
                Label { text: qsTr("RADIO"); color: "#78d8ee"; font.bold: true }
                Label { text: app.mode; color: "#aab8c5" }
                Label { text: qsTr("VFO A"); color: "#8d9aa5" }
                TextField {
                    id: frequencyField
                    Layout.fillWidth: true
                    text: app.frequency > 0 ? app.frequency.toString() : "0"
                    selectByMouse: true
                    validator: IntValidator { bottom: 30000; top: 75000000 }
                    onAccepted: app.setFrequency(text)
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button { text: qsTr("−"); onClicked: app.setFrequency((app.frequency - 100).toString()) }
                    Label { text: app.frequency > 0 ? (app.frequency / 1000000).toFixed(6) + " MHz" : "—"; color: "#d7f9ff"; Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter }
                    Button { text: qsTr("+"); onClicked: app.setFrequency((app.frequency + 100).toString()) }
                }
                Label { text: qsTr("Mode"); color: "#8d9aa5" }
                ComboBox {
                    id: modeBox
                    Layout.fillWidth: true
                    model: [qsTr("LSB"), qsTr("USB"), qsTr("CW"), qsTr("FM"), qsTr("AM"), qsTr("DATA")]
                    onActivated: app.sendCat("MD", "set", (currentIndex + 1).toString())
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#263946" }
                Label { text: qsTr("RX / TX"); color: "#78d8ee"; font.bold: true }
                Button {
                    id: ptt
                    Layout.fillWidth: true
                    text: app.transmitting ? qsTr("TRANSMIT — release") : qsTr("Press and hold to transmit")
                    palette.button: app.transmitting ? "#b83535" : "#264c61"
                    onPressed: app.pressPtt()
                    onReleased: app.releasePtt()
                }
                Item { Layout.fillHeight: true }
                Label { text: app.status; color: "#8ecae6"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Button { text: qsTr("Log out"); Layout.fillWidth: true; onClicked: app.logout() }
            }

            ColumnLayout {
                spacing: 10
                Label { text: qsTr("STRUCTURED CAT"); color: "#78d8ee"; font.bold: true }
                Label { text: qsTr("High-risk commands require a one-time server confirmation."); color: "#f3c969"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                TextField { id: catCommand; Layout.fillWidth: true; placeholderText: qsTr("Command, e.g. FA or MD"); maximumLength: 2; inputMethodHints: Qt.ImhUppercaseOnly }
                ComboBox { id: catOperation; Layout.fillWidth: true; model: [qsTr("read"), qsTr("set"), qsTr("action")] }
                TextField { id: catValue; Layout.fillWidth: true; placeholderText: qsTr("value (optional for read)"); enabled: catOperation.currentIndex !== 0 }
                Button {
                    text: qsTr("Send CAT")
                    Layout.fillWidth: true
                    enabled: catCommand.text.length === 2
                    onClicked: app.sendCat(catCommand.text, ["read", "set", "action"][catOperation.currentIndex], catValue.text)
                }
                Label { text: qsTr("All messages use the v1 JSON envelope; raw CAT strings are never sent."); color: "#718493"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Item { Layout.fillHeight: true }
            }

            ColumnLayout {
                spacing: 10
                Label { text: qsTr("AUDIO"); color: "#78d8ee"; font.bold: true }
                CheckBox { text: qsTr("RX audio"); checked: app.audioRunning; onCheckedChanged: checked ? app.startAudio() : app.stopAudio() }
                CheckBox { text: qsTr("TX microphone"); checked: app.audioRunning }
                Label { text: qsTr("PCM S16LE · 48 kHz · 20 ms"); color: "#aab8c5" }
                Label { text: qsTr("The current baseline uses the protocol's interoperable PCM codec."); color: "#718493"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Item { Layout.fillHeight: true }
            }
        }
    }
}
