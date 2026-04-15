// ui/AssistantWidget.cpp — Animated Circle UI for AI voice assistant
// ---------------------------------------------------------------------------
// Renders state-dependent animated circles using QPainter.  All drawing is
// resolution-independent (based on the widget's smallest dimension) so the
// circle scales with the window.
// ---------------------------------------------------------------------------

#include "AssistantWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QRadialGradient>

#include <cmath>

namespace atlas::ui {

// ---- colour palette -------------------------------------------------------

static const QColor kIdleCore      {0x3A, 0x7B, 0xD5};       // soft blue
static const QColor kIdleGlow      {0x3A, 0x7B, 0xD5, 0x40};
static const QColor kIdleRing      {0x3A, 0x7B, 0xD5, 0x60};

static const QColor kListenCore    {0x58, 0xD6, 0x8D};       // green
static const QColor kListenGlow    {0x58, 0xD6, 0x8D, 0x38};
static const QColor kListenRing    {0x58, 0xD6, 0x8D, 0x50};

static const QColor kProcessCore   {0xF5, 0xB0, 0x41};       // amber
static const QColor kProcessGlow   {0xF5, 0xB0, 0x41, 0x38};
static const QColor kProcessRing   {0xF5, 0xB0, 0x41, 0x60};

static const QColor kSpeakCore     {0xAF, 0x7A, 0xC5};       // purple
static const QColor kSpeakGlow     {0xAF, 0x7A, 0xC5, 0x38};
static const QColor kSpeakRing     {0xAF, 0x7A, 0xC5, 0x60};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

AssistantWidget::AssistantWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(200, 200);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setCursor(Qt::PointingHandCursor);

    updateColours();

    elapsed_.start();
    animTimer_.setInterval(1000 / kFps);
    connect(&animTimer_, &QTimer::timeout, this, qOverload<>(&QWidget::update));
    animTimer_.start();
}

AssistantWidget::~AssistantWidget() = default;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

AssistantWidget::State AssistantWidget::state() const { return state_; }

void AssistantWidget::setState(State newState) {
    if (state_ == newState)
        return;
    state_ = newState;
    updateColours();
    update();
}

