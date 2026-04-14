import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 960
    height: 640
    visible: true
    title: "Atlas Assistant"
    color: "#0F0F0F"

    property string liveTranscript: "Live transcription will appear here."
    property string aiResponse: "AI response text will appear here."
    property color listRowColorA: "#222222"
    property color listRowColorB: "#2A2A2A"

    function voiceFeedback(message) {
        console.log("VOICE_FEEDBACK:" + message)
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: homeScreen
    }

    Component {
        id: homeScreen
        Rectangle {
            color: "#0F0F0F"
            anchors.fill: parent
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20
                Button {
                    text: "Talk to Assistant"
                    font.pixelSize: 24
                    Layout.preferredWidth: 420
                    Layout.preferredHeight: 72
                    onClicked: {
                        root.voiceFeedback("Listening")
                        stack.push(voiceScreen)
                    }
                }
                Button {
                    text: "Check Email"
                    font.pixelSize: 24
                    Layout.preferredWidth: 420
                    Layout.preferredHeight: 72
                    onClicked: {
                        root.voiceFeedback("Opening email")
                        stack.push(emailScreen)
                    }
                }
                Button {
                    text: "View Calendar"
                    font.pixelSize: 24
                    Layout.preferredWidth: 420
                    Layout.preferredHeight: 72
                    onClicked: {
                        root.voiceFeedback("Opening calendar")
                        stack.push(calendarScreen)
                    }
                }
            }
        }
    }

    Component {
        id: voiceScreen
        Rectangle {
            color: "#111111"
            anchors.fill: parent
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 18

                Text {
                    text: "Voice"
                    color: "white"
                    font.pixelSize: 28
                }

                Row {
                    spacing: 6
                    Repeater {
                        model: 16
                        Rectangle {
                            width: 12
                            height: 20 + (index % 5) * 14
                            color: "#58D68D"
                            radius: 3
                        }
                    }
                }

                Text {
                    text: root.liveTranscript
                    color: "#EAEAEA"
                    font.pixelSize: 18
                    wrapMode: Text.Wrap
                }

                Text {
                    text: root.aiResponse
                    color: "#A9DFBF"
                    font.pixelSize: 18
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    spacing: 16
                    Button {
                        text: "Mic"
                        font.pixelSize: 22
                        Layout.preferredHeight: 64
                        onClicked: root.voiceFeedback("Listening")
                    }
                    Button {
                        text: "Back"
                        font.pixelSize: 22
                        Layout.preferredHeight: 64
                        onClicked: {
                            root.voiceFeedback("Returning home")
                            stack.pop()
                        }
                    }
                }
            }
        }
    }

    Component {
        id: emailScreen
        Rectangle {
            color: "#111111"
            anchors.fill: parent
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 14
                Text {
                    text: "Email"
                    color: "white"
                    font.pixelSize: 28
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: ["Project Update - team@example.com", "Family Photos - aunt@example.com"]
                    delegate: Rectangle {
                        width: parent.width
                        height: 64
                        color: index % 2 === 0 ? root.listRowColorA : root.listRowColorB
                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: "#F5F5F5"
                            font.pixelSize: 18
                        }
                    }
                }
                RowLayout {
                    Button {
                        text: "Hear Summary"
                        font.pixelSize: 22
                        Layout.preferredHeight: 64
                        onClicked: root.voiceFeedback("Reading email summary")
                    }
                    Button {
                        text: "Back"
                        font.pixelSize: 22
                        Layout.preferredHeight: 64
                        onClicked: {
                            root.voiceFeedback("Returning home")
                            stack.pop()
                        }
                    }
                }
            }
        }
    }

    Component {
        id: calendarScreen
        Rectangle {
            color: "#111111"
            anchors.fill: parent
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 14
                Text {
                    text: "Calendar"
                    color: "white"
                    font.pixelSize: 28
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: ["Team Standup - Tomorrow 09:00", "Doctor - Friday 11:30"]
                    delegate: Rectangle {
                        width: parent.width
                        height: 64
                        color: index % 2 === 0 ? root.listRowColorA : root.listRowColorB
                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: "#F5F5F5"
                            font.pixelSize: 18
                        }
                    }
                }
                RowLayout {
                    Button {
                        text: "Add Event via Voice"
                        font.pixelSize: 22
                        Layout.preferredHeight: 64
                        onClicked: root.voiceFeedback("Event added")
                    }
                    Button {
                        text: "Settings"
                        font.pixelSize: 22
                        Layout.preferredHeight: 64
                        onClicked: {
                            root.voiceFeedback("Opening settings")
                            stack.push(settingsScreen)
                        }
                    }
                    Button {
                        text: "Back"
                        font.pixelSize: 22
                        Layout.preferredHeight: 64
                        onClicked: {
                            root.voiceFeedback("Returning home")
                            stack.pop()
                        }
                    }
                }
            }
        }
    }

    Component {
        id: settingsScreen
        Rectangle {
            color: "#111111"
            anchors.fill: parent
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 14
                Text {
                    text: "Settings"
                    color: "white"
                    font.pixelSize: 28
                }
                TextField {
                    placeholderText: "Email credentials"
                    font.pixelSize: 18
                    Layout.preferredHeight: 56
                }
                TextField {
                    placeholderText: "CalDAV config"
                    font.pixelSize: 18
                    Layout.preferredHeight: 56
                }
                TextField {
                    placeholderText: "LLM settings"
                    font.pixelSize: 18
                    Layout.preferredHeight: 56
                }
                Button {
                    text: "Back"
                    font.pixelSize: 22
                    Layout.preferredHeight: 64
                    onClicked: {
                        root.voiceFeedback("Returning home")
                        stack.pop()
                    }
                }
            }
        }
    }
}
