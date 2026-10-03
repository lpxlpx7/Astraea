#include "mainwindow.h"

#include <QButtonGroup>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QSettings>
#include <QScrollArea>
#include <QStatusBar>
#include <QStyle>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {
QLabel *label(const QString &text, const QString &objectName = {})
{
    auto *result = new QLabel(text);
    result->setObjectName(objectName);
    result->setWordWrap(true);
    return result;
}

QLabel *localizedLabel(const QString &cn, const QString &en, const QString &name = {})
{
    auto *widget = label(cn, name);
    widget->setProperty("textCn", cn);
    widget->setProperty("textEn", en);
    return widget;
}

void localizeButton(QPushButton *button, const QString &cn, const QString &en)
{
    button->setProperty("textCn", cn);
    button->setProperty("textEn", en);
}

QVBoxLayout *column(QWidget *parent, int margin = 0)
{
    auto *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(12);
    return layout;
}
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_chinese = QSettings().value(QStringLiteral("language"), QStringLiteral("zh")).toString() != QStringLiteral("en");
    setWindowTitle(QStringLiteral("Astraea"));
    setMinimumSize(1080, 680);
    resize(1280, 780);

    auto *root = new QWidget;
    root->setObjectName(QStringLiteral("root"));
    auto *layout = new QHBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(makeSidebar());

    auto *content = new QWidget;
    auto *contentLayout = column(content, 30);
    auto *topbar = new QHBoxLayout;
    m_pageTitle = label({}, QStringLiteral("title"));
    m_pageSubtitle = label({}, QStringLiteral("subtitle"));
    auto *heading = new QVBoxLayout;
    heading->addWidget(m_pageTitle);
    heading->addWidget(m_pageSubtitle);
    topbar->addLayout(heading);
    topbar->addStretch();
    m_languageButton = new QPushButton;
    m_languageButton->setObjectName(QStringLiteral("language"));
    connect(m_languageButton, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    topbar->addWidget(m_languageButton);
    contentLayout->addLayout(topbar);

    m_pages = new QStackedWidget;
    m_pages->addWidget(makeDashboard());
    m_pages->addWidget(makeSettingsPage());
    contentLayout->addWidget(m_pages, 1);
    layout->addWidget(content, 1);
    setCentralWidget(root);

    const QString pluginPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("plugins"));
    connect(&m_pluginManager, &PluginManager::pluginsChanged, this, &MainWindow::refreshPlugins);
    connect(&m_pluginManager, &PluginManager::errorOccurred, this, [this](const QString &message) {
        statusBar()->showMessage(message, 5000);
    });
    m_pluginManager.scan(pluginPath);
    retranslateUi();
}

QWidget *MainWindow::makeSidebar()
{
    m_sidebar = new QFrame;
    m_sidebar->setObjectName(QStringLiteral("sidebar"));
    m_sidebar->setFixedWidth(224);
    m_sidebarLayout = column(m_sidebar, 18);
    auto *layout = m_sidebarLayout;
    auto *brandRow = new QWidget;
    brandRow->setObjectName(QStringLiteral("brandRow"));
    auto *brandLayout = new QHBoxLayout(brandRow);
    brandLayout->setContentsMargins(0, 0, 0, 0);
    brandLayout->setSpacing(10);
    m_brandIcon = new QLabel;
    m_brandIcon->setObjectName(QStringLiteral("brandIcon"));
    m_brandIcon->setFixedSize(34, 34);
    m_brandIcon->setScaledContents(true);
    m_brandIcon->setPixmap(QPixmap(QStringLiteral(":/branding/logo-bright.png")));
    m_brandLayout = brandLayout;
    brandLayout->addWidget(m_brandIcon, 0, Qt::AlignVCenter);
    m_brandName = label(QStringLiteral("Astraea"), QStringLiteral("brand"));
    brandLayout->addWidget(m_brandName, 1, Qt::AlignVCenter);
    layout->addWidget(brandRow);
    auto *caption = localizedLabel(QStringLiteral("模拟飞行工作台"), QStringLiteral("Flight platform"), QStringLiteral("eyebrow"));
    layout->addWidget(caption);
    layout->addSpacing(24);

    m_navigation = new QButtonGroup(this);
    m_navigation->setExclusive(true);
    auto *navigationScroll = new QScrollArea;
    navigationScroll->setObjectName(QStringLiteral("sidebarScroll"));
    navigationScroll->setFrameShape(QFrame::NoFrame);
    navigationScroll->setWidgetResizable(true);
    navigationScroll->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    navigationScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigationScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    navigationScroll->setStyleSheet(QStringLiteral("QScrollArea#sidebarScroll { background: transparent; border: 0; }"));
    auto *navigationWidget = new QWidget;
    navigationWidget->setObjectName(QStringLiteral("sidebarNavigation"));
    m_navigationLayout = column(navigationWidget, 0);
    m_navigationLayout->setSpacing(4);
    m_navigationLayout->setAlignment(Qt::AlignTop);
    navigationWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    navigationScroll->setWidget(navigationWidget);
    layout->addWidget(navigationScroll, 1);

    auto addButton = [this](const QString &text, int page) {
        auto *button = new QPushButton(text);
        button->setObjectName(QStringLiteral("sidebarNav"));
        button->setCheckable(true);
        button->setMinimumHeight(36);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_navigation->addButton(button);
        m_navigationLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, page] { setPage(page); });
        return button;
    };
    m_overviewButton = addButton(QStringLiteral("◈   概览"), 0);
    m_overviewButton->setChecked(true);
    m_settingsButton = addButton(QStringLiteral("⚙   设置"), 1);
    layout->addSpacing(4);
    m_sidebarToggle = new QPushButton(QStringLiteral("‹   收起侧边栏"));
    m_sidebarToggle->setObjectName(QStringLiteral("sidebarToggle"));
    m_sidebarToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_sidebarToggle, &QPushButton::clicked, this, [this] { setSidebarCollapsed(!m_sidebarCollapsed); });
    layout->addWidget(m_sidebarToggle);
    auto *hint = label(QStringLiteral("v0.1.0"), QStringLiteral("sidebarVersion"));
    hint->setAlignment(Qt::AlignCenter);
    layout->addWidget(hint);
    return m_sidebar;
}

