#pragma once

#include <QObject>

struct PluginInfo
{
    QString binPath;
    QString displayName;
    QString version;
    QString type;
};

class PluginManager : public QObject
{
    Q_OBJECT

public:
    static PluginManager &instance()
    {
        static PluginManager *_instance = nullptr;
        if (!_instance) {
            _instance = new PluginManager;
        }
        return *_instance;
    };

    ~PluginManager();

    const PluginInfo *pluginByType(const QString &type);

private:
    explicit PluginManager(QObject *parent = nullptr);
    void init();

    bool m_isInitialized = false;
    QList<PluginInfo *> m_pluginInfos;
};
