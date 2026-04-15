// ui/MainWindow.cpp — drag-and-drop Qt window for ONNX wake word models
// ---------------------------------------------------------------------------
// Accepts .onnx files dropped onto the window, copies them into the managed
// models directory, and loads them into the WakeWordEngine at runtime.
//
// Requirements:
//   - Qt6::Widgets (and optionally Qt6::Quick for the QML-based UI).
//   - ATLAS_HAS_ONNXRUNTIME is optional; the UI works regardless.
// ---------------------------------------------------------------------------

#include "MainWindow.h"

#include "wake/ModelManager.h"
#include "wake/WakeWordEngine.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMimeData>
#include <QUrl>
#include <QVBoxLayout>

#include <iostream>

namespace atlas::ui {

// ---------------------------------------------------------------------------
// Shared drop-hint stylesheets (avoids duplication across event handlers)
// ---------------------------------------------------------------------------

static const char* kDropHintNormal =
    "QLabel {"
    "  background-color: #1E1E2E;"
    "  color: #A9DFBF;"
    "  font-size: 16px;"
    "  padding: 24px;"
    "  border: 2px dashed #58D68D;"
    "  border-radius: 8px;"
    "}";

static const char* kDropHintActive =
    "QLabel {"
    "  background-color: #2E4E3E;"
    "  color: #58D68D;"
    "  font-size: 16px;"
    "  padding: 24px;"
    "  border: 2px solid #58D68D;"
    "  border-radius: 8px;"
    "}";

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

MainWindow::MainWindow(atlas::wake::WakeWordEngine& engine,
                       atlas::wake::ModelManager& manager,
                       QWidget* parent)
    : QMainWindow(parent), engine_(engine), manager_(manager) {
    setupUi();
    setAcceptDrops(true);
    refreshModelList();
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
// UI setup
// ---------------------------------------------------------------------------

void MainWindow::setupUi() {
    setWindowTitle("Atlas Assistant — Wake Word Models");
    resize(600, 420);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* layout = new QVBoxLayout(central);

    // Drop hint banner.
    dropHint_ = new QLabel(
        "Drag and drop .onnx wake word model files here", central);
    dropHint_->setAlignment(Qt::AlignCenter);
    dropHint_->setStyleSheet(kDropHintNormal);
    layout->addWidget(dropHint_);

    // Model list.
    auto* listLabel = new QLabel("Active Wake Word Models:", central);
    listLabel->setStyleSheet("color: white; font-size: 14px; margin-top: 12px;");
    layout->addWidget(listLabel);

    modelList_ = new QListWidget(central);
    modelList_->setStyleSheet(
        "QListWidget {"
        "  background-color: #222222;"
        "  color: #F5F5F5;"
        "  font-size: 14px;"
        "  border: 1px solid #444;"
        "}"
        "QListWidget::item { padding: 6px; }");
    layout->addWidget(modelList_);

    // Status bar label.
    statusLabel_ = new QLabel("Ready", central);
    statusLabel_->setStyleSheet(
        "color: #888; font-size: 12px; margin-top: 4px;");
    layout->addWidget(statusLabel_);

    central->setStyleSheet("background-color: #0F0F0F;");
}

// ---------------------------------------------------------------------------
// Model list refresh
// ---------------------------------------------------------------------------

void MainWindow::refreshModelList() {
    modelList_->clear();
    const auto models = manager_.listModels();
    for (const auto& name : models) {
        modelList_->addItem(QString::fromStdString(name));
    }

    if (models.empty()) {
        modelList_->addItem("(no models loaded)");
    }
}

// ---------------------------------------------------------------------------
// Status display
// ---------------------------------------------------------------------------

void MainWindow::showStatus(const QString& message) {
    statusLabel_->setText(message);
    std::cout << "[MainWindow] " << message.toStdString() << '\n';
}

// ---------------------------------------------------------------------------
// Drag-and-drop handlers
// ---------------------------------------------------------------------------

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        // Accept the drag only if at least one URL ends with .onnx.
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.toLocalFile().endsWith(".onnx", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                dropHint_->setStyleSheet(kDropHintActive);
                return;
            }
        }
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent* /*event*/) {
    // Restore normal drop hint styling when the drag leaves the window.
    dropHint_->setStyleSheet(kDropHintNormal);
}

void MainWindow::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    // Restore normal drop hint styling.
    dropHint_->setStyleSheet(kDropHintNormal);

    if (!event->mimeData()->hasUrls()) {
        return;
    }

    int loaded = 0;
    int rejected = 0;

    for (const QUrl& url : event->mimeData()->urls()) {
        const QString localPath = url.toLocalFile();
        if (localPath.isEmpty()) {
            continue;
        }

        if (!localPath.endsWith(".onnx", Qt::CaseInsensitive)) {
            showStatus("Rejected: " + QFileInfo(localPath).fileName()
                       + " (not an .onnx file)");
            ++rejected;
            continue;
        }

        const bool ok = manager_.addModel(localPath.toStdString());
        if (ok) {
            showStatus("Loaded: " + QFileInfo(localPath).fileName());
            ++loaded;
        } else {
            showStatus("Failed to load: " + QFileInfo(localPath).fileName());
            ++rejected;
        }
    }

    refreshModelList();

    if (loaded > 0 && rejected == 0) {
        showStatus(QString("Successfully loaded %1 model(s)").arg(loaded));
    } else if (loaded > 0) {
        showStatus(QString("Loaded %1, rejected %2 file(s)").arg(loaded).arg(rejected));
    }

    event->acceptProposedAction();
}

} // namespace atlas::ui