QWidget *MainWindow::makeDashboard()
{
    auto *page = new QWidget;
    auto *layout = column(page);
    auto *hero = new QFrame;
    hero->setObjectName(QStringLiteral("hero"));
    auto *heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(24, 22, 24, 22);
    auto *heroText = new QVBoxLayout;
    heroText->addWidget(localizedLabel(QStringLiteral("你的飞行工作台"), QStringLiteral("Your flight desk"), QStringLiteral("eyebrow")));
    heroText->addWidget(localizedLabel(QStringLiteral("一站式连接你的飞行工具。"), QStringLiteral("Your flight tools, in one place."), QStringLiteral("title")));
    heroText->addWidget(localizedLabel(QStringLiteral("这是一个空白插件外壳。安装插件后，对应功能会出现在左侧。"), QStringLiteral("Start with an empty shell. Installed plugins appear in the sidebar."), QStringLiteral("subtitle")));
    heroLayout->addLayout(heroText, 1);
    auto *open = new QPushButton(QStringLiteral("＋  打开插件目录"));
    open->setObjectName(QStringLiteral("primary"));
    localizeButton(open, QStringLiteral("打开插件目录"), QStringLiteral("Open plugins folder"));
    connect(open, &QPushButton::clicked, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_pluginManager.directory()));
    });
    heroLayout->addWidget(open, 0, Qt::AlignBottom);
    layout->addWidget(hero);

    auto *count = localizedLabel(QStringLiteral("已安装插件"), QStringLiteral("Installed plugins"), QStringLiteral("pluginCount"));
    layout->addWidget(count);
    layout->addWidget(localizedLabel(QStringLiteral("将可信的 Qt 插件或 .astraea.json 清单放入 plugins 文件夹，然后在设置中重新扫描。"), QStringLiteral("Add trusted Qt plugins or .astraea.json manifests to the plugins folder, then rescan in Settings."), QStringLiteral("subtitle")));
    layout->addStretch();
    return page;
}

QWidget *MainWindow::card(const QString &title, const QString &body, const QString &accent)
{
    auto *frame = new QFrame;
    frame->setObjectName(QStringLiteral("card"));
    auto *layout = column(frame, 18);
    auto *row = new QHBoxLayout;
    row->addWidget(label(title, QStringLiteral("muted")));
    row->addStretch();
    if (!accent.isEmpty()) row->addWidget(label(accent, QStringLiteral("badge")));
    layout->addLayout(row);
    layout->addWidget(label(body, body.size() < 8 ? QStringLiteral("metric") : QStringLiteral("subtitle")));
    return frame;
}

QWidget *MainWindow::makeSettingsPage()
{
    auto *page = new QWidget;
    auto *layout = column(page);
    layout->addWidget(localizedLabel(QStringLiteral("插件目录"), QStringLiteral("Plugins folder"), QStringLiteral("subtitle")));
    layout->addWidget(label(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("plugins"))));
    layout->addWidget(localizedLabel(QStringLiteral("界面字体：Segoe UI"), QStringLiteral("Interface font: Segoe UI"), QStringLiteral("subtitle")));
    auto *rescan = new QPushButton;
    localizeButton(rescan, QStringLiteral("重新扫描插件"), QStringLiteral("Rescan plugins"));
    connect(rescan, &QPushButton::clicked, this, [this] {
        // Destroy plugin views before unloading their libraries.
        setPage(1);
        while (m_pages->count() > 2) delete m_pages->widget(2);
        m_pluginManager.scan(m_pluginManager.directory());
    });
    layout->addWidget(rescan, 0, Qt::AlignLeft);
    layout->addStretch();
    return page;
}

