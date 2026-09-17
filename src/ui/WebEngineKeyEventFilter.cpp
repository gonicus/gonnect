#include "WebEngineKeyEventFilter.h"
#include <QKeyEvent>

WebEngineKeyEventFilter::WebEngineKeyEventFilter(QObject *parent) : QObject{ parent } { }

bool WebEngineKeyEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        const auto *keyEvent = static_cast<QKeyEvent *>(event);

        if (obj->metaObject()->className()
            == QStringLiteral("QtWebEngineCore::RenderWidgetHostViewQtDelegateItem")) {

            const auto key = keyEvent->key();
            const bool ctrlPressed = keyEvent->modifiers().testFlag(Qt::ControlModifier);
            const bool shiftPressed = keyEvent->modifiers().testFlag(Qt::ShiftModifier);

            if (key == Qt::Key_F11) {
                Q_EMIT f11Pressed();
            } else if (key == Qt::Key_Escape) {
                Q_EMIT escapePressed();
            } else if (key == Qt::Key_F && ctrlPressed) {
                Q_EMIT ctrlFPressed();
            } else if (key == Qt::Key_K && ctrlPressed) {
                Q_EMIT ctrlFPressed();
            } else if (key == Qt::Key_M && ctrlPressed && shiftPressed) {
                Q_EMIT ctrlShiftMPressed();
            }

            return false;
        }
    }

    return QObject::eventFilter(obj, event);
}
