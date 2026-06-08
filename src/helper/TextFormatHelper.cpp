#include "TextFormatHelper.h"

TextFormatHelper::TextFormatHelper(QObject *parent) : QObject{ parent } { }

QString TextFormatHelper::formatFileSize(qint64 byteSize) const
{
    if (byteSize <= 0) {
        return tr("0 B");
    }

    static const QStringList units = { tr("B"), tr("KB"), tr("MB"), tr("GB"), tr("TB"), tr("PB") };

    quint8 i = static_cast<quint8>(std::floor(std::log(byteSize) / std::log(1024)));
    i = std::clamp<quint8>(i, 0, units.length() - 1);

    const qreal value = byteSize / std::pow(1024, i);

    return QString("%1 %2").arg(QLocale().toString(value, 'f', i == 0 ? 0 : 2), units.at(i));
}
