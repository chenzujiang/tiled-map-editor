/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "treeviewcombobox.h"

#include <QWheelEvent>

namespace Tiled {
namespace Internal {

TreeViewComboBoxView::TreeViewComboBoxView(QWidget *parent)
    : QTreeView(parent)
{
    // TODO: Disable the root for all items (with a custom delegate?)
    setRootIsDecorated(false);
}
//调整宽度
void TreeViewComboBoxView::adjustWidth(int width)
{
    setMaximumWidth(width);
    setMinimumWidth(qMin(qMax(sizeHintForColumn(0), minimumSizeHint().width()), width));
}


TreeViewComboBox::TreeViewComboBox(QWidget *parent)
    : QComboBox(parent), m_skipNextHide(false)
{
    m_view = new TreeViewComboBoxView;
    m_view->setHeaderHidden(true);
    m_view->setItemsExpandable(true);
    //该属性包含用户是否可扩展这些项。
   // 这个属性包含用户是否可以交互式地展开和折叠项目。
    setView(m_view);
    m_view->viewport()->installEventFilter(this);
}
//索引上面
QModelIndex TreeViewComboBox::indexAbove(QModelIndex index) const
{
    do
        index = m_view->indexAbove(index);
    while (index.isValid() && !(model()->flags(index) & Qt::ItemIsSelectable));
    return index;
}

QModelIndex TreeViewComboBox::indexBelow(QModelIndex index) const
{
    do
        index = m_view->indexBelow(index);
    while (index.isValid() && !(model()->flags(index) & Qt::ItemIsSelectable));
    return index;
}

QModelIndex TreeViewComboBox::lastIndex(const QModelIndex &index) const
{
    if (index.isValid() && !m_view->isExpanded(index))
        return index;

    int rows = m_view->model()->rowCount(index);
    if (rows == 0)
        return index;
    return lastIndex(m_view->model()->index(rows - 1, 0, index));
}

void TreeViewComboBox::wheelEvent(QWheelEvent *e)
{
    QModelIndex index = m_view->currentIndex();
    if (e->delta() > 0)
        index = indexAbove(index);
    else if (e->delta() < 0)
        index = indexBelow(index);

    e->accept();
    if (!index.isValid())
        return;

    setCurrentModelIndex(index);

    // for compatibility we emit activated with a useless row parameter
    emit activated(index.row());
}

void TreeViewComboBox::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Up || e->key() == Qt::Key_PageUp) {
        setCurrentModelIndex(indexAbove(m_view->currentIndex()));
    } else if (e->key() == Qt::Key_Down || e->key() == Qt::Key_PageDown) {
        setCurrentModelIndex(indexBelow(m_view->currentIndex()));
    } else if (e->key() == Qt::Key_Home) {
        QModelIndex index = m_view->model()->index(0, 0);
        if (index.isValid() && !(model()->flags(index) & Qt::ItemIsSelectable))
            index = indexBelow(index);
        setCurrentModelIndex(index);
    } else if (e->key() == Qt::Key_End) {
        QModelIndex index = lastIndex(m_view->rootIndex());
        if (index.isValid() && !(model()->flags(index) & Qt::ItemIsSelectable))
            index = indexAbove(index);
        setCurrentModelIndex(index);
    } else {
        QComboBox::keyPressEvent(e);
        return;
    }

    e->accept();
}

void TreeViewComboBox::setCurrentModelIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    setRootModelIndex(model()->parent(index));
    QComboBox::setCurrentIndex(index.row());
    setRootModelIndex(QModelIndex());
    m_view->setCurrentIndex(index);
}

bool TreeViewComboBox::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && object == view()->viewport()) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        QModelIndex index = view()->indexAt(mouseEvent->pos());
        if (!view()->visualRect(index).contains(mouseEvent->pos()))
            m_skipNextHide = true;
    }
    return false;
}
//显示弹出
void TreeViewComboBox::showPopup()
{
    m_view->adjustWidth(topLevelWidget()->geometry().width());
    QComboBox::showPopup();
}

void TreeViewComboBox::hidePopup()
{
    if (m_skipNextHide)
        m_skipNextHide = false;
    else
        QComboBox::hidePopup();
}

TreeViewComboBoxView *TreeViewComboBox::view() const
{
    return m_view;
}

} // namespace Internal
} // namespace Tiled
