#include "msfsmanager.h"
#include "../scancache.h"

#include <QAbstractItemView>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QProcess>
#include <QPushButton>
#include <QGroupBox>
#include <QStandardPaths>
#include <QSettings>
#include <QTextStream>
#include <QVBoxLayout>
#include <QThread>
#include <QtConcurrent>

namespace {
QString readInstalledPackagesPath(const QString &configPath)
{
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (!line.startsWith(QStringLiteral("InstalledPackagesPath"), Qt::CaseInsensitive)) continue;
        QString value = line.section(QLatin1Char(' '), 1).trimmed();
        if (value.isEmpty()) value = line.section(QLatin1Char('='), 1).trimmed();
        value.remove(QLatin1Char('"'));
        return QDir::fromNativeSeparators(value);
    }
    return {};
}

qint64 directorySize(const QString &path)
{
    qint64 total = 0;
    QDirIterator iterator(path, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) { iterator.next(); total += iterator.fileInfo().size(); }
    return total;
}

QString formatBytes(qint64 bytes)
{
    if (bytes < 1024) return QStringLiteral("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024 * 1024 * 1024) return QStringLiteral("%1 MB").arg(bytes / 1024.0 / 1024.0, 0, 'f', 1);
    return QStringLiteral("%1 GB").arg(bytes / 1024.0 / 1024.0 / 1024.0, 0, 'f', 2);
}

QString categoryFor(const QString &path)
{
    bool livery = false;
    bool scenery = false;
    bool aircraft = false;
    bool airport = false;
    bool library = false;
    bool utility = false;
    QDirIterator iterator(path, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        if (iterator.fileInfo().isDir()) continue;
        const QString file = iterator.fileName().toLower();
        const QString relative = QDir(path).relativeFilePath(iterator.filePath()).toLower();
        if (file == QStringLiteral("livery.cfg")) livery = true;
        if (relative.contains(QStringLiteral("scenery")) || relative.contains(QStringLiteral("world")) || relative.contains(QStringLiteral("airport"))) scenery = true;
        if (relative.contains(QStringLiteral("simobjects/airplanes")) || relative.contains(QStringLiteral("simobjects\\airplanes")) || file == QStringLiteral("aircraft.cfg")) aircraft = true;
        if (relative.contains(QStringLiteral("airport")) || relative.contains(QStringLiteral("navdata")) || file == QStringLiteral("airport.xml")) airport = true;
        if (relative.contains(QStringLiteral("texture")) || relative.contains(QStringLiteral("effects")) || relative.contains(QStringLiteral("fonts"))) library = true;
        if (file == QStringLiteral("manifest.json") || file == QStringLiteral("layout.json") || file == QStringLiteral("community")) utility = true;
    }
    if (livery) return QStringLiteral("涂装");
    if (aircraft) return QStringLiteral("机模");
    if (airport) return QStringLiteral("机场/环境");
    if (scenery) return QStringLiteral("地景");
    if (library) return QStringLiteral("库/资源");
    if (utility) return QStringLiteral("工具/组件");
    return QStringLiteral("其他");
}

QString actionStyle()
{
    return QStringLiteral("QPushButton#pluginAction { color: #e8edf5; background: #283b58; border: 1px solid #49688f; border-radius: 6px; padding: 5px 10px; } QPushButton#pluginAction:hover { background: #395578; color: white; }");
}
}

QString MsfsManagerPlugin::id() const { return QStringLiteral("astraea.msfs2024.manager"); }
QString MsfsManagerPlugin::name(const QLocale &locale) const { return locale.language() == QLocale::Chinese ? QStringLiteral("MSFS 2024 插件管理器") : QStringLiteral("MSFS 2024 Add-on Manager"); }
QString MsfsManagerPlugin::description(const QLocale &locale) const { return locale.language() == QLocale::Chinese ? QStringLiteral("扫描、分类并管理 MSFS 2024 Community 插件。") : QStringLiteral("Scan, classify, and manage MSFS 2024 Community add-ons."); }
QWidget *MsfsManagerPlugin::createWidget(QWidget *parent) { return new MsfsManagerWidget(parent); }

