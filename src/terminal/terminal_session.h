#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <QStringList>

#include <memory>

class TerminalSession : public QObject
{
    Q_OBJECT

public:
    explicit TerminalSession(QObject *parent = nullptr);
    ~TerminalSession() override;

    bool start(const QString &command, const QStringList &arguments = {});
    void writeData(const QByteArray &data);
    void resize(int columns, int rows);
    void stop();

signals:
    void dataReceived(const QByteArray &data);
    void finished(int exitCode);

private slots:
    void handleReadyRead();
    void handleChildExit();

private:
    void closePty();

    int m_masterFd = -1;
    pid_t m_childPid = -1;
    std::unique_ptr<QSocketNotifier> m_readNotifier;
};
