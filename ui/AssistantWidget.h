#pragma once

// ---------------------------------------------------------------------------
// AssistantWidget — Animated circle UI for AI voice assistant
// ---------------------------------------------------------------------------
// A QPainter-based widget that renders a pulsating/orbiting circle to provide
// visual feedback for the voice assistant's current state:
//
//   Idle       — gentle breathing pulse, muted colour
//   Listening  — expanding ripples, bright accent
//   Processing — orbiting ring of dots, rotating
//   Speaking   — amplitude-driven waveform ring
//
// This widget is self-contained; call setState() to transition and it
// handles all animation internally via a QTimer.
// ---------------------------------------------------------------------------

#ifndef ATLAS_UI_ASSISTANTWIDGET_H
#define ATLAS_UI_ASSISTANTWIDGET_H

#include <QColor>
#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

namespace atlas::ui {

class AssistantWidget : public QWidget {
    Q_OBJECT

public:
    /// Assistant visual states.
    enum class State {
        Idle,
        Listening,
        Processing,
        Speaking
    };

    explicit AssistantWidget(QWidget* parent = nullptr);
    ~AssistantWidget() override;

    /// Get the current visual state.
    State state() const;

    /// Set the speech amplitude (0.0–1.0) for the Speaking state waveform.
    void setSpeechAmplitude(float amplitude);

public slots:
    /// Transition to a new visual state.
    void setState(State newState);

signals:
    /// Emitted when the user clicks the centre circle (toggle mic).
    void circleClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

private:
    // Animation tick
    QTimer animTimer_;
    QElapsedTimer elapsed_;

    State state_{State::Idle};
    float speechAmplitude_{0.0f};

    // Derived colours per state — set in setState().
    QColor coreColor_;
    QColor glowColor_;
    QColor ringColor_;

    void updateColours();

    // Paint helpers
    void drawIdleCircle(class QPainter& p, const QPointF& centre, double radius,
                        double t) const;
    void drawListeningCircle(class QPainter& p, const QPointF& centre,
                             double radius, double t) const;
    void drawProcessingCircle(class QPainter& p, const QPointF& centre,
                              double radius, double t) const;
    void drawSpeakingCircle(class QPainter& p, const QPointF& centre,
                            double radius, double t) const;

    static constexpr int kFps = 60;
};

} // namespace atlas::ui

#endif // ATLAS_UI_ASSISTANTWIDGET_H
