#pragma once

// ---------------------------------------------------------------------------
// AssistantWindow — Chat UI that sends /chat requests and speaks replies
// ---------------------------------------------------------------------------
// A QMainWindow subclass that provides a basic text input for sending
// messages to the AI back-end via a /chat HTTP endpoint.  Responses are
// displayed in a read-only text area and vocalised through PiperTTS.
// ---------------------------------------------------------------------------

#ifndef ATLAS_UI_ASSISTANTWINDOW_H
#define ATLAS_UI_ASSISTANTWINDOW_H

#include <QLineEdit>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QTextEdit>

namespace atlas::voice {
class PiperTTS;
} // namespace atlas::voice

namespace atlas::ui {

class AssistantWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AssistantWindow(QWidget* parent = nullptr);
    ~AssistantWindow() override;

private slots:
    /// Send the current input text to the /chat endpoint.
    void sendChat();

private:
    void setupUi();

    atlas::voice::PiperTTS* m_tts;
    QNetworkAccessManager*  m_network;

    QTextEdit* m_chatLog;
    QLineEdit* m_input;
};

} // namespace atlas::ui

#endif // ATLAS_UI_ASSISTANTWINDOW_H
