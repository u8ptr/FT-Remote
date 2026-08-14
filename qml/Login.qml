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
        width: 560
        standardButtons: Dialog.Cancel
        visible: app.pendingFingerprint.length > 0
        onRejected: app.rejectPendingCertificate()
        contentItem: ColumnLayout {
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
        width: Math.min(parent.width - 64, 640)
        spacing: 14

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
            padding: 24
            background: Rectangle { color: "#132b38"; radius: 10; border.color: "#0b5d75"; border.width: 1 }
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                Label { text: qsTr("Server"); color: "#c7d8e2"; font.pixelSize: 14 }
                TextField {
                    id: serverField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    text: app.server
                    placeholderText: qsTr("https://radio.example.com:8787")
                    color: "#10212b"
                    placeholderTextColor: "#8396a5"
                    selectionColor: "#197a9a"
                    selectedTextColor: "#ffffff"
                    background: Rectangle {
                        color: "#f4f7f9"
                        radius: 5
                        border.color: serverField.activeFocus ? "#31b5d1" : "#b7c5cd"
                        border.width: serverField.activeFocus ? 2 : 1
                    }
                    onTextChanged: if (activeFocus) app.server = text
                }
                Label { text: qsTr("Username"); color: "#c7d8e2"; font.pixelSize: 14 }
                TextField {
                    id: usernameField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    text: app.username
                    color: "#10212b"
                    placeholderTextColor: "#8396a5"
                    selectionColor: "#197a9a"
                    selectedTextColor: "#ffffff"
                    background: Rectangle {
                        color: "#f4f7f9"
                        radius: 5
                        border.color: usernameField.activeFocus ? "#31b5d1" : "#b7c5cd"
                        border.width: usernameField.activeFocus ? 2 : 1
                    }
                    onTextChanged: if (activeFocus) app.username = text
                }
                Label { text: qsTr("Password"); color: "#c7d8e2"; font.pixelSize: 14 }
                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    echoMode: TextInput.Password
                    text: app.savedPassword
                    color: "#10212b"
                    placeholderTextColor: "#8396a5"
                    selectionColor: "#197a9a"
                    selectedTextColor: "#ffffff"
                    background: Rectangle {
                        color: "#f4f7f9"
                        radius: 5
                        border.color: passwordField.activeFocus ? "#31b5d1" : "#b7c5cd"
                        border.width: passwordField.activeFocus ? 2 : 1
                    }
                }
                CheckBox {
                    id: rememberBox
                    text: qsTr("Remember password in the system keychain")
                    spacing: 10
                    checked: app.rememberPassword
                    indicator: Rectangle {
                        x: 0
                        y: rememberBox.height / 2 - height / 2
                        implicitWidth: 20
                        implicitHeight: 20
                        radius: 4
                        color: rememberBox.checked ? "#1686a5" : "#0e202b"
                        border.color: rememberBox.checked ? "#31b5d1" : "#71909e"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: "#ffffff"
                            font.pixelSize: 15
                            font.bold: true
                            visible: rememberBox.checked
                        }
                    }
                    contentItem: Text {
                        leftPadding: rememberBox.indicator.width + rememberBox.spacing
                        text: rememberBox.text
                        color: "#d7e6ef"
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                    onCheckedChanged: app.setRememberPassword(checked)
                }
                Label { text: app.status; color: "#8ecae6"; visible: text.length > 0; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Button {
                    id: connectButton
                    text: qsTr("Connect")
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    enabled: serverField.text.length > 0 && usernameField.text.length > 0 && passwordField.text.length > 0
                    background: Rectangle {
                        radius: 6
                        color: connectButton.enabled ? "#087f9b" : "#29404c"
                        border.color: connectButton.enabled ? "#31b5d1" : "#3b5663"
                        border.width: 1
                    }
                    contentItem: Text {
                        text: connectButton.text
                        color: connectButton.enabled ? "#f7fcff" : "#9aabb4"
                        font.pixelSize: 15
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
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
