#include "pluginmanager.h"

#include <QDir>
#include <QDesktopServices>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QJsonParseError>
#include <QUrl>

PluginManager::PluginManager(QObject *parent) : QObject(parent) {}

void PluginManager::clearLoadedPlugins()
{
    for (const auto &plugin : m_plugins) {
        if (plugin.loader) {
            plugin.loader->unload();
            delete plugin.loader;
        }
    }
    m_plugins.clear();
    m_scannedFiles.clear();
}

PluginManager::~PluginManager()
{
    clearLoadedPlugins();
}

void PluginManager::scan(const QString &directory)
{
    clearLoadedPlugins();
    m_directory = directory;
    QDir dir(directory);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    for (const auto &file : dir.entryInfoList(QDir::Files)) {
        if (m_scannedFiles.contains(file.absoluteFilePath())) continue;
        const QString suffix = file.suffix().toLower();
        if (suffix == QStringLiteral("json") && file.fileName().endsWith(QStringLiteral(".astraea.json"))) {
            scanManifest(file);
            continue;
        }
        if (suffix == QStringLiteral("exe") || suffix == QStringLiteral("com") || suffix == QStringLiteral("bat") || suffix == QStringLiteral("cmd")) {
            m_plugins.push_back({PluginKind::Executable, nullptr, nullptr, file.absoluteFilePath(), file.baseName(), file.baseName(), QStringLiteral("External application"), file.absoluteFilePath()});
            m_scannedFiles.insert(file.absoluteFilePath());
            continue;
        }
        if (suffix == QStringLiteral("html") || suffix == QStringLiteral("htm") || suffix == QStringLiteral("url")) {
            const PluginKind kind = suffix == QStringLiteral("url") ? PluginKind::Web : PluginKind::Html;
            m_plugins.push_back({kind, nullptr, nullptr, file.absoluteFilePath(), file.baseName(), file.baseName(), QStringLiteral("Local web plugin"), kind == PluginKind::Web ? file.absoluteFilePath() : QUrl::fromLocalFile(file.absoluteFilePath()).toString()});
            m_scannedFiles.insert(file.absoluteFilePath());
            continue;
        }
        if (suffix != QStringLiteral("dll") && suffix != QStringLiteral("so") && suffix != QStringLiteral("dylib")) continue;

        auto *loader = new QPluginLoader(file.absoluteFilePath(), this);
        QObject *object = loader->instance();
        auto *plugin = qobject_cast<Astraea::PluginInterface *>(object);
        if (!plugin) {
            emit errorOccurred(QStringLiteral("Could not load plugin %1: %2").arg(file.fileName(), loader->errorString()));
            loader->unload();
            delete loader;
            continue;
        }
        m_plugins.push_back({PluginKind::NativeQt, loader, plugin, file.absoluteFilePath(), plugin->id(), plugin->name(QLocale()), plugin->description(QLocale()), file.absoluteFilePath()});
        m_scannedFiles.insert(file.absoluteFilePath());
    }
    for (const auto &folder : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QFileInfo manifest(QDir(folder.absoluteFilePath()).filePath(QStringLiteral("plugin.astraea.json")));
        if (manifest.exists() && !m_scannedFiles.contains(manifest.absoluteFilePath())) scanManifest(manifest);
    }
    emit pluginsChanged();
}

void PluginManager::scanManifest(const QFileInfo &file)
{
    QFile manifest(file.absoluteFilePath());
    if (!manifest.open(QIODevice::ReadOnly)) return;
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(manifest.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        emit errorOccurred(QStringLiteral("Invalid plugin manifest: %1").arg(file.fileName()));
        return;
    }
    const QJsonObject object = document.object();
    const QString type = object.value(QStringLiteral("type")).toString().toLower();
    PluginKind kind = PluginKind::Script;
    if (type == QStringLiteral("exe") || type == QStringLiteral("app") || type == QStringLiteral("application")) kind = PluginKind::Executable;
    else if (type == QStringLiteral("command") || type == QStringLiteral("uri")) kind = PluginKind::Command;
    else if (type == QStringLiteral("web") || type == QStringLiteral("url")) kind = PluginKind::Web;
    else if (type == QStringLiteral("html") || type == QStringLiteral("page")) kind = PluginKind::Html;
    else if (type == QStringLiteral("folder") || type == QStringLiteral("directory")) kind = PluginKind::Folder;
    else if (type != QStringLiteral("script")) {
        emit errorOccurred(QStringLiteral("Unsupported plugin type: %1").arg(type));
        return;
    }
    QString entry = object.value(QStringLiteral("entry")).toString();
    if (entry.isEmpty()) {
        emit errorOccurred(QStringLiteral("Missing plugin entry: %1").arg(file.fileName()));
        return;
    }
    if (kind == PluginKind::Web || kind == PluginKind::Command) {
        const QUrl url(entry);
        if (!url.isValid() || url.scheme().isEmpty()) {
            emit errorOccurred(QStringLiteral("Web and URI plugins require a valid URL"));
            return;
        }
    } else if (kind == PluginKind::Folder) {
        entry = QDir(file.absolutePath()).absoluteFilePath(entry);
        if (!QFileInfo(entry).isDir()) {
            emit errorOccurred(QStringLiteral("Plugin folder not found: %1").arg(entry));
            return;
        }
    } else {
        entry = QDir(file.absolutePath()).absoluteFilePath(entry);
        if (!QFileInfo::exists(entry)) {
            emit errorOccurred(QStringLiteral("Plugin entry not found: %1").arg(entry));
            return;
        }
    }
    LoadedPlugin plugin{kind, nullptr, nullptr, file.absoluteFilePath(), object.value(QStringLiteral("id")).toString(file.baseName()), object.value(QStringLiteral("name")).toString(file.baseName()), object.value(QStringLiteral("description")).toString(), entry};
    for (const auto &argument : object.value(QStringLiteral("arguments")).toArray()) plugin.arguments.append(argument.toString());
    plugin.workingDirectory = file.absolutePath();
    if (kind == PluginKind::Script) {
        const QString interpreter = object.value(QStringLiteral("interpreter")).toString();
        if (interpreter.isEmpty()) {
            emit errorOccurred(QStringLiteral("Script plugins require an interpreter"));
            return;
        }
        plugin.arguments.prepend(entry);
        plugin.entry = interpreter;
    }
    m_plugins.push_back(plugin);
    m_scannedFiles.insert(file.absoluteFilePath());
}

void PluginManager::launch(const LoadedPlugin &plugin)
{
    if (plugin.kind == PluginKind::NativeQt) return;
    if (plugin.kind == PluginKind::Executable || plugin.kind == PluginKind::Script) {
        if (!QProcess::startDetached(plugin.entry, plugin.arguments, plugin.workingDirectory))
            emit errorOccurred(QStringLiteral("Failed to start: %1").arg(plugin.entry));
    } else {
        const QUrl url = (plugin.kind == PluginKind::Web || plugin.kind == PluginKind::Command) ? QUrl(plugin.entry) : QUrl::fromLocalFile(plugin.entry);
        if (!QDesktopServices::openUrl(url)) emit errorOccurred(QStringLiteral("Failed to open: %1").arg(plugin.entry));
    }
}
