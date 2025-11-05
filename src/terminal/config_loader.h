#ifndef TERMINAL_CONFIG_LOADER_H
#define TERMINAL_CONFIG_LOADER_H

#include <QFileSystemWatcher>
#include <QObject>
#include <QVariantMap>

#include <memory>

class ConfigLoader : public QObject
{
    Q_OBJECT

public:
    explicit ConfigLoader(QObject *parent = nullptr);

    QVariantMap load(const QString &overridePath = QString());

signals:
    void configurationChanged(const QVariantMap &config);

private:
    QVariantMap parseToml(const QByteArray &data) const;
    void watchFile(const QString &path);

    QString m_defaultPath;
    std::unique_ptr<QFileSystemWatcher> m_watcher;
    QString m_activePath;
};
#endif
