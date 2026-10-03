#pragma once

#include "../../plugins/plugininterface.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

struct PmdgLiveryEntry
{
    QString packageName;
    QString liveryName;
    QString aircraft;
    QString path;
};

class PmdgManagerPlugin final : public QObject, public Astraea::PluginInterface
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

class PmdgManagerWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit PmdgManagerWidget(QWidget *parent = nullptr);

private slots:
    void chooseCommunity();
    void scanLiveries();
    void installLivery();

private:
    void startScan(bool fullScan);
    QString detectCommunity() const;
    void setStatus(const QString &message, bool error = false);
    void populate(const QVector<PmdgLiveryEntry> &entries);
    void openPath(const QString &path);

    QLineEdit *m_communityPath = nullptr;
    QComboBox *m_aircraft = nullptr;
    QListWidget *m_liveries = nullptr;
    QLabel *m_summary = nullptr;
    QLabel *m_status = nullptr;
    int m_scanGeneration = 0;
};
