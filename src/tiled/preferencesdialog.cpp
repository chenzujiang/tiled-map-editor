/*
 * preferencesdialog.cpp
 * Copyright 2009-2010, Thorbj酶rn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "autoupdater.h"
#include "languagemanager.h"
#include "pluginlistmodel.h"
#include "preferences.h"

#include <QSortFilterProxyModel>

#include "qtcompat_p.h"

using namespace Tiled;
using namespace Tiled::Internal;

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
    , mUi(new Ui::PreferencesDialog)
    , mLanguages(LanguageManager::instance()->availableLanguages())
{
    mUi->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
#endif

#if defined(QT_NO_OPENGL)
    mUi->openGL->setEnabled(false);
#else
    mUi->openGL->setEnabled(true);
#endif

    for (const QString &name : qAsConst(mLanguages)) {
        //Qlocale类在不同语言的数字及其字符串表示形式之间进行转换。
        //Qlocale在其构造函数中使用语言/国家对初始化，并提供与qstring类似的从数字到字符串和从字符串到数字的转换函数。
        QLocale locale(name);
        QString string = QString(QLatin1String("%1 (%2)"))
            .arg(QLocale::languageToString(locale.language()))
            .arg(QLocale::countryToString(locale.country()));
        mUi->languageCombo->addItem(string, name);
    }

    mUi->languageCombo->model()->sort(0);
    mUi->languageCombo->insertItem(0, tr("System default"));

    mUi->styleCombo->addItems(QStringList()
                              << QApplication::translate("PreferencesDialog", "Native")
                              << QApplication::translate("PreferencesDialog", "Tiled Fusion"));

    mUi->styleCombo->setItemData(0, Preferences::SystemDefaultStyle);
    mUi->styleCombo->setItemData(1, Preferences::TiledStyle);

    PluginListModel *pluginListModel = new PluginListModel(this);
    QSortFilterProxyModel *pluginProxyModel = new QSortFilterProxyModel(this);
    pluginProxyModel->setSortLocaleAware(true);//This property holds the local aware setting used for comparing strings when sorting
    pluginProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    pluginProxyModel->setSourceModel(pluginListModel);//Sets the given sourceModel to be processed by the proxy model.
    pluginProxyModel->sort(0);
/*  This function will create and set a new selection model, replacing any model that was previously set with setSelectionModel().
 *  However, the old selection model will not be deleted as it may be shared between several views.
 *  We recommend that you delete the old selection model if it is no longer required.
 *  This is done with the following code:
 *  QItemSelectionModel *m = view->selectionModel();
 *  view->setModel(new model);
 *  delete m;
 */
    mUi->pluginList->setModel(pluginProxyModel);

    fromPreferences();

    auto *preferences = Preferences::instance();

    connect(mUi->enableDtd, &QCheckBox::toggled,
            preferences, &Preferences::setDtdEnabled);
    connect(mUi->reloadTilesetImages, &QCheckBox::toggled,
            preferences, &Preferences::setReloadTilesetsOnChanged);
    connect(mUi->openLastFiles, &QCheckBox::toggled,
            preferences, &Preferences::setOpenLastFilesOnStartup);
    connect(mUi->safeSaving, &QCheckBox::toggled,
            preferences, &Preferences::setSafeSavingEnabled);

    connect(mUi->languageCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::languageSelected);
    connect(mUi->gridColor, &ColorButton::colorChanged,
            preferences, &Preferences::setGridColor);
    connect(mUi->gridFine, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            preferences, &Preferences::setGridFine);
    connect(mUi->objectLineWidth, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            preferences, &Preferences::setObjectLineWidth);
    connect(mUi->openGL, &QCheckBox::toggled,
            preferences, &Preferences::setUseOpenGL);
    connect(mUi->wheelZoomsByDefault, &QCheckBox::toggled,
            preferences, &Preferences::setWheelZoomsByDefault);

    connect(mUi->styleCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PreferencesDialog::styleComboChanged);
    connect(mUi->baseColor, &ColorButton::colorChanged,
            preferences, &Preferences::setBaseColor);
    connect(mUi->selectionColor, &ColorButton::colorChanged,
            preferences, &Preferences::setSelectionColor);

    connect(mUi->autoUpdateCheckBox, &QPushButton::toggled,
            this, &PreferencesDialog::autoUpdateToggled);
    connect(mUi->checkForUpdate, &QPushButton::clicked,
            this, &PreferencesDialog::checkForUpdates);

    connect(pluginListModel, &PluginListModel::setPluginEnabled,
            preferences, &Preferences::setPluginEnabled);
}

