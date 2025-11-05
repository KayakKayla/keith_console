#include "terminal_session.h"

#include <QCoreApplication>
#include <QSocketNotifier>
#include <QTimer>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#if defined(Q_OS_MACOS)
#include <util.h>
#else
#include <pty.h>
#endif
#include <signal.h>
#include <unistd.h>

#include <vector>

namespace {
constexpr int kDefaultColumns = 80;
constexpr int kDefaultRows = 24;
} // namespace

TerminalSession::TerminalSession(QObject *parent)
    : QObject(parent)
{
}

TerminalSession::~TerminalSession()
{
    closePty();
}

bool TerminalSession::start(const QString &command, const QStringList &arguments)
{
    if (m_childPid > 0) {
        return false;
    }

    struct winsize ws {};
    ws.ws_col = kDefaultColumns;
    ws.ws_row = kDefaultRows;

    int masterFd = -1;
    pid_t pid = forkpty(&masterFd, nullptr, nullptr, &ws);
    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        QStringList args = arguments;
        if (args.isEmpty()) {
            args << "-l";
        }
        QByteArray cmd = command.toLocal8Bit();
        QList<QByteArray> argArray;
        argArray.reserve(args.size() + 1);
        argArray << cmd;
        for (const QString &arg : args) {
            argArray << arg.toLocal8Bit();
        }
        std::vector<char *> argv(argArray.size() + 1, nullptr);
        for (int i = 0; i < argArray.size(); ++i) {
            argv[i] = argArray[i].data();
        }
        ::execvp(cmd.constData(), argv.data());
        _exit(127);
    }

    m_childPid = pid;
    m_masterFd = masterFd;

    m_readNotifier = std::make_unique<QSocketNotifier>(m_masterFd, QSocketNotifier::Read, this);
    connect(m_readNotifier.get(), &QSocketNotifier::activated, this, &TerminalSession::handleReadyRead);

    return true;
}

void TerminalSession::writeData(const QByteArray &data)
{
    if (m_masterFd < 0) {
        return;
    }
    ::write(m_masterFd, data.constData(), static_cast<size_t>(data.size()));
}

void TerminalSession::resize(int columns, int rows)
{
    if (m_masterFd < 0) {
        return;
    }
    struct winsize ws {};
    ws.ws_col = columns;
    ws.ws_row = rows;
    ::ioctl(m_masterFd, TIOCSWINSZ, &ws);
}

void TerminalSession::handleReadyRead()
{
    if (m_masterFd < 0) {
        return;
    }

    QByteArray buffer(4096, Qt::Uninitialized);
    const ssize_t bytesRead = ::read(m_masterFd, buffer.data(), static_cast<size_t>(buffer.size()));
    if (bytesRead <= 0) {
        handleChildExit();
        return;
    }
    buffer.resize(static_cast<int>(bytesRead));
    emit dataReceived(buffer);
}

void TerminalSession::handleChildExit()
{
    if (m_childPid <= 0) {
        return;
    }

    int status = 0;
    pid_t result = ::waitpid(m_childPid, &status, WNOHANG);
    if (result == 0) {
        return;
    }

    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    emit finished(exitCode);
    closePty();
}

void TerminalSession::stop()
{
    closePty();
}

void TerminalSession::closePty()
{
    if (m_readNotifier) {
        m_readNotifier->setEnabled(false);
        m_readNotifier.reset();
    }
    if (m_masterFd >= 0) {
        ::close(m_masterFd);
        m_masterFd = -1;
    }
    if (m_childPid > 0) {
        ::kill(m_childPid, SIGKILL);
        ::waitpid(m_childPid, nullptr, 0);
        m_childPid = -1;
    }
}
