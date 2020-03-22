/*
 * pluginlistmodel.cpp
 * Copyright 2015, Thorbj酶rn Lindeijer <bjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "pluginlistmodel.h"

#include "pluginmanager.h"

#include <QFileInfo>
#include <QIcon>
#include <QDebug>
namespace Tiled {
namespace Internal {

PluginListModel::PluginListModel(QObject *parent)
    : QAbstractListModel(parent)
    , mPluginIcon(QIcon(QLatin1String(":images/16x16/plugin.png")))
    , mPluginErrorIcon(QIcon(QLatin1String(":images/16x16/error.png")))
{
    QPixmap pluginIcon2x(QLatin1String(":images/32x32/plugin.png"));
    pluginIcon2x.setDevicePixelRatio(2);
    mPluginIcon.addPixmap(pluginIcon2x);

    QPixmap pluginErrorIcon2x(QLatin1String(":images/32x32/error.png"));
    pluginErrorIcon2x.setDevicePixelRatio(2);
    mPluginErrorIcon.addPixmap(pluginErrorIcon2x);
}

int PluginListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())//Returns true if this model index is valid; otherwise returns false.
        return 0;
    qDebug()<<PluginManager::instance()->plugins().size();
    qDebug()<<parent.column();
    return PluginManager::instance()->plugins().size();
}
//https://www.cnblogs.com/takeaction/p/3661862.html
QVariant PluginListModel::data(const QModelIndex &index, int role) const
{
    auto &plugin = PluginManager::instance()->plugins().at(index.row());//从插件管理者哪里获取插件文件对象

    switch (role) {
    case Qt::CheckStateRole://This role is used to obtain the checked state of an item. (Qt::CheckState)
        if (plugin.defaultEnable && plugin.state == PluginDefault)
            return Qt::Checked;
        else if (plugin.state == PluginEnabled || plugin.state == PluginStatic)
            return Qt::Checked;
        else
            return Qt::Unchecked;
    case Qt::DecorationRole: {//The data to be rendered as a decoration in the form of an icon.
        if (plugin.hasError())
            return mPluginErrorIcon.pixmap(16);
        else
            return mPluginIcon.pixmap(16, plugin.instance ? QIcon::Normal : QIcon::Disabled);
    }
    case Qt::DisplayRole://The key data to be rendered in the form of text.
        qDebug()<<plugin.fileName();
        return QFileInfo(plugin.fileName()).fileName();
    case Qt::ToolTipRole: {//The data displayed in the item's tooltip.
        if (plugin.hasError())
            return plugin.errorString();

        return plugin.fileName();
    }
    }

    return QVariant();
}

Qt::ItemFlags PluginListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags rc = QAbstractListModel::flags(index);
    rc |= Qt::ItemIsUserCheckable;//支持用户可以选中和不选中

    auto &plugin = PluginManager::instance()->plugins().at(index.row());
    if (plugin.state == PluginStatic)
        rc &= ~Qt::ItemIsEnabled;//如果静态插件,保证它是失能的

    return rc;
}

bool PluginListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    auto &plugin = PluginManager::instance()->plugins().at(index.row());

    if (role == Qt::CheckStateRole) {
        Qt::CheckState c = static_cast<Qt::CheckState>(value.toInt());
        const bool enabled = c == Qt::Checked;

        if (plugin.state != (enabled ? PluginEnabled : PluginDisabled))
            emit setPluginEnabled(QFileInfo(plugin.fileName()).fileName(), enabled);

        return true;
    }

    return false;
}

} // namespace Internal
} // namespace Tiled

