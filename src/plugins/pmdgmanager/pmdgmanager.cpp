#include "pmdgmanager.h"
#include "../scancache.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QDesktopServices>
#include <QPushButton>
#include <QStandardPaths>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextStream>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QRegularExpression>
#include <QUrl>
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
        QString result = line.section(QLatin1Char(' '), 1).trimmed();
        if (result.isEmpty()) result = line.section(QLatin1Char('='), 1).trimmed();
        result.remove(QLatin1Char('"'));
        return QDir::fromNativeSeparators(result);
    }
    return {};
}

QString readLiveryName(const QString &path)
{
    QFile file(QDir(path).filePath(QStringLiteral("livery.cfg")));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QFileInfo(path).fileName();
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.startsWith(QStringLiteral("name"), Qt::CaseInsensitive) && line.contains(QLatin1Char('='))) return line.section(QLatin1Char('='), 1).trimmed().remove(QLatin1Char('"'));
    }
    return QFileInfo(path).fileName();
}

QString pmdgAircraftCode(const QString &value);

QString aircraftFor(const QString &package, const QString &livery)
{
    const QString relativePath = QDir::fromNativeSeparators(QDir(package).relativeFilePath(livery));
    const QStringList parts = relativePath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i) {
        if (parts.at(i).compare(QStringLiteral("Airplanes"), Qt::CaseInsensitive) == 0 && i + 1 < parts.size()) {
            // Third-party liveries commonly identify the aircraft solely by
            // the folder after SimObjects/Airplanes, e.g.
            // SimObjects/Airplanes/PMDG 737-800/liveries/pmdg/...
            return pmdgAircraftCode(parts.at(i + 1));
        }
    }
    return pmdgAircraftCode(QFileInfo(package).fileName() + QLatin1Char(' ') + livery);
}

QString aircraftFolderFor(const QString &package, const QString &livery)
{
    const QString relativePath = QDir::fromNativeSeparators(QDir(package).relativeFilePath(livery));
    const QStringList parts = relativePath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (int i = 0; i + 1 < parts.size(); ++i) {
        if (parts.at(i).compare(QStringLiteral("Airplanes"), Qt::CaseInsensitive) == 0) return parts.at(i + 1);
    }
    return {};
}

QString fallbackAircraftFolder(const QString &code)
{
    if (code == QStringLiteral("738")) return QStringLiteral("PMDG 737-800");
    if (code == QStringLiteral("739")) return QStringLiteral("PMDG 737-900");
    if (code == QStringLiteral("772")) return QStringLiteral("PMDG 777-200ER");
    if (code == QStringLiteral("77W")) return QStringLiteral("PMDG 777-300ER");
    if (code == QStringLiteral("736")) return QStringLiteral("PMDG 737-600");
    if (code == QStringLiteral("737")) return QStringLiteral("PMDG 737-700");
    return code;
}

QString pmdgAircraftCode(const QString &value)
{
    const QString folder = value.toLower();
    if (folder.contains(QStringLiteral("737-600")) || folder.contains(QStringLiteral("737_600")) || folder.contains(QStringLiteral("736"))) return QStringLiteral("736");
    if (folder.contains(QStringLiteral("737-700")) || folder.contains(QStringLiteral("737_700")) || folder.contains(QStringLiteral("737-7")) || folder.contains(QStringLiteral("737_7")) || folder.contains(QStringLiteral("7377"))) return QStringLiteral("737");
    if (folder.contains(QStringLiteral("737-800")) || folder.contains(QStringLiteral("737_800")) || folder.contains(QStringLiteral("738"))) return QStringLiteral("738");
    if (folder.contains(QStringLiteral("737-900")) || folder.contains(QStringLiteral("737_900")) || folder.contains(QStringLiteral("739"))) return QStringLiteral("739");
    if (folder.contains(QStringLiteral("777-200")) || folder.contains(QStringLiteral("777_200")) || folder.contains(QStringLiteral("77er")) || folder.contains(QStringLiteral("772"))) return QStringLiteral("772");
    if (folder.contains(QStringLiteral("777-300")) || folder.contains(QStringLiteral("777_300")) || folder.contains(QStringLiteral("77w")) || folder.contains(QStringLiteral("77w"))) return QStringLiteral("77W");
    return value.trimmed().isEmpty() ? QStringLiteral("PMDG") : value;
}

QString safeName(QString name)
{
    name.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]+")), QStringLiteral("-"));
    while (name.contains(QStringLiteral("--"))) name.replace(QStringLiteral("--"), QStringLiteral("-"));
    return name.isEmpty() ? QStringLiteral("pmdg-livery") : name;
}