MsfsManagerWidget::MsfsManagerWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->addWidget(new QLabel(QStringLiteral("管理 Community 内容：分类、启用/禁用和打开插件位置。"), this));

    auto *pathRow = new QHBoxLayout;
    m_communityPath = new QLineEdit(this);
    m_communityPath->setPlaceholderText(QStringLiteral("Community 文件夹路径"));
    auto *browse = new QPushButton(QStringLiteral("选择"), this);
    pathRow->addWidget(m_communityPath, 1);
    pathRow->addWidget(browse);
    layout->addLayout(pathRow);

    auto *box = new QGroupBox(QStringLiteral("Community 插件管理"), this);
    auto *boxLayout = new QVBoxLayout(box);
    auto *actions = new QHBoxLayout;
    m_scanButton = new QPushButton(QStringLiteral("扫描 Community"), box);
    auto *open = new QPushButton(QStringLiteral("打开 Community"), box);
    actions->addWidget(m_scanButton);
    actions->addWidget(open);
    actions->addStretch();
    boxLayout->addLayout(actions);
    m_progress = new QProgressBar(box);
    m_progress->setTextVisible(true);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    boxLayout->addWidget(m_progress);
    m_summary = new QLabel(box);
    boxLayout->addWidget(m_summary);
    m_plugins = new QListWidget(box);
    m_plugins->setSelectionMode(QAbstractItemView::SingleSelection);
    boxLayout->addWidget(m_plugins, 1);
    layout->addWidget(box, 1);
    m_status = new QLabel(this);
    layout->addWidget(m_status);

    connect(browse, &QPushButton::clicked, this, &MsfsManagerWidget::chooseCommunity);
    connect(m_scanButton, &QPushButton::clicked, this, &MsfsManagerWidget::scanCommunity);
    connect(open, &QPushButton::clicked, this, &MsfsManagerWidget::openCommunity);
    m_communityPath->setText(QSettings().value(QStringLiteral("msfs/communityPath"), detectCommunity()).toString());
    startScan(false);
}

QString MsfsManagerWidget::detectCommunity() const
{
    const QStringList configs = {
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(QStringLiteral("../Microsoft Flight Simulator 2024/UserCfg.opt")),
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(QStringLiteral("../Microsoft Flight Simulator/UserCfg.opt")),
        QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath(QStringLiteral("Packages/Microsoft.Limitless_8wekyb3d8bbwe/LocalCache/UserCfg.opt"))
    };
    for (const QString &config : configs) {
        const QString installed = readInstalledPackagesPath(QDir::cleanPath(config));
        const QString community = QDir(installed).filePath(QStringLiteral("Community"));
        if (!installed.isEmpty() && QDir(community).exists()) return QDir::toNativeSeparators(community);
    }
    return {};
}

void MsfsManagerWidget::chooseCommunity()
{
    const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("选择 MSFS 2024 Community 文件夹"), m_communityPath->text());
    if (!path.isEmpty()) { m_communityPath->setText(path); QSettings().setValue(QStringLiteral("msfs/communityPath"), path); scanCommunity(); }
}

void MsfsManagerWidget::scanCommunity()
{
    startScan(true);
}

void MsfsManagerWidget::startScan(bool fullScan)
{
    m_plugins->clear();
    const int generation = ++m_scanGeneration;
    const QString input = m_communityPath->text().trimmed();
    const QString path = input.isEmpty() ? QString() : QDir(input).absolutePath();
    if (path.isEmpty() || !QDir(path).exists()) {
        m_summary->setText(path.isEmpty() ? QStringLiteral("尚未设置 Community 文件夹，插件列表为空。") : QStringLiteral("Community 文件夹无效，插件列表为空。"));
        setScanBusy(false);
        return;
    }
    setScanBusy(true);
    QSettings().setValue(QStringLiteral("msfs/communityPath"), path);
    const QString cachePath = ScanCache::filePath(QStringLiteral("community"), path);
    auto *watcher = new QFutureWatcher<ScanCache::Result<MsfsCommunityEntry>>(this);
    m_scanWatcher = watcher;
    connect(watcher, &QFutureWatcher<ScanCache::Result<MsfsCommunityEntry>>::finished, this, &MsfsManagerWidget::scanFinished);
    watcher->setProperty("generation", generation);
    watcher->setProperty("cachePath", cachePath);
    watcher->setFuture(QtConcurrent::run([path, cachePath, fullScan] {
        ScanCache::Result<MsfsCommunityEntry> result;
        const QJsonObject cached = fullScan ? QJsonObject() : ScanCache::load(cachePath);
        const QFileInfoList entries = QDir(path).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
        result.entries.reserve(entries.size());
        for (const QFileInfo &entry : entries) {
            MsfsCommunityEntry item;
            item.path = entry.absoluteFilePath();
            item.name = entry.fileName();
            item.disabled = item.name.endsWith(QStringLiteral(".disabled"), Qt::CaseInsensitive);
            const QJsonObject previous = cached.value(item.name).toObject();
            if (previous.contains(QStringLiteral("category")) && previous.contains(QStringLiteral("size"))) {
                item.size = previous.value(QStringLiteral("size")).toString().toLongLong();
                item.category = previous.value(QStringLiteral("category")).toString();
                ++result.reused;
            } else {
                item.size = directorySize(item.path);
                item.category = categoryFor(item.path);
                ++result.scanned;
            }
            result.packages.insert(item.name, QJsonObject{
                {QStringLiteral("size"), QString::number(item.size)},
                {QStringLiteral("category"), item.category}
            });
            result.entries.append(item);
        }
        return result;
    }));
}

