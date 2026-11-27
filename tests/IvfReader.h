#pragma once

#include <QByteArray>
#include <QFile>
#include <QList>
#include <QString>
#include <QtEndian>

#include <optional>

// Frame payloads of an IVF file, see tests/resources/video/README.md
inline std::optional<QList<QByteArray>> readIvfFrames(const QString& path) {
    constexpr qsizetype FileHeaderSize = 32;
    constexpr qsizetype FrameHeaderSize = 12;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }

    const QByteArray content = file.readAll();
    if (!content.startsWith("DKIF")) {
        return std::nullopt;
    }

    QList<QByteArray> frames;
    qsizetype offset = FileHeaderSize;
    while (offset + FrameHeaderSize <= content.size()) {
        const auto frameSize = static_cast<qsizetype>(qFromLittleEndian<quint32>(content.constData() + offset));
        offset += FrameHeaderSize;
        if (offset + frameSize > content.size()) {
            return std::nullopt;
        }
        frames.append(content.sliced(offset, frameSize));
        offset += frameSize;
    }
    return frames;
}