void AssistantWidget::setSpeechAmplitude(float amplitude) {
    speechAmplitude_ = std::clamp(amplitude, 0.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// Size hints
// ---------------------------------------------------------------------------

QSize AssistantWidget::minimumSizeHint() const { return {200, 200}; }
QSize AssistantWidget::sizeHint() const { return {400, 400}; }

// ---------------------------------------------------------------------------
// Colour mapping
// ---------------------------------------------------------------------------

void AssistantWidget::updateColours() {
    switch (state_) {
    case State::Idle:
        coreColor_ = kIdleCore;
        glowColor_ = kIdleGlow;
        ringColor_ = kIdleRing;
        break;
    case State::Listening:
        coreColor_ = kListenCore;
        glowColor_ = kListenGlow;
        ringColor_ = kListenRing;
        break;
    case State::Processing:
        coreColor_ = kProcessCore;
        glowColor_ = kProcessGlow;
        ringColor_ = kProcessRing;
        break;
    case State::Speaking:
        coreColor_ = kSpeakCore;
        glowColor_ = kSpeakGlow;
        ringColor_ = kSpeakRing;
        break;
    }
}

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------

void AssistantWidget::mousePressEvent(QMouseEvent* event) {
    const QPointF centre(width() / 2.0, height() / 2.0);
    const double r = std::min(width(), height()) * 0.28;
    const double dx = event->position().x() - centre.x();
    const double dy = event->position().y() - centre.y();
    if (dx * dx + dy * dy <= r * r) {
        emit circleClicked();
    }
    QWidget::mousePressEvent(event);
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void AssistantWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Dark background
    p.fillRect(rect(), QColor(0x0F, 0x0F, 0x0F));

    const QPointF centre(width() / 2.0, height() / 2.0);
    const double baseRadius = std::min(width(), height()) * 0.22;
    const double t = elapsed_.elapsed() / 1000.0;  // seconds

    switch (state_) {
    case State::Idle:       drawIdleCircle(p, centre, baseRadius, t);       break;
    case State::Listening:  drawListeningCircle(p, centre, baseRadius, t);  break;
    case State::Processing: drawProcessingCircle(p, centre, baseRadius, t); break;
    case State::Speaking:   drawSpeakingCircle(p, centre, baseRadius, t);   break;
    }
}

// ---- Idle: gentle breathing pulse -----------------------------------------

void AssistantWidget::drawIdleCircle(QPainter& p, const QPointF& centre,
                                      double radius, double t) const {
    const double breath = 1.0 + 0.06 * std::sin(t * 1.8);
    const double r = radius * breath;

    // Outer glow
    QRadialGradient glow(centre, r * 1.8);
    glow.setColorAt(0.0, glowColor_);
    glow.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(glow);
    p.drawEllipse(centre, r * 1.8, r * 1.8);

    // Core circle
    QRadialGradient core(centre, r);
    core.setColorAt(0.0, coreColor_.lighter(130));
    core.setColorAt(0.7, coreColor_);
    core.setColorAt(1.0, coreColor_.darker(130));
    p.setBrush(core);
    p.drawEllipse(centre, r, r);

    // Thin ring
    p.setBrush(Qt::NoBrush);
    QPen ringPen(ringColor_, 1.5);
    p.setPen(ringPen);
    const double ringR = r * 1.25 + 3.0 * std::sin(t * 1.2);
    p.drawEllipse(centre, ringR, ringR);
}

// ---- Listening: expanding ripple rings ------------------------------------

void AssistantWidget::drawListeningCircle(QPainter& p, const QPointF& centre,
                                           double radius, double t) const {
    // Ripples (3 concentric expanding rings)
    constexpr int kRipples = 3;
    for (int i = 0; i < kRipples; ++i) {
        double phase = std::fmod(t * 0.8 + i * (1.0 / kRipples), 1.0);
        double rippleR = radius * (1.2 + phase * 1.2);
        int alpha = static_cast<int>(180 * (1.0 - phase));
        QColor rippleCol = ringColor_;
        rippleCol.setAlpha(alpha);
        QPen rp(rippleCol, 2.0);
        p.setPen(rp);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(centre, rippleR, rippleR);
    }

    // Core circle (slightly pulsing)
    const double pulse = 1.0 + 0.05 * std::sin(t * 4.0);
    const double r = radius * pulse;

    QRadialGradient glow(centre, r * 1.6);
    glow.setColorAt(0.0, glowColor_);
    glow.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(glow);
    p.drawEllipse(centre, r * 1.6, r * 1.6);

    QRadialGradient core(centre, r);
    core.setColorAt(0.0, coreColor_.lighter(140));
    core.setColorAt(1.0, coreColor_);
    p.setBrush(core);
    p.drawEllipse(centre, r, r);
}

// ---- Processing: orbiting dots around the core ----------------------------

void AssistantWidget::drawProcessingCircle(QPainter& p, const QPointF& centre,
                                            double radius, double t) const {
    // Core
    QRadialGradient core(centre, radius);
    core.setColorAt(0.0, coreColor_.lighter(120));
    core.setColorAt(1.0, coreColor_);
    p.setPen(Qt::NoPen);
    p.setBrush(core);
    p.drawEllipse(centre, radius, radius);

    // Orbiting dots
    constexpr int kDots = 10;
    const double orbitR = radius * 1.45;
    const double angularSpeed = 2.0;  // radians / s

    for (int i = 0; i < kDots; ++i) {
        double angle = t * angularSpeed + i * (2.0 * M_PI / kDots);
        double dx = orbitR * std::cos(angle);
        double dy = orbitR * std::sin(angle);

        // Fade trailing dots
        int alpha = 80 + static_cast<int>(175.0 * (static_cast<double>(i) / kDots));
        QColor dotCol = ringColor_;
        dotCol.setAlpha(alpha);

        double dotR = 4.0 + 3.0 * (static_cast<double>(i) / kDots);
        p.setBrush(dotCol);
        p.drawEllipse(QPointF(centre.x() + dx, centre.y() + dy), dotR, dotR);
    }

    // Spinning arc
    const double arcR = radius * 1.25;
    QRectF arcRect(centre.x() - arcR, centre.y() - arcR, arcR * 2, arcR * 2);
    int startAngle = static_cast<int>(t * 200) % 360 * 16;
    QPen arcPen(coreColor_, 2.5, Qt::SolidLine, Qt::RoundCap);
    p.setPen(arcPen);
    p.setBrush(Qt::NoBrush);
    p.drawArc(arcRect, startAngle, 90 * 16);
}

// ---- Speaking: amplitude-driven waveform ring ----------------------------

void AssistantWidget::drawSpeakingCircle(QPainter& p, const QPointF& centre,
                                          double radius, double t) const {
    const double amp = static_cast<double>(speechAmplitude_);

    // Outer glow scaled by amplitude
    const double glowR = radius * (1.5 + 0.3 * amp);
    QRadialGradient glow(centre, glowR);
    glow.setColorAt(0.0, glowColor_);
    glow.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(glow);
    p.drawEllipse(centre, glowR, glowR);

    // Core
    const double coreR = radius * (1.0 + 0.04 * amp);
    QRadialGradient core(centre, coreR);
    core.setColorAt(0.0, coreColor_.lighter(130));
    core.setColorAt(1.0, coreColor_);
    p.setBrush(core);
    p.drawEllipse(centre, coreR, coreR);

    // Waveform ring (sinusoidal distortion along the circle perimeter)
    constexpr int kSegments = 120;
    const double waveR = radius * 1.35;
    const double waveAmp = radius * 0.12 * amp;

    QPainterPath wavePath;
    for (int i = 0; i <= kSegments; ++i) {
        double angle = 2.0 * M_PI * i / kSegments;
        double offset = waveAmp * std::sin(angle * 8.0 + t * 6.0);
        double rr = waveR + offset;
        double px = centre.x() + rr * std::cos(angle);
        double py = centre.y() + rr * std::sin(angle);
        if (i == 0)
            wavePath.moveTo(px, py);
        else
            wavePath.lineTo(px, py);
    }
    wavePath.closeSubpath();

    QPen wavePen(ringColor_, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(wavePen);
    p.setBrush(Qt::NoBrush);
    p.drawPath(wavePath);
}

} // namespace atlas::ui
