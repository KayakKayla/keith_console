#include "terminal_bridge.h"

#include "config_loader.h"
#include "terminal_session.h"
#include "logger.h"

#include <QDebug>
#include <QStringList>

namespace {
constexpr int kMaxBufferSize = 20000;
}

TerminalBridge::TerminalBridge(QObject *parent)
    : QObject(parent)
    , m_session(std::make_unique<TerminalSession>())
    , m_loader(std::make_unique<ConfigLoader>())
{
    connect(m_session.get(), &TerminalSession::dataReceived, this, [this](const QByteArray &data) {
        appendData(data);
    });
    connect(m_session.get(), &TerminalSession::finished, this, [](int exitCode) {
        qDebug() << "Terminal session finished with code" << exitCode;
    });
    connect(m_loader.get(), &ConfigLoader::configurationChanged, this, [this](const QVariantMap &config) {
        m_config = config;
        if (auto logger = terminalLogger()) {
            logger->info("Configuration reloaded from {}", config.value("_path").toString().toStdString());
        }
        emit configChanged();
        startSession();
    });

    reloadConfig();
}

TerminalBridge::~TerminalBridge() = default;

QString TerminalBridge::buffer() const
{
    return m_buffer;
}

QVariantMap TerminalBridge::config() const
{
    return m_config;
}

void TerminalBridge::sendText(const QString &text)
{
    if (!m_session) {
        return;
    }
    m_session->writeData(text.toUtf8());
}

void TerminalBridge::reloadConfig()
{
    if (!m_loader) {
        return;
    }
    m_loader->load();
}

void TerminalBridge::appendData(const QByteArray &data)
{
    m_buffer.append(QString::fromUtf8(data));
    if (m_buffer.size() > kMaxBufferSize) {
        m_buffer = m_buffer.right(kMaxBufferSize);
    }
    emit bufferChanged();
}

void TerminalBridge::startSession()
{
    const QString command = m_config.value("shell.command", "/bin/sh").toString();
    QStringList args;
    if (command.endsWith("sh")) {
        args << "-l";
    }
    m_session->stop();
    const bool started = m_session->start(command, args);
    if (!started) {
        qWarning() << "Failed to start terminal session";
        if (auto logger = terminalLogger()) {
            logger->error("Failed to start terminal session for {}", command.toStdString());
        }
        return;
    }
    if (auto logger = terminalLogger()) {
        logger->info("Started terminal session using command {}", command.toStdString());
    }
}
