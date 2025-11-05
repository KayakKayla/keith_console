#include "config_loader.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QStringList>

namespace {
QString locateConfigFile(const QString &overridePath)
{
    if (!overridePath.isEmpty()) {
        return overridePath;
    }

    const QString customPath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!customPath.isEmpty()) {
        const QString candidate = QDir(customPath).filePath("default.toml");
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }

    return QDir(QCoreApplication::applicationDirPath())
        .filePath("../config/default.toml");
}

QVariant convertValue(const QString &value)
{
    if (value.compare("true", Qt::CaseInsensitive) == 0) {
        return true;
    }
    if (value.compare("false", Qt::CaseInsensitive) == 0) {
        return false;
    }

    bool ok = false;
    int intValue = value.toInt(&ok);
    if (ok) {
        return intValue;
    }

    double doubleValue = value.toDouble(&ok);
    if (ok) {
        return doubleValue;
    }

    QString str = value.trimmed();
    if (str.startsWith('"') && str.endsWith('"') && str.size() >= 2) {
        str = str.mid(1, str.size() - 2);
    }
    return str;
}
} // namespace

ConfigLoader::ConfigLoader(QObject *parent)
    : QObject(parent)
{
    m_defaultPath = locateConfigFile({});
}

QVariantMap ConfigLoader::load(const QString &overridePath)
{
    const QString path = locateConfigFile(overridePath.isEmpty() ? m_defaultPath : overridePath);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    const QByteArray data = file.readAll();
    QVariantMap config = parseToml(data);
    config.insert("_path", path);
    watchFile(path);
    m_activePath = path;
    emit configurationChanged(config);
    return config;
}

QVariantMap ConfigLoader::parseToml(const QByteArray &data) const
{
    QVariantMap config;

    const QList<QByteArray> lines = data.split('\n');
    for (QByteArray lineBytes : lines) {
        QString line = QString::fromUtf8(lineBytes).trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        const auto parts = line.split('=');
        if (parts.size() < 2) {
            continue;
        }

        const QString key = parts.first().trimmed();
        const QString value = parts.mid(1).join("=").trimmed();
        config.insert(key, convertValue(value));
    }

    return config;
}

void ConfigLoader::watchFile(const QString &path)
{
    if (!m_watcher) {
        m_watcher = std::make_unique<QFileSystemWatcher>(this);
        connect(m_watcher.get(), &QFileSystemWatcher::fileChanged, this, [this](const QString &changedPath) {
            if (changedPath == m_activePath) {
                load(m_activePath);
            }
        });
    }

    if (!m_watcher->files().contains(path)) {
        m_watcher->addPath(path);
    }
}
