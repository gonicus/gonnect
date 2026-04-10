#pragma once
#include <QObject>
#include "../NetworkHelper.h"

class WindowsNetworkHelper : public NetworkHelper
{
    Q_OBJECT
    Q_DISABLE_COPY(WindowsNetworkHelper)

public:
    explicit WindowsNetworkHelper();
    ~WindowsNetworkHelper() = default;

    QStringList nameservers() const override;
};
