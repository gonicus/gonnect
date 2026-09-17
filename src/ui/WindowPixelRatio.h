#pragma once

#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QWindow>

/**
 * Exposes the device pixel ratio of a window and the pixel grid derived from it to QML.
 */
class WindowPixelRatio : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged FINAL)
    Q_PROPERTY(qreal ratio READ ratio NOTIFY ratioChanged FINAL)
    Q_PROPERTY(int quantum READ quantum NOTIFY ratioChanged FINAL)

public:
    explicit WindowPixelRatio(QObject *parent = nullptr);

    QWindow *window() const { return m_window; }
    void setWindow(QWindow *window);

    qreal ratio() const { return m_ratio; }

    // Smallest number of whole logical pixels that covers a whole number of device pixels
    int quantum() const { return m_quantum; }

Q_SIGNALS:
    void windowChanged();
    void ratioChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // Number of logical pixels after which the device pixel grid lines up again
    static int quantumFor(qreal ratio);

    void update();

    QPointer<QWindow> m_window;
    qreal m_ratio = 1.0;
    int m_quantum = 1;
};