void MainWindow::setPage(int index)
{
    m_pages->setCurrentIndex(index);
    retranslateUi();
}

void MainWindow::toggleLanguage()
{
    m_chinese = !m_chinese;
    QSettings().setValue(QStringLiteral("language"), m_chinese ? QStringLiteral("zh") : QStringLiteral("en"));
    retranslateUi();
}

void MainWindow::refreshPlugins()
{
    rebuildPluginNavigation();
    retranslateUi();
}

void MainWindow::retranslateUi()
{
    const QString overview = m_chinese ? QStringLiteral("概览") : QStringLiteral("Overview");
    const QString settings = m_chinese ? QStringLiteral("设置") : QStringLiteral("Settings");
    m_overviewButton->setText(m_sidebarCollapsed ? QStringLiteral("◈") : QStringLiteral("◈   %1").arg(overview));
    m_settingsButton->setText(m_sidebarCollapsed ? QStringLiteral("⚙") : QStringLiteral("⚙   %1").arg(settings));
    m_languageButton->setText(m_chinese ? QStringLiteral("English") : QStringLiteral("中文"));
    m_languageButton->setToolTip(m_chinese ? QStringLiteral("切换到英语") : QStringLiteral("Switch to Chinese"));
    const int index = m_pages->currentIndex();
    m_pageTitle->setText(index == 0 ? overview : settings);
    m_pageSubtitle->setText(m_chinese ? QStringLiteral("欢迎回到你的模拟飞行工作台") : QStringLiteral("Welcome back to your flight desk"));
    m_sidebarToggle->setText(m_sidebarCollapsed ? QStringLiteral("›") : (m_chinese ? QStringLiteral("‹   收起侧边栏") : QStringLiteral("‹   Collapse sidebar")));
    m_overviewButton->setToolTip(overview);
    m_settingsButton->setToolTip(settings);
    m_sidebarToggle->setToolTip(m_chinese ? QStringLiteral("展开/收起侧边栏") : QStringLiteral("Expand/collapse sidebar"));
    for (auto *widget : findChildren<QLabel *>()) {
        if (widget->property("textCn").isValid()) widget->setText(widget->property(m_chinese ? "textCn" : "textEn").toString());
    }
    for (auto *button : findChildren<QPushButton *>()) {
        if (button->property("textCn").isValid()) button->setText(button->property(m_chinese ? "textCn" : "textEn").toString());
    }
    if (auto *count = findChild<QLabel *>(QStringLiteral("pluginCount")))
        count->setText((m_chinese ? QStringLiteral("已安装插件：%1") : QStringLiteral("Installed plugins: %1")).arg(m_pluginManager.plugins().size()));
    for (int i = 0; i < m_pluginButtons.size(); ++i) {
        const auto &plugin = m_pluginManager.plugins().at(i);
        const QString name = plugin.instance ? plugin.instance->name(QLocale(m_chinese ? "zh_CN" : "en_US")) : plugin.displayName;
        m_pluginButtons.at(i)->setText(m_sidebarCollapsed ? QStringLiteral("◇") : QStringLiteral("◇   %1").arg(name));
        m_pluginButtons.at(i)->setToolTip(name);
        const auto labels = m_pages->widget(m_pluginPageIndexes.at(i))->findChildren<QLabel *>();
        const QLocale locale(m_chinese ? "zh_CN" : "en_US");
        for (auto *widget : labels) {
            if (widget->objectName() == QStringLiteral("title")) widget->setText(name);
            if (widget->objectName() == QStringLiteral("subtitle")) widget->setText(plugin.instance ? plugin.instance->description(locale) : plugin.description);
        }
    }
    if (index < 2) (index == 0 ? m_overviewButton : m_settingsButton)->setChecked(true);
    if (index >= 2) showPlugin(index - 2);
}

void MainWindow::rebuildPluginNavigation()
{
    for (auto *button : m_pluginButtons) {
        m_navigation->removeButton(button);
        m_navigationLayout->removeWidget(button);
        delete button;
    }
    m_pluginButtons.clear();
    m_pluginPageIndexes.clear();
    while (m_pages->count() > 2) delete m_pages->widget(2);
    const auto plugins = m_pluginManager.plugins();
    for (int i = 0; i < plugins.size(); ++i) {
        auto *button = new QPushButton(m_sidebarCollapsed ? QStringLiteral("◇") : QStringLiteral("◇   %1").arg(plugins.at(i).displayName));
        button->setCheckable(true);
        button->setMinimumHeight(42);
        m_navigation->addButton(button);
        button->setObjectName(QStringLiteral("sidebarNav"));
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        // Sidebar order is Overview, plugins, Settings. The settings button
        // already occupies the second fixed slot, so insert plugins before it.
        m_navigationLayout->insertWidget(i + 1, button);
        const int pageIndex = m_pages->addWidget(makePluginPage(plugins.at(i)));
        m_pluginPageIndexes.append(pageIndex);
        m_pluginButtons.append(button);
        connect(button, &QPushButton::clicked, this, [this, i] { showPlugin(i); });
    }
}

