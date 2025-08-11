#include "JitsiDevicesModel.h"
#include "JitsiConnector.h"
#include "JitsiMediaDevice.h"

JitsiDevicesModel::JitsiDevicesModel(QObject *parent) : QAbstractListModel{ parent }
{

    connect(this, &JitsiDevicesModel::typeFilterChanged, this, &JitsiDevicesModel::rebuildModel);
    connect(this, &JitsiDevicesModel::jitsiConnectorChanged, this, [this]() {
        if (m_jitsiConnectorContext) {
            m_jitsiConnectorContext->deleteLater();
            m_jitsiConnectorContext = nullptr;
        }

        if (m_jitsiConnector) {
            m_jitsiConnectorContext = new QObject(this);
            connect(m_jitsiConnector, &JitsiConnector::endedJitsiDevicesReset,
                    m_jitsiConnectorContext, [this]() { rebuildModel(); });
        }

        rebuildModel();
    });
}

QHash<int, QByteArray> JitsiDevicesModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::DeviceId), "deviceId" },
        { static_cast<int>(Roles::GroupId), "groupId" },
        { static_cast<int>(Roles::Label), "label" },
        { static_cast<int>(Roles::Type), "type" },
    };
}

void JitsiDevicesModel::rebuildModel()
{
    beginResetModel();
    m_devices.clear();

    if (m_jitsiConnector) {

        if (m_typeFilter & DeviceType::AudioInput) {
            const auto audioInputDevices = m_jitsiConnector->audioInputDevices();
            m_devices += audioInputDevices;
        }
        if (m_typeFilter & DeviceType::AudioOutput) {
            const auto audioOutputDevices = m_jitsiConnector->audioOutputDevices();
            m_devices += audioOutputDevices;
        }
        if (m_typeFilter & DeviceType::VideoInput) {
            const auto videoInputDevices = m_jitsiConnector->videoInputDevices();
            m_devices += videoInputDevices;
        }
    }

    endResetModel();
}

int JitsiDevicesModel::rowCount(const QModelIndex &) const
{
    return m_devices.size();
}

QVariant JitsiDevicesModel::data(const QModelIndex &index, int role) const
{
    const auto device = q_check_ptr(m_devices.at(index.row()));

    switch (role) {
    case static_cast<int>(Roles::DeviceId):
        return device->deviceId();

    case static_cast<int>(Roles::GroupId):
        return device->groupId();

    case static_cast<int>(Roles::Type):
        return static_cast<int>(device->type());

    case static_cast<int>(Roles::Label):
    default:
        return device->label();
    }
}
