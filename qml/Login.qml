import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: loginPage
    property var app

    background: Rectangle { color: "#101820" }

        Dialog {
        id: certificateDialog
        modal: true
        title: qsTr("Trust server certificate")
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Cancel
        visible: app.pendingFingerprint.length > 0
        onRejected: app.rejectPendingCertificate()
        contentItem: ColumnLayout {
            width: 520
            spacing: 10
            Label { text: qsTr("The server certificate is not trusted yet."); color: "#e8edf2"; wrapMode: Text.WordWrap }
            Label { text: qsTr("Subject: ") + app.pendingCertificateSubject; color: "#b9c4cf"; wrapMode: Text.WordWrap }
            Label { text: qsTr("SHA-256 fingerprint:"); color: "#b9c4cf" }
            TextArea { text: app.pendingFingerprint; readOnly: true; selectByMouse: true; Layout.fillWidth: true; wrapMode: TextArea.Wrap; color: "#d7f9ff"; background: Rectangle { color: "#172734"; radius: 4 } }
            Label { text: qsTr("Only trust this fingerprint if it was verified through an independent channel."); color: "#f3c969"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Button { text: qsTr("Trust and connect"); Layout.alignment: Qt.AlignRight; onClicked: { app.trustPendingCertificate(); certificateDialog.close() } }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 520)
        spacing: 16

        Label {
            text: qsTr("FT Remote")
            font.pixelSize: 32
            font.bold: true
            color: "#d7f9ff"
            Layout.alignment: Qt.AlignHCenter
        }
        Label {
            text: qsTr("Connect to your FTBridge station")
            color: "#93a7b8"
            Layout.alignment: Qt.AlignHCenter
        }

        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: "#172734"; radius: 8; border.color: "#294457" }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12
                Label { text: qsTr("Server"); color: "#b9c4cf" }
                TextField {
                    id: serverField
                    Layout.fillWidth: true
                    text: app.server
                    placeholderText: qsTr("https://radio.example.com:8787")
                    onTextChanged: if (activeFocus) app.server = text
                }
                Label { text: qsTr("Username"); color: "#b9c4cf" }
                TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    text: app.username
                    onTextChanged: if (activeFocus) app.username = text
                }
                Label { text: qsTr("Password"); color: "#b9c4cf" }
                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    echoMode: TextInput.Password
                    text: app.savedPassword
                }
                CheckBox {
                    id: rememberBox
                    text: qsTr("Remember password in the system keychain")
                    checked: app.rememberPassword
                    onCheckedChanged: app.setRememberPassword(checked)
                }
                Label { text: app.status; color: "#8ecae6"; visible: text.length > 0; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Button {
                    text: qsTr("Connect")
                    Layout.fillWidth: true
                    enabled: serverField.text.length > 0 && usernameField.text.length > 0 && passwordField.text.length > 0
                    onClicked: app.login(serverField.text, usernameField.text, passwordField.text, rememberBox.checked)
                }
            }
        }
        Label { text: qsTr("TLS is verified by the system trust store or an explicit first-use fingerprint."); color: "#6f8595"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }

    Connections {
        target: app
        function onLoginFieldsChanged() {
            if (!serverField.activeFocus) serverField.text = app.server
            if (!usernameField.activeFocus) usernameField.text = app.username
            if (!passwordField.activeFocus) passwordField.text = app.savedPassword
            rememberBox.checked = app.rememberPassword
        }
        function onLoginError(message) {
            errorLabel.text = message
            errorLabel.visible = true
        }
    }

    Label {
        id: errorLabel
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        width: Math.min(parent.width - 48, 680)
        visible: false
        color: "#ff8a80"
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }
}
