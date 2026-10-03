#pragma once

#include "../../plugins/plugininterface.h"

#include <QVector>
#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;

struct MsfsCommunityEntry
{
    QString path;
    QString name;
    QString category;
    bool disabled = false;
    qint64 size = 0;
};

class MsfsManagerPlugin final : public QObject, public Astraea::PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ASTRAEA_PLUGIN_IID)
    Q_INTERFACES(Astraea::PluginInterface)
public:
    QString id() const override;
    QString name(const QLocale &locale) const override;
    QString description(const QLocale &locale) const override;
    QWidget *createWidget(QWidget *parent) override;
};

class MsfsManagerWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit MsfsManagerWidget(QWidget *parent = nullptr);

private slots:
    void chooseCommunity();
    void scanCommunity();
    void scanFinished();
    void openCommunity();

private:
    void startScan(bool fullScan);
    QString detectCommunity() const;
    void setStatus(const QString &message, bool error = false);
    void setScanBusy(bool busy);
    void populatePlugins(const QVector<MsfsCommunityEntry> &entries);
    void openPluginPath(const QString &path);
    void togglePluginPath(const QString &path);

    QLineEdit *m_communityPath = nullptr;
    QLabel *m_summary = nullptr;
    QLabel *m_status = nullptr;
    QListWidget *m_plugins = nullptr;
    QProgressBar *m_progress = nullptr;
    QPushButton *m_scanButton = nullptr;
    QObject *m_scanWatcher = nullptr;
    int m_scanGeneration = 0;
};
