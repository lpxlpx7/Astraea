#pragma once

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVector>

namespace ScanCache {

inline QString filePath(const QString &kind, const QString &community)
{
    const QString normalized = QDir::cleanPath(QDir::fromNativeSeparators(community)).toCaseFolded();
    const QString digest = QString::fromLatin1(QCryptographicHash::hash(normalized.toUtf8(), QCryptographicHash::Sha256).toHex());
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
        .filePath(QStringLiteral("scan-cache/%1-%2.json").arg(kind, digest));
}

inline QJsonObject load(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    if (root.value(QStringLiteral("version")).toInt() != 1) return {};
    return root.value(QStringLiteral("packages")).toObject();
}

inline bool save(const QString &path, const QJsonObject &packages)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    const QByteArray data = QJsonDocument(QJsonObject{
        {QStringLiteral("version"), 1}, {QStringLiteral("packages"), packages}
    }).toJson(QJsonDocument::Compact);
    if (file.write(data) != data.size()) return false;
    return file.commit();
}

template<class Entry> struct Result
{
    QVector<Entry> entries;
    QJsonObject packages;
    int reused = 0;
    int scanned = 0;
};

} // namespace ScanCache
