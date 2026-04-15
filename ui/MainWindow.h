#pragma once

// ---------------------------------------------------------------------------
// MainWindow — Qt Widgets window with drag-and-drop support for .onnx models
// ---------------------------------------------------------------------------
// Provides a minimal Qt Widgets UI that:
//   1. Accepts .onnx files via drag and drop.
//   2. Copies dropped files into the managed models directory.
//   3. Displays a list of currently loaded wake word models.
//   4. Shows visual feedback when a model is loaded or a wake word fires.
//
// This class is compiled only when BUILD_QT_UI is ON and Qt6::Widgets is
// available.
// ---------------------------------------------------------------------------

#ifndef ATLAS_UI_MAINWINDOW_H
#define ATLAS_UI_MAINWINDOW_H

#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QString>

namespace atlas::ui {
class AssistantWidget;
} // namespace atlas::ui

// Forward declarations — we don't want wake/ headers in the UI header when
// the consumer doesn't need them.
namespace atlas::wake {
class WakeWordEngine;
class ModelManager;
} // namespace atlas::wake

namespace atlas::ui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /// Construct the window.
    /// \p engine  WakeWordEngine instance to register dropped models with.
    /// \p manager ModelManager used to copy and track files.
    explicit MainWindow(atlas::wake::WakeWordEngine& engine,
                        atlas::wake::ModelManager& manager,
                        QWidget* parent = nullptr);

    ~MainWindow() override;

public slots:
    /// Refresh the model list widget from the ModelManager.
    void refreshModelList();

    /// Display a transient status message.
    void showStatus(const QString& message);

protected:
    // Drag-and-drop overrides.
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    atlas::wake::WakeWordEngine& engine_;
    atlas::wake::ModelManager& manager_;

    QListWidget* modelList_{nullptr};
    QLabel* statusLabel_{nullptr};
    QLabel* dropHint_{nullptr};
    atlas::ui::AssistantWidget* assistantWidget_{nullptr};

    void setupUi();
};

} // namespace atlas::ui

#endif // ATLAS_UI_MAINWINDOW_H
