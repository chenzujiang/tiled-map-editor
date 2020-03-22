/*
 * patreondialog.cpp
 * Copyright 2015, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "patreondialog.h"
#include "ui_patreondialog.h"

#include "preferences.h"
#include "utils.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QMenu>

using namespace Tiled::Internal;

PatreonDialog::PatreonDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PatreonDialog)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
#endif

    ui->setupUi(this);

    resize(Utils::dpiScaled(size()));

    const QDate today(QDate::currentDate());

    auto laterMenu = new QMenu(this);
    laterMenu->addAction(tr("Remind me next week"))->setData(today.addDays(7));
    laterMenu->addAction(tr("Remind me next month"))->setData(today.addMonths(1));
    laterMenu->addAction(tr("Don't remind me"))->setData(QDate());
    ui->maybeLaterButton->setMenu(laterMenu);//以后再说button里面添加menu

    connect(ui->gotoPatreon, &QPushButton::clicked, this, &PatreonDialog::openPatreonPage);//访问众筹网站
    connect(ui->alreadyPatron, &QPushButton::clicked, this, &PatreonDialog::sayThanks);    //我已经成为赞助人
    connect(laterMenu, &QMenu::triggered, this, &PatreonDialog::maybeLater);
}

PatreonDialog::~PatreonDialog()
{
    delete ui;
}

void PatreonDialog::openPatreonPage()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("https://www.patreon.com/bjorn")));
}

void PatreonDialog::sayThanks()
{
    Preferences *prefs = Preferences::instance();
    prefs->setPatron(true);

    QMessageBox box(QMessageBox::NoIcon, tr("Thanks!"),//感谢您的支持,Tiled会越来越好
                    tr("Thanks a lot for your support! With your help Tiled will keep getting better."),
                    QMessageBox::Close, this);
    box.exec();

    close();
}
//构造函数中设置了时间了
void PatreonDialog::maybeLater(QAction *action)
{
    const QDate date = action->data().toDate();
    Preferences::instance()->setPatreonDialogReminder(date);
    close();
}
