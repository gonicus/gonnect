#pragma once

#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QWindow>

/**
 * On Linux, this informs the window manager about the window shadow that is drawn in case of
 * client side window decoration.
 */
class WindowShadowMargins : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged FINAL)
    Q_PROPERTY(int margin READ margin WRITE setMargin NOTIFY marginChanged FINAL)

public:
    explicit WindowShadowMargins(QObject *parent = nullptr);

    QWindow *window() const { return m_window; }
    void setWindow(QWindow *window);

    int margin() const { return m_margin; }
    void setMargin(int margin);

Q_SIGNALS:
    void windowChanged();
    void marginChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void apply();
    void applyWayland(int margin);
    void applyX11(int margin);

    QPointer<QWindow> m_window;
    int m_margin = 0;

    // Currently applied window margin, -1 if not set yet
    int m_appliedMargin = -1;
};
