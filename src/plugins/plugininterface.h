#pragma once

#include <QObject>
#include <QLocale>
#include <QString>
#include <QWidget>

namespace Astraea {

class PluginInterface
{
public:
    virtual ~PluginInterface() = default;
    virtual QString id() const = 0;
    virtual QString name(const QLocale &locale) const = 0;
    virtual QString description(const QLocale &locale) const = 0;
    virtual QWidget *createWidget(QWidget *parent) = 0;
};

} // namespace Astraea

#define ASTRAEA_PLUGIN_IID "com.astraea.PluginInterface/1.0"
Q_DECLARE_INTERFACE(Astraea::PluginInterface, ASTRAEA_PLUGIN_IID)
