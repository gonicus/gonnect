#pragma once

#include <QAbstractListModel>
#include <qqmlintegration.h>

#include "JitsiConnector.h"

class JitsiMediaDevice;

class JitsiDevicesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_CLASSINFO("DefaultProperty", "jitsiConnector")

    Q_PROPERTY(JitsiConnector *jitsiConnector MEMBER m_jitsiConnector NOTIFY jitsiConnectorChanged
                       FINAL)
    Q_PROPERTY(JitsiDevicesModel::DeviceTypes typeFilter MEMBER m_typeFilter NOTIFY
                       typeFilterChanged FINAL)

public:
    enum class DeviceType {
        Unknown = 0,
        AudioInput = 1 << 0,
        AudioOutput = 1 << 1,
        VideoInput = 1 << 2
    };
    Q_ENUM(DeviceType)
    Q_DECLARE_FLAGS(DeviceTypes, DeviceType)
    Q_FLAG(DeviceTypes)

    enum class Roles { DeviceId = Qt::UserRole + 1, GroupId, Label, Type };

    explicit JitsiDevicesModel(QObject *parent = nullptr);

    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

private:
    JitsiConnector *m_jitsiConnector = nullptr;
    QObject *m_jitsiConnectorContext = nullptr;
    JitsiDevicesModel::DeviceTypes m_typeFilter = DeviceType::Unknown;
    QList<JitsiMediaDevice *> m_devices;

private slots:
    void rebuildModel();

signals:
    void jitsiConnectorChanged();
    void typeFilterChanged();
};
