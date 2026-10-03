#pragma once

#include "../plugins/plugininterface.h"

#include <QPluginLoader>
#include <QObject>
#include <QVector>
#include <QProcess>
#include <QFileInfo>
#include <QSet>

enum class PluginKind {
    NativeQt,
    Executable,
    Command,
    Web,
    Html,
    Folder,
    Script
};

struct LoadedPlugin {
    PluginKind kind = PluginKind::NativeQt;
    QPluginLoader *loader = nullptr;
    Astraea::PluginInterface *instance = nullptr;
    QString filePath;
    QString id;
    QString displayName;
    QString description;
    QString entry;
    QStringList arguments;
    QString workingDirectory;
};

class PluginManager final : public QObject
{
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);
    ~PluginManager() override;

    void scan(const QString &directory);
    const QVector<LoadedPlugin> &plugins() const { return m_plugins; }
    QString directory() const { return m_directory; }
    void launch(const LoadedPlugin &plugin);

signals:
    void pluginsChanged();
    void errorOccurred(const QString &message);

private:
    void clearLoadedPlugins();
    void scanManifest(const QFileInfo &file);
    QString m_directory;
    QVector<LoadedPlugin> m_plugins;
    QSet<QString> m_scannedFiles;
};