bool copyDirectory(const QString &source, const QString &destination, QString *error)
{
    if (!QDir(source).exists() || !QDir().mkpath(destination)) { if (error) *error = QStringLiteral("无法创建安装目录"); return false; }
    QDir sourceDir(source);
    QDirIterator iterator(source, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QString target = QDir(destination).filePath(sourceDir.relativeFilePath(iterator.filePath()));
        if (iterator.fileInfo().isDir()) { if (!QDir().mkpath(target)) { if (error) *error = target; return false; } }
        else if (!QFile::copy(iterator.filePath(), target)) { if (error) *error = iterator.fileName(); return false; }
    }
    return true;
}

QString findLiveryFolder(const QString &root)
{
    if (QFileInfo(QDir(root).filePath(QStringLiteral("livery.cfg"))).exists()) return root;
    QDirIterator iterator(root, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iterator.hasNext()) { iterator.next(); if (QFileInfo(QDir(iterator.filePath()).filePath(QStringLiteral("livery.cfg"))).exists()) return iterator.filePath(); }
    return {};
}
}

QString PmdgManagerPlugin::id() const { return QStringLiteral("astraea.pmdg.livery.manager"); }
QString PmdgManagerPlugin::name(const QLocale &locale) const { return locale.language() == QLocale::Chinese ? QStringLiteral("PMDG 涂装管理") : QStringLiteral("PMDG Livery Manager"); }
QString PmdgManagerPlugin::description(const QLocale &locale) const { return locale.language() == QLocale::Chinese ? QStringLiteral("扫描和安装 MSFS 2024 PMDG 涂装。") : QStringLiteral("Scan and install MSFS 2024 PMDG liveries."); }
QWidget *PmdgManagerPlugin::createWidget(QWidget *parent) { return new PmdgManagerWidget(parent); }

PmdgManagerWidget::PmdgManagerWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->addWidget(new QLabel(QStringLiteral("PMDG 涂装管理：扫描 Community 中的 PMDG 涂装，或安装新的涂装包。"), this));
    auto *pathRow = new QHBoxLayout;
    m_communityPath = new QLineEdit(this);
    m_communityPath->setPlaceholderText(QStringLiteral("Community 文件夹路径"));
    auto *browse = new QPushButton(QStringLiteral("选择"), this);
    auto *scan = new QPushButton(QStringLiteral("扫描涂装"), this);
    pathRow->addWidget(m_communityPath, 1); pathRow->addWidget(browse); pathRow->addWidget(scan);
    layout->addLayout(pathRow);
    auto *installRow = new QHBoxLayout;
    installRow->addWidget(new QLabel(QStringLiteral("安装目标机型："), this));
    m_aircraft = new QComboBox(this);
    m_aircraft->addItems({QStringLiteral("PMDG 737-600"), QStringLiteral("PMDG 737-700"), QStringLiteral("PMDG 737-800"), QStringLiteral("PMDG 737-900"), QStringLiteral("PMDG 777-200ER"), QStringLiteral("PMDG 777-300ER"), QStringLiteral("PMDG 777F")});
    installRow->addWidget(m_aircraft);
    auto *install = new QPushButton(QStringLiteral("安装 ZIP / 文件夹"), this);
    installRow->addWidget(install); installRow->addStretch();
    layout->addLayout(installRow);
    m_summary = new QLabel(this); layout->addWidget(m_summary);
    m_liveries = new QListWidget(this); m_liveries->setSelectionMode(QAbstractItemView::SingleSelection); layout->addWidget(m_liveries, 1);
    m_status = new QLabel(this); layout->addWidget(m_status);
    connect(browse, &QPushButton::clicked, this, &PmdgManagerWidget::chooseCommunity);
    connect(scan, &QPushButton::clicked, this, &PmdgManagerWidget::scanLiveries);
    connect(install, &QPushButton::clicked, this, &PmdgManagerWidget::installLivery);
    m_communityPath->setText(QSettings().value(QStringLiteral("msfs/communityPath"), detectCommunity()).toString());
    startScan(false);
}

QString PmdgManagerWidget::detectCommunity() const
{
    const QStringList configs = {QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(QStringLiteral("../Microsoft Flight Simulator 2024/UserCfg.opt")), QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath(QStringLiteral("Packages/Microsoft.Limitless_8wekyb3d8bbwe/LocalCache/UserCfg.opt"))};
    for (const QString &config : configs) { const QString installed = readInstalledPackagesPath(QDir::cleanPath(config)); const QString community = QDir(installed).filePath(QStringLiteral("Community")); if (!installed.isEmpty() && QDir(community).exists()) return QDir::toNativeSeparators(community); }
    return {};
}

