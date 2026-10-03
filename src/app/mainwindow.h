#pragma once

#include "pluginmanager.h"

#include <QMainWindow>

class QButtonGroup;
class QStackedWidget;
class QLabel;
class QPropertyAnimation;
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QListWidget;
class QFrame;
class QLabel;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void setPage(int index);
    void toggleLanguage();
    void refreshPlugins();

private:
    QWidget *makeSidebar();
    QWidget *makeDashboard();
    QWidget *makeSettingsPage();
    QWidget *makePluginPage(const LoadedPlugin &plugin);
    QWidget *card(const QString &title, const QString &body, const QString &accent = QString());
    void retranslateUi();
    void rebuildPluginNavigation();
    void showPlugin(int pluginIndex);
    void setSidebarCollapsed(bool collapsed);

    PluginManager m_pluginManager;
    QStackedWidget *m_pages = nullptr;
    QButtonGroup *m_navigation = nullptr;
    QVBoxLayout *m_sidebarLayout = nullptr;
    QVBoxLayout *m_navigationLayout = nullptr;
    QHBoxLayout *m_brandLayout = nullptr;
    QFrame *m_sidebar = nullptr;
    QPushButton *m_sidebarToggle = nullptr;
    QLabel *m_brandIcon = nullptr;
    QLabel *m_brandName = nullptr;
    QPropertyAnimation *m_sidebarAnimation = nullptr;
    QPushButton *m_languageButton = nullptr;
    QLabel *m_pageTitle = nullptr;
    QLabel *m_pageSubtitle = nullptr;
    QPushButton *m_overviewButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QVector<QPushButton *> m_pluginButtons;
    QVector<int> m_pluginPageIndexes;
    bool m_chinese = true;
    bool m_sidebarCollapsed = false;
};
