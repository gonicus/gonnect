#pragma once

#include <QObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcShortcuts)

struct ShortcutItem
{
    QString id;
    QString description;
    QString triggerDescription;
};

using Shortcut = QPair<QString, QVariantMap>;
using Shortcuts = QList<Shortcut>;

class GlobalShortcuts : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(GlobalShortcuts)

public:
    Q_REQUIRED_RESULT static GlobalShortcuts &instance();

    explicit GlobalShortcuts(QObject *parent = nullptr) : QObject(parent) { }
    ~GlobalShortcuts() = default;

    virtual bool isSupported() const = 0;
    virtual void setShortcuts(Shortcuts &shortcuts) = 0;
    virtual QList<ShortcutItem *> shortcuts() const = 0;

Q_SIGNALS:
    void initialized();
    void shortcutsChanged();
    void activated(QString id);
};
