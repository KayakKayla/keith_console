#pragma once

#include <QObject>
#include <QVariantMap>

#include <memory>

class TerminalSession;
class ConfigLoader;

class TerminalBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString buffer READ buffer NOTIFY bufferChanged)
    Q_PROPERTY(QVariantMap config READ config NOTIFY configChanged)

public:
    explicit TerminalBridge(QObject *parent = nullptr);
    ~TerminalBridge() override;

    QString buffer() const;
    QVariantMap config() const;

    Q_INVOKABLE void sendText(const QString &text);
    Q_INVOKABLE void reloadConfig();

signals:
    void bufferChanged();
    void configChanged();

private:
    void appendData(const QByteArray &data);
    void startSession();

    QString m_buffer;
    QVariantMap m_config;
    std::unique_ptr<TerminalSession> m_session;
    std::unique_ptr<ConfigLoader> m_loader;
};
