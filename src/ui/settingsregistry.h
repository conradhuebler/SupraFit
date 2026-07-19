/*
 * SupraFit application-settings registry
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QVector>

class QObject;

/*! \brief Single source of truth for SupraFit's application settings. Claude Generated
 *
 * Historically every user setting had to be declared in four disconnected places (the
 * persisted-key list in SupraFitGui, the default-value if-chain in ReadSettings, the widget
 * creation in ConfigDialog and the write-back in ConfigDialog::accept). This registry collapses
 * that to one table: it owns the key, its default and — for user-visible settings — the metadata
 * ConfigDialog needs to build the widget. Persistence (QSettings) and defaults are derived from
 * it; the ConfigDialog is generated from it (special widgets flagged as Kind::Custom stay
 * hand-built while the registry still owns their key/default/persistence).
 *
 * The settings themselves remain plain dynamic properties on the QApplication instance
 * (qApp->property(key)); this registry only centralises their declaration. */
namespace SupraFitSettings {

/*! \brief Widget kind ConfigDialog uses to render a setting. */
enum class Kind {
    Bool, //!< QCheckBox
    Int, //!< QSpinBox
    Double, //!< QDoubleSpinBox
    String, //!< QLineEdit
    Custom //!< hand-built in the dialog (e.g. directory-mode radios); registry still owns key/default/persistence
};

/*! \brief One application setting: its key, default and (optional) dialog metadata. */
struct SettingDef {
    QString key; //!< qApp dynamic-property key
    QVariant defaultValue; //!< value applied when the key is missing from QSettings
    Kind kind = Kind::Bool; //!< widget type for the generated dialog
    QString group; //!< dialog tab (e.g. "General Settings"); empty => persisted-only, no widget
    QString label; //!< widget label
    QString tooltip; //!< widget tooltip
    QString note; //!< optional explanatory text shown as a label above the widget (section header / privacy note)
    double min = 0.0; //!< Int/Double minimum
    double max = 0.0; //!< Int/Double maximum
    double step = 0.0; //!< Double single step (0 => Qt default)
    int decimals = -1; //!< Double decimals (-1 => Qt default)
    QString suffix; //!< Int/Double suffix (e.g. "%")
    QString dependsOn; //!< key of a Bool setting; this widget is enabled only while that one is checked
    bool inDialog = true; //!< false => no auto-generated widget (persisted-only or hand-built)
    bool persisted = true; //!< false => lives only for the session, never written to QSettings
    bool resetIfZero = false; //!< Int only: also re-apply the default when the stored value is 0
};

/*! \brief The full settings table (built once, order = declaration order). */
const QVector<SettingDef>& registry();

/*! \brief Look up a setting by key, or nullptr if unknown. */
const SettingDef* find(const QString& key);

/*! \brief All persisted keys (used to save/load via QSettings). */
QStringList persistedKeys();

/*! \brief Set every registry key that is currently missing on \p app to its default.
 *  \return the keys that were missing and have just been initialised (e.g. to detect first start). */
QStringList applyDefaults(QObject* app);

}