PreferencesDialog::~PreferencesDialog()
{
    delete mUi;
}

void PreferencesDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange: {
            mUi->retranslateUi(this);
            retranslateUi();
        }
        break;
    default:
        break;
    }
}

void PreferencesDialog::languageSelected(int index)
{
    const QString language = mUi->languageCombo->itemData(index).toString();
    Preferences *prefs = Preferences::instance();
    prefs->setLanguage(language);
}

void PreferencesDialog::fromPreferences()
{
    const Preferences *prefs = Preferences::instance();

    mUi->reloadTilesetImages->setChecked(prefs->reloadTilesetsOnChange());
    mUi->enableDtd->setChecked(prefs->dtdEnabled());
    mUi->openLastFiles->setChecked(prefs->openLastFilesOnStartup());
    mUi->safeSaving->setChecked(prefs->safeSavingEnabled());
    if (mUi->openGL->isEnabled())
        mUi->openGL->setChecked(prefs->useOpenGL());
    mUi->wheelZoomsByDefault->setChecked(prefs->wheelZoomsByDefault());

    // Not found (-1) ends up at index 0, system default
    int languageIndex = mUi->languageCombo->findData(prefs->language());
    if (languageIndex == -1)
        languageIndex = 0;
    mUi->languageCombo->setCurrentIndex(languageIndex);
    mUi->gridColor->setColor(prefs->gridColor());
    mUi->gridFine->setValue(prefs->gridFine());
    mUi->objectLineWidth->setValue(prefs->objectLineWidth());

    int styleComboIndex = mUi->styleCombo->findData(prefs->applicationStyle());
    if (styleComboIndex == -1)
        styleComboIndex = 1;

    mUi->styleCombo->setCurrentIndex(styleComboIndex);
    mUi->baseColor->setColor(prefs->baseColor());
    mUi->selectionColor->setColor(prefs->selectionColor());
    bool systemStyle = prefs->applicationStyle() == Preferences::SystemDefaultStyle;
    mUi->baseColor->setEnabled(!systemStyle);
    mUi->baseColorLabel->setEnabled(!systemStyle);
    mUi->selectionColor->setEnabled(!systemStyle);
    mUi->selectionColorLabel->setEnabled(!systemStyle);

    // Auto-updater settings
    auto updater = AutoUpdater::instance();
    mUi->autoUpdateCheckBox->setEnabled(updater);
    mUi->checkForUpdate->setEnabled(updater);
    if (updater) {
        bool autoUpdateEnabled = updater->automaticallyChecksForUpdates();
        auto lastChecked = updater->lastUpdateCheckDate();
        auto lastCheckedString = lastChecked.toString(Qt::DefaultLocaleLongDate);
        mUi->autoUpdateCheckBox->setChecked(autoUpdateEnabled);
        mUi->lastAutoUpdateCheckLabel->setText(tr("Last checked: %1").arg(lastCheckedString));
    }
}

void PreferencesDialog::retranslateUi()
{
    mUi->languageCombo->setItemText(0, tr("System default"));

    mUi->styleCombo->setItemText(0, QApplication::translate("PreferencesDialog", "Native"));
    mUi->styleCombo->setItemText(1, QApplication::translate("PreferencesDialog", "Tiled Fusion"));
}

void PreferencesDialog::styleComboChanged()
{
    Preferences *prefs = Preferences::instance();
    int style = mUi->styleCombo->currentData().toInt();

    prefs->setApplicationStyle(static_cast<Preferences::ApplicationStyle>(style));

    bool systemStyle = prefs->applicationStyle() == Preferences::SystemDefaultStyle;
    mUi->baseColor->setEnabled(!systemStyle);
    mUi->baseColorLabel->setEnabled(!systemStyle);
    mUi->selectionColor->setEnabled(!systemStyle);
    mUi->selectionColorLabel->setEnabled(!systemStyle);
}

void PreferencesDialog::autoUpdateToggled(bool checked)
{
    if (auto updater = AutoUpdater::instance())
        updater->setAutomaticallyChecksForUpdates(checked);
}

void PreferencesDialog::checkForUpdates()
{
    if (auto updater = AutoUpdater::instance()) {
        updater->checkForUpdates();
        // todo: do something with the last checked label
    }
}
