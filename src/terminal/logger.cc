#include "logger.h"

#include <QDir>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

namespace {
std::shared_ptr<spdlog::logger> g_logger;
constexpr size_t kMaxLogSize = 5 * 1024 * 1024;
constexpr size_t kMaxRotations = 3;
}

void initLogger(const QString &logDirectory)
{
    QDir dir(logDirectory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    const QString logPath = dir.filePath("keith_console.log");
    auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logPath.toStdString(), kMaxLogSize, kMaxRotations);
    g_logger = std::make_shared<spdlog::logger>("keith_console", sink);
    g_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_default_logger(g_logger);
}

std::shared_ptr<spdlog::logger> terminalLogger()
{
    return g_logger;
}