void MsfsManagerWidget::scanFinished()
{
    auto *watcher = static_cast<QFutureWatcher<ScanCache::Result<MsfsCommunityEntry>> *>(sender());
    if (!watcher) return;
    if (watcher->property("generation").toInt() != m_scanGeneration) { watcher->deleteLater(); return; }
    const auto result = watcher->result();
    populatePlugins(result.entries);
    const bool saved = ScanCache::save(watcher->property("cachePath").toString(), result.packages);
    watcher->deleteLater();
    m_scanWatcher = nullptr;
    setScanBusy(false);
    setStatus(QStringLiteral("完成：缓存复用 %1 个，新扫描 %2 个%3").arg(result.reused).arg(result.scanned)
        .arg(saved ? QString() : QStringLiteral("（缓存保存失败）")), !saved);
}

void MsfsManagerWidget::setScanBusy(bool busy)
{
    m_scanButton->setEnabled(!busy);
    m_scanButton->setText(busy ? QStringLiteral("扫描中…") : QStringLiteral("扫描 Community"));
    if (busy) { m_progress->setRange(0, 0); setStatus(QStringLiteral("正在后台扫描 Community…")); }
    else { m_progress->setRange(0, 100); m_progress->setValue(100); }
}

void MsfsManagerWidget::populatePlugins(const QVector<MsfsCommunityEntry> &entries)
{
    m_plugins->setUpdatesEnabled(false);
    m_plugins->clear();
    int enabled = 0;
    for (const auto &entry : entries) {
        if (!entry.disabled) ++enabled;
        auto *item = new QListWidgetItem(m_plugins);
        auto *row = new QWidget(m_plugins);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(10, 5, 10, 5);
        auto *name = new QLabel(QStringLiteral("%1  ·  %2  ·  %3  ·  %4").arg(entry.disabled ? QStringLiteral("[已禁用]") : QStringLiteral("[启用]"), entry.category, entry.name, formatBytes(entry.size)), row);
        rowLayout->addWidget(name, 1);
        auto *open = new QPushButton(QStringLiteral("打开位置"), row);
        auto *toggle = new QPushButton(entry.disabled ? QStringLiteral("启用") : QStringLiteral("禁用"), row);
        for (auto *button : {open, toggle}) { button->setObjectName(QStringLiteral("pluginAction")); button->setMinimumSize(82, 32); button->setStyleSheet(actionStyle()); }
        rowLayout->addWidget(open);
        rowLayout->addWidget(toggle);
        item->setSizeHint(row->sizeHint());
        m_plugins->setItemWidget(item, row);
        connect(open, &QPushButton::clicked, this, [this, path = entry.path] { openPluginPath(path); });
        connect(toggle, &QPushButton::clicked, this, [this, path = entry.path] { togglePluginPath(path); });
    }
    m_plugins->setUpdatesEnabled(true);
    m_summary->setText(QStringLiteral("插件包：%1（启用 %2，禁用 %3）").arg(entries.size()).arg(enabled).arg(entries.size() - enabled));
}

void MsfsManagerWidget::openPluginPath(const QString &path)
{
    QProcess::startDetached(QStringLiteral("explorer.exe"), {QStringLiteral("/select,%1").arg(QDir::toNativeSeparators(path))});
}

void MsfsManagerWidget::togglePluginPath(const QString &path)
{
    const QFileInfo info(path);
    const bool disabled = info.fileName().endsWith(QStringLiteral(".disabled"), Qt::CaseInsensitive);
    const QString target = disabled ? info.absoluteFilePath().left(info.absoluteFilePath().size() - 9) : info.absoluteFilePath() + QStringLiteral(".disabled");
    if (!QDir().rename(info.absoluteFilePath(), target)) { setStatus(QStringLiteral("无法修改插件状态"), true); return; }
    // Preserve the package's cached classification/size across enable/disable.
    const QString cachePath = ScanCache::filePath(QStringLiteral("community"), info.absolutePath());
    QJsonObject cached = ScanCache::load(cachePath);
    if (cached.contains(info.fileName())) {
        cached.insert(QFileInfo(target).fileName(), cached.take(info.fileName()));
        ScanCache::save(cachePath, cached);
    }
    startScan(false);
}

void MsfsManagerWidget::openCommunity()
{
    if (QDir(m_communityPath->text()).exists()) QProcess::startDetached(QStringLiteral("explorer.exe"), {QDir::toNativeSeparators(m_communityPath->text())});
}

void MsfsManagerWidget::setStatus(const QString &message, bool error)
{
    m_status->setText(message);
    m_status->setStyleSheet(error ? QStringLiteral("color: #ff807a;") : QStringLiteral("color: #76ddb2;"));
}
