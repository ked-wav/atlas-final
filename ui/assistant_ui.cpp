// ui/assistant_ui.cpp — Chat window with Piper TTS integration
// ---------------------------------------------------------------------------
// Sends user messages to the /chat endpoint and speaks the AI reply aloud
// using PiperTTS.
// ---------------------------------------------------------------------------

#include "assistant_ui.h"

#include "voice/piper_tts.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace atlas::ui {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

AssistantWindow::AssistantWindow(QWidget* parent)
    : QMainWindow(parent) {

    // Wire up PiperTTS following the instructions at the bottom of piper_tts.h:
    //   m_tts = new atlas::voice::PiperTTS();
    m_tts = new atlas::voice::PiperTTS();

    m_network = new QNetworkAccessManager(this);

    setupUi();
}

AssistantWindow::~AssistantWindow() {
    // Clean up PiperTTS as described in piper_tts.h wiring instructions.
    delete m_tts;
}

// ---------------------------------------------------------------------------
// UI layout
// ---------------------------------------------------------------------------

void AssistantWindow::setupUi() {
    setWindowTitle("Atlas Assistant — Chat");
    resize(520, 400);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* layout = new QVBoxLayout(central);

    // Chat log (read-only).
    m_chatLog = new QTextEdit(central);
    m_chatLog->setReadOnly(true);
    m_chatLog->setStyleSheet(
        "QTextEdit {"
        "  background-color: #1E1E2E;"
        "  color: #F5F5F5;"
        "  font-size: 14px;"
        "  border: 1px solid #444;"
        "}");
    layout->addWidget(m_chatLog);

    // Input row.
    auto* inputRow = new QHBoxLayout();
    m_input = new QLineEdit(central);
    m_input->setPlaceholderText("Type a message…");
    m_input->setStyleSheet(
        "QLineEdit {"
        "  background-color: #222;"
        "  color: #EEE;"
        "  font-size: 14px;"
        "  padding: 6px;"
        "  border: 1px solid #555;"
        "  border-radius: 4px;"
        "}");
    inputRow->addWidget(m_input);

    auto* sendBtn = new QPushButton("Send", central);
    sendBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3A7BD5;"
        "  color: white;"
        "  font-size: 14px;"
        "  padding: 6px 16px;"
        "  border-radius: 4px;"
        "}");
    inputRow->addWidget(sendBtn);
    layout->addLayout(inputRow);

    central->setStyleSheet("background-color: #0F0F0F;");

    // Connect signals.
    connect(sendBtn, &QPushButton::clicked, this, &AssistantWindow::sendChat);
    connect(m_input, &QLineEdit::returnPressed, this, &AssistantWindow::sendChat);
}

// ---------------------------------------------------------------------------
// /chat request
// ---------------------------------------------------------------------------

void AssistantWindow::sendChat() {
    const QString message = m_input->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    m_chatLog->append("<b>You:</b> " + message);
    m_input->clear();

    // Build JSON payload for the /chat endpoint.
    QJsonObject payload;
    payload["message"] = message;

    QNetworkRequest request(QUrl("http://localhost:8080/chat"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply =
        m_network->post(request, QJsonDocument(payload).toJson());

    // When the /chat reply arrives, display it and speak it aloud.
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            m_chatLog->append("<i>Error: " + reply->errorString() + "</i>");
            return;
        }

        const QByteArray data = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        const QString replyText =
            doc.object().value("reply").toString(QString::fromUtf8(data));

        m_chatLog->append("<b>Atlas:</b> " + replyText);

        // Speak the assistant's reply using PiperTTS.
        m_tts->speak(replyText.toStdString());
    });
}

} // namespace atlas::ui