QWidget *MainWindow::makePluginPage(const LoadedPlugin &plugin)
{
    auto *page = new QWidget;
    auto *layout = column(page);
    if (plugin.kind == PluginKind::NativeQt && plugin.instance) {
        // Native Qt plugins are embedded directly in the right-hand workspace.
        auto *view = plugin.instance->createWidget(page);
        if (view) {
            layout->addWidget(view, 1);
        } else {
            layout->addWidget(label(plugin.displayName, QStringLiteral("title")));
            layout->addWidget(label(plugin.description, QStringLiteral("subtitle")));
            layout->addWidget(label(m_chinese ? QStringLiteral("插件没有返回可显示的页面。") : QStringLiteral("The plugin did not provide a view."), QStringLiteral("subtitle")));
        }
    } else {
        layout->addWidget(label(plugin.displayName, QStringLiteral("title")));
        layout->addWidget(label(plugin.description, QStringLiteral("subtitle")));
        auto *open = new QPushButton(m_chinese ? QStringLiteral("打开插件") : QStringLiteral("Open plugin"));
        localizeButton(open, QStringLiteral("打开插件"), QStringLiteral("Open plugin"));
        open->setObjectName(QStringLiteral("primary"));
        connect(open, &QPushButton::clicked, this, [this, plugin] {
            if (QMessageBox::question(this, QStringLiteral("Astraea"), QStringLiteral("Only run trusted plugins. Open %1?").arg(plugin.displayName)) != QMessageBox::Yes) return;
            m_pluginManager.launch(plugin);
        });
        layout->addWidget(open, 0, Qt::AlignLeft);
    }
    layout->addStretch();
    return page;
}

void MainWindow::showPlugin(int pluginIndex)
{
    if (pluginIndex < 0 || pluginIndex >= m_pluginPageIndexes.size()) return;
    m_pages->setCurrentIndex(m_pluginPageIndexes.at(pluginIndex));
    m_pluginButtons.at(pluginIndex)->setChecked(true);
    const auto &plugin = m_pluginManager.plugins().at(pluginIndex);
    const QLocale locale(m_chinese ? "zh_CN" : "en_US");
    m_pageTitle->setText(plugin.instance ? plugin.instance->name(locale) : plugin.displayName);
    m_pageSubtitle->setText(plugin.instance ? plugin.instance->description(locale) : plugin.description);
}

void MainWindow::setSidebarCollapsed(bool collapsed)
{
    if (m_sidebarCollapsed == collapsed && !m_sidebarAnimation) return;
    m_sidebarCollapsed = collapsed;
    const int startWidth = m_sidebar->width();
    const int targetWidth = collapsed ? 68 : 224;
    if (m_sidebarAnimation) {
        m_sidebarAnimation->stop();
        m_sidebarAnimation->deleteLater();
    }
    m_sidebar->setMinimumWidth(0);
    m_sidebar->setMaximumWidth(startWidth);
    m_sidebarAnimation = new QPropertyAnimation(m_sidebar, "maximumWidth", this);
    m_sidebarAnimation->setDuration(220);
    m_sidebarAnimation->setStartValue(startWidth);
    m_sidebarAnimation->setEndValue(targetWidth);
    connect(m_sidebarAnimation, &QPropertyAnimation::finished, this, [this, targetWidth] {
        m_sidebar->setFixedWidth(targetWidth);
        m_sidebarAnimation = nullptr;
    });
    m_sidebarAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    m_sidebarLayout->setContentsMargins(collapsed ? 6 : 18, 18, collapsed ? 6 : 18, 18);
    for (auto *widget : m_sidebar->findChildren<QLabel *>()) widget->setVisible(!collapsed);
    m_brandIcon->setVisible(true);
    m_brandName->setVisible(!collapsed);
    m_brandLayout->setAlignment(collapsed ? Qt::AlignCenter : Qt::AlignLeft);
    for (auto *button : m_pluginButtons) button->setProperty("collapsed", collapsed);
    m_overviewButton->setProperty("collapsed", collapsed);
    m_settingsButton->setProperty("collapsed", collapsed);
    m_sidebarToggle->setProperty("collapsed", collapsed);
    m_sidebar->style()->unpolish(m_sidebar);
    m_sidebar->style()->polish(m_sidebar);
    retranslateUi();
}
