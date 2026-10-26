#include "WindowShadowMargins.h"

#include <QGuiApplication>
#include <QPlatformSurfaceEvent>

#if QT_CONFIG(wayland)
#  include <QtGui/qpa/qplatformwindow_p.h>
#endif

#if QT_CONFIG(xcb)
#  include <QtGui/qguiapplication_platform.h>
#  include <xcb/xcb.h>
#endif

WindowShadowMargins::WindowShadowMargins(QObject *parent) : QObject{ parent } { }

void WindowShadowMargins::setWindow(QWindow *window)
{
    if (m_window == window) {
        return;
    }

    if (m_window) {
        m_window->removeEventFilter(this);
    }

    m_window = window;
    m_appliedMargin = -1;

    if (m_window) {
        m_window->installEventFilter(this);
    }

    Q_EMIT windowChanged();
    apply();
}

void WindowShadowMargins::setMargin(int margin)
{
    if (m_margin == margin) {
        return;
    }

    m_margin = margin;

    Q_EMIT marginChanged();
    apply();
}

bool WindowShadowMargins::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window && event->type() == QEvent::DevicePixelRatioChange
        && m_appliedMargin >= 0 && QGuiApplication::platformName() == "xcb") {
        applyX11(m_appliedMargin);
    } else if (watched == m_window && event->type() == QEvent::PlatformSurface) {
        switch (static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType()) {
        case QPlatformSurfaceEvent::SurfaceCreated:
            apply();
            break;
        case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
            m_appliedMargin = -1;
            break;
        }
    }

    return QObject::eventFilter(watched, event);
}

void WindowShadowMargins::apply()
{
    if (!m_window || !m_window->handle() || m_appliedMargin == m_margin) {
        return;
    }

    const auto platform = QGuiApplication::platformName();

    if (platform == "wayland") {
        applyWayland(m_margin);
    } else if (platform == "xcb") {
        applyX11(m_margin);
    }

    m_appliedMargin = m_margin;
}

void WindowShadowMargins::applyWayland(int margin)
{
#if QT_CONFIG(wayland)
    auto waylandWindow = m_window->nativeInterface<QNativeInterface::Private::QWaylandWindow>();
    if (!waylandWindow) {
        return;
    }

    const QSize size = m_window->size();
    waylandWindow->setCustomMargins(QMargins(margin, margin, margin, margin));
    if (m_appliedMargin < 0) {
        m_window->resize(size);
    }
#else
    Q_UNUSED(margin)
#endif
}

void WindowShadowMargins::applyX11(int margin)
{
#if QT_CONFIG(xcb)
    auto x11App = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!x11App) {
        return;
    }

    xcb_connection_t *connection = x11App->connection();

    static constexpr char atomName[] = "_GTK_FRAME_EXTENTS";
    auto cookie = xcb_intern_atom(connection, false, sizeof(atomName) - 1, atomName);
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, nullptr);
    if (!reply) {
        return;
    }
    const xcb_atom_t atom = reply->atom;
    free(reply);

    const auto window = static_cast<xcb_window_t>(m_window->winId());

    if (margin > 0) {
        const auto extent = static_cast<uint32_t>(qRound(margin * m_window->devicePixelRatio()));
        const uint32_t extents[4] = { extent, extent, extent, extent };
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, atom, XCB_ATOM_CARDINAL, 32,
                            4, extents);
    } else {
        xcb_delete_property(connection, window, atom);
    }
    xcb_flush(connection);
#else
    Q_UNUSED(margin)
#endif
}