void PmdgManagerWidget::chooseCommunity()
{
    const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("选择 Community 文件夹"), m_communityPath->text());
    if (!path.isEmpty()) { m_communityPath->setText(path); QSettings().setValue(QStringLiteral("msfs/communityPath"), path); scanLiveries(); }
}

void PmdgManagerWidget::scanLiveries()
{
    startScan(true);
}

void PmdgManagerWidget::startScan(bool fullScan)
{
    m_liveries->clear();
    const int generation = ++m_scanGeneration;
    const QString input = m_communityPath->text().trimmed();
    const QString community = input.isEmpty() ? QString() : QDir(input).absolutePath();
    if (community.isEmpty() || !QDir(community).exists()) { m_summary->setText(QStringLiteral("尚未设置有效的 Community 文件夹。")); return; }
    QSettings().setValue(QStringLiteral("msfs/communityPath"), community);
    m_summary->setText(QStringLiteral("正在扫描 PMDG 涂装…"));
    const QString cachePath = ScanCache::filePath(QStringLiteral("pmdg-v3"), community);
    auto *watcher = new QFutureWatcher<ScanCache::Result<PmdgLiveryEntry>>(this);
    connect(watcher, &QFutureWatcher<ScanCache::Result<PmdgLiveryEntry>>::finished, this, [this, watcher, generation] {
        if (generation != m_scanGeneration) { watcher->deleteLater(); return; }
        const auto result = watcher->result();
        populate(result.entries);
        const bool saved = ScanCache::save(watcher->property("cachePath").toString(), result.packages);
        m_summary->setText(QStringLiteral("已找到 %1 个 PMDG 涂装（缓存复用 %2 个插件包，新扫描 %3 个）")
            .arg(result.entries.size()).arg(result.reused).arg(result.scanned));
        if (!saved) setStatus(QStringLiteral("扫描完成，但缓存保存失败"), true);
        else setStatus(QStringLiteral("扫描完成"));
        watcher->deleteLater();
    });
    watcher->setProperty("cachePath", cachePath);
    watcher->setFuture(QtConcurrent::run([community, cachePath, fullScan] {
        ScanCache::Result<PmdgLiveryEntry> result;
        const QJsonObject cached = fullScan ? QJsonObject() : ScanCache::load(cachePath);
        const QFileInfoList packages = QDir(community).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &package : packages) {
            const QJsonObject previous = cached.value(package.fileName()).toObject();
            if (previous.contains(QStringLiteral("entries"))) {
                for (const auto &value : previous.value(QStringLiteral("entries")).toArray()) {
                    const QJsonObject object = value.toObject();
                    result.entries.append({package.fileName(), object.value(QStringLiteral("name")).toString(), object.value(QStringLiteral("aircraft")).toString(), object.value(QStringLiteral("path")).toString()});
                }
                result.packages.insert(package.fileName(), previous);
                ++result.reused;
                continue;
            }
            QJsonArray packageEntries;
            QDirIterator iterator(package.absoluteFilePath(), QDir::Files, QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                iterator.next();
                if (iterator.fileName().compare(QStringLiteral("livery.cfg"), Qt::CaseInsensitive) != 0) continue;
                const QString path = QFileInfo(iterator.filePath()).absolutePath();
                const QString aircraft = aircraftFor(package.absoluteFilePath(), path);
                if (!package.fileName().contains(QStringLiteral("pmdg"), Qt::CaseInsensitive) && !aircraft.contains(QStringLiteral("pmdg"), Qt::CaseInsensitive)) continue;
                const QString name = readLiveryName(path);
                result.entries.append({package.fileName(), name, aircraft, path});
                packageEntries.append(QJsonObject{{QStringLiteral("name"), name}, {QStringLiteral("aircraft"), aircraft}, {QStringLiteral("path"), path}});
            }
            result.packages.insert(package.fileName(), QJsonObject{{QStringLiteral("entries"), packageEntries}});
            ++result.scanned;
        }
        return result;
    }));
}

void PmdgManagerWidget::populate(const QVector<PmdgLiveryEntry> &entries)
{
    m_liveries->clear();
    QMap<QString, QVector<PmdgLiveryEntry>> grouped;
    for (const auto &entry : entries) grouped[entry.aircraft].append(entry);
    for (auto group = grouped.cbegin(); group != grouped.cend(); ++group) {
        auto *header = new QListWidgetItem(QStringLiteral("  %1  (%2 个涂装)").arg(group.key()).arg(group.value().size()), m_liveries);
        QFont headerFont = header->font(); headerFont.setBold(true); header->setFont(headerFont); header->setFlags(Qt::NoItemFlags); header->setBackground(QColor(QStringLiteral("#222a37")));
        for (const auto &entry : group.value()) {
            auto *item = new QListWidgetItem(m_liveries); auto *row = new QWidget(m_liveries); auto *rowLayout = new QHBoxLayout(row); rowLayout->setContentsMargins(20, 4, 10, 4); rowLayout->addWidget(new QLabel(QStringLiteral("%1  ·  %2").arg(entry.liveryName, entry.packageName), row), 1); auto *open = new QPushButton(QStringLiteral("打开位置"), row); open->setMinimumSize(82, 32); rowLayout->addWidget(open); item->setSizeHint(row->sizeHint()); m_liveries->setItemWidget(item, row); connect(open, &QPushButton::clicked, this, [this, path = entry.path] { openPath(path); });
        }
    }
    if (entries.isEmpty()) m_liveries->addItem(QStringLiteral("尚未识别到 PMDG 涂装"));
    m_summary->setText(QStringLiteral("已找到 %1 个 PMDG 涂装").arg(entries.size()));
}

