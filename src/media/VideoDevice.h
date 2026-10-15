#pragma once

#include <QMetaType>
#include <QString>
#include <qqmlintegration.h>

struct VideoDevice
{
    Q_GADGET
    QML_VALUE_TYPE(videoDevice)

    Q_PROPERTY(QString id MEMBER id CONSTANT)
    Q_PROPERTY(QString description MEMBER description CONSTANT)
    Q_PROPERTY(bool isDefault MEMBER isDefault CONSTANT)

public:
    QString id;
    QString description;
    bool isDefault = false;

    bool isNull() const { return id.isEmpty(); }
    bool operator==(const VideoDevice &other) const { return id == other.id; }
};

Q_DECLARE_METATYPE(VideoDevice)

class VideoDeviceListRegistration
{
    Q_GADGET
    QML_FOREIGN(QList<VideoDevice>)
    QML_ANONYMOUS
    QML_SEQUENTIAL_CONTAINER(VideoDevice)
};
