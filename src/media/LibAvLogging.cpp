#include "LibAvLogging.h"

#include <QLoggingCategory>
#include <QString>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
}

#include <array>
#include <cstdarg>
#include <mutex>

Q_LOGGING_CATEGORY(lcLibAv, "gonnect.media.libav")

namespace {

void logMessage(void *context, int level, const char *format, va_list arguments)
{
    if (level > AV_LOG_VERBOSE) {
        return;
    }

    // ffmpeg tracks whether next line needs a prefix
    // Callers run in several therads
    thread_local int shouldPrintPrefix = 1;

    std::array<char, 1024> line{};
    av_log_format_line2(context, level, format, arguments, line.data(),
                        static_cast<int>(line.size()), &shouldPrintPrefix);

    const auto message = QString::fromUtf8(line.data()).trimmed();
    if (message.isEmpty()) {
        return;
    }

    if (level <= AV_LOG_ERROR) {
        qCWarning(lcLibAv).noquote() << message;
    } else if (level <= AV_LOG_WARNING) {
        qCInfo(lcLibAv).noquote() << message;
    } else {
        qCDebug(lcLibAv).noquote() << message;
    }
}

} // namespace

void LibAvLogging::install()
{
    static std::once_flag isInstalled;
    std::call_once(isInstalled, []() {
        av_log_set_callback(logMessage);
        qCDebug(lcLibAv) << LIBAVCODEC_IDENT << "configured with" << avcodec_configuration();
    });
}
