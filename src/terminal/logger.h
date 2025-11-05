#pragma once

#include <memory>

#include <QString>

#include <spdlog/spdlog.h>

void initLogger(const QString &logDirectory);
std::shared_ptr<spdlog::logger> terminalLogger();
