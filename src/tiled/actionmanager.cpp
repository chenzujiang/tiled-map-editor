/*
 * actionmanager.cpp
 * Copyright 2016, Thorbj酶rn Lindeijer <bjorn@lindeijer.nl>
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

#include "actionmanager.h"

#include <QHash>
//http://techieliang.com/2017/12/649/
namespace Tiled {
namespace Internal {

class ActionManagerPrivate
{
public:
    QHash<Id, QAction*> mIdToAction;
};

static ActionManager *m_instance = nullptr;
static ActionManagerPrivate *d;


ActionManager::ActionManager(QObject *parent)
    : QObject(parent)
{
    m_instance = this;
    d = new ActionManagerPrivate;
}

ActionManager::~ActionManager()
{
    delete d;
}
/***
 * @projectName   Tiled
 * @brief         id map Action
 * @author        Casey.Chen
 * @date          2019-05-18
 */
void ActionManager::registerAction(QAction *action, Id id)//传来的字符串会在这里进行id的构造
{//如果哈希表包含有该id 返回true，!true进入断言。如果有该id被占用//Returns true if the hash contains an item with the key; otherwise returns false.
    Q_ASSERT_X(!d->mIdToAction.contains(id), "ActionManager::registerAction", "duplicate id");
    d->mIdToAction.insert(id, action);
}
//通过key 返回QAction
QAction *ActionManager::action(Id id)
{
    auto act = d->mIdToAction.value(id);//get value by key
    Q_ASSERT_X(act, "ActionManager::action", "unknown id");
    return act;
}

} // namespace Internal
} // namespace Tiled
