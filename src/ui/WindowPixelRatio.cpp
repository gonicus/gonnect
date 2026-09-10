#include "WindowPixelRatio.h"

#include <QEvent>

WindowPixelRatio::WindowPixelRatio(QObject *parent) : QObject{ parent } { }

int WindowPixelRatio::quantumFor(qreal ratio)
{
    constexpr int maxDenominator = 16;

    for (int n = 1; n <= maxDenominator; ++n) {
        const qreal devicePixels = n * ratio;

        if (qAbs(devicePixels - qRound(devicePixels)) < 0.0001) {
            return n;
        }
    }

    return 1;
}

void WindowPixelRatio::setWindow(QWindow *window)
{
    if (m_window == window) {
        return;
    }

    if (m_window) {
        m_window->removeEventFilter(this);
    }

    m_window = window;

    if (m_window) {
        m_window->installEventFilter(this);
    }

    Q_EMIT windowChanged();
    update();
}

bool WindowPixelRatio::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window && event->type() == QEvent::DevicePixelRatioChange) {
        update();
    }

    return QObject::eventFilter(watched, event);
}

void WindowPixelRatio::update()
{
    const qreal ratio = m_window ? m_window->devicePixelRatio() : 1.0;

    if (qFuzzyCompare(ratio, m_ratio)) {
        return;
    }

    m_ratio = ratio;
    m_quantum = quantumFor(ratio);

    Q_EMIT ratioChanged();
}