void PmdgManagerWidget::openPath(const QString &path) { QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath())); }

void PmdgManagerWidget::installLivery()
{
    const QString community = QDir::cleanPath(m_communityPath->text().trimmed());
    if (!QDir(community).exists()) { setStatus(QStringLiteral("请先设置有效的 Community 文件夹"), true); return; }
    const auto choice = QMessageBox::question(this, QStringLiteral("安装 PMDG 涂装"), QStringLiteral("选择“是”安装 ZIP，选择“否”安装已解压文件夹。"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (choice == QMessageBox::Cancel) return;
    const QString source = choice == QMessageBox::Yes ? QFileDialog::getOpenFileName(this, QStringLiteral("选择 ZIP"), {}, QStringLiteral("ZIP 文件 (*.zip)")) : QFileDialog::getExistingDirectory(this, QStringLiteral("选择涂装文件夹"), community);
    if (source.isEmpty()) return;
    QTemporaryDir temporary; if (!temporary.isValid()) { setStatus(QStringLiteral("无法创建临时目录"), true); return; }
    QString root = temporary.path();
    if (source.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) { QProcess extract; extract.start(QStringLiteral("tar"), {QStringLiteral("-xf"), source, QStringLiteral("-C"), temporary.path()}); if (!extract.waitForFinished(120000) || extract.exitCode() != 0) { setStatus(QStringLiteral("解压失败"), true); return; } root = findLiveryFolder(temporary.path()); }
    else root = findLiveryFolder(source);
    if (root.isEmpty()) { setStatus(QStringLiteral("未找到 livery.cfg"), true); return; }
    const QString liveryName = readLiveryName(root);
    QString name = safeName(liveryName);
    if (name == QStringLiteral("pmdg-livery") || name.isEmpty()) name = safeName(QFileInfo(source).completeBaseName());
    QString destination = QDir(community).filePath(QStringLiteral("pmdg-%1").arg(name));
    int index = 2;
    while (QDir(destination).exists()) destination = QDir(community).filePath(QStringLiteral("pmdg-%1-%2").arg(name).arg(index++));
    // Prefer the aircraft directory included by the creator. This prevents a
    // 777 livery from being installed under the default 737-800 selection.
    QString aircraftFolder = aircraftFolderFor(temporary.path(), root);
    if (aircraftFolder.isEmpty() && !source.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) aircraftFolder = aircraftFolderFor(source, root);
    if (aircraftFolder.isEmpty()) aircraftFolder = fallbackAircraftFolder(pmdgAircraftCode(m_aircraft->currentText()));
    const QString target = QDir(destination).filePath(QStringLiteral("SimObjects/Airplanes/%1/liveries/pmdg/%2").arg(aircraftFolder, QFileInfo(root).fileName()));
    QString error; if (!copyDirectory(root, target, &error)) { setStatus(QStringLiteral("安装失败：%1").arg(error), true); return; }
    QDir().mkpath(destination);
    QFile layout(QDir(destination).filePath(QStringLiteral("layout.json")));
    if (layout.open(QIODevice::WriteOnly)) layout.write("{}");
    QFile manifest(QDir(destination).filePath(QStringLiteral("manifest.json"))); if (manifest.open(QIODevice::WriteOnly | QIODevice::Text)) manifest.write(QJsonDocument(QJsonObject{{QStringLiteral("dependencies"), QJsonArray()}, {QStringLiteral("content_type"), QStringLiteral("AIRCRAFT")}, {QStringLiteral("title"), name}, {QStringLiteral("package_version"), QStringLiteral("1.0.0")}}).toJson());
    setStatus(QStringLiteral("涂装安装完成")); startScan(false);
}

void PmdgManagerWidget::setStatus(const QString &message, bool error) { m_status->setText(message); m_status->setStyleSheet(error ? QStringLiteral("color:#ff807a;") : QStringLiteral("color:#76ddb2;")); }
