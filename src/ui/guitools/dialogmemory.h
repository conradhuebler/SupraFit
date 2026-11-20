/*
 * DialogMemory - remembered geometry, splitters and last values of the import dialogs
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
 * Claude Generated (UX package 2a of the import dialogs).
 */

#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVariant>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QSplitter;
class QWidget;

/*! \brief Settings memory of a modal import dialog (Claude Generated).
 *
 * Everything lives in QSettings under "dialogs/<name>/...":
 * - "geometry": size and position (a QRect), written when the dialog is hidden (exec() ends), applied by
 *   restoreGeometry() only if the stored window lies on a screen that exists now (its top strip must be
 *   reachable) and is not smaller than the dialog's minimum size; else the dialog keeps the size the
 *   constructor gave it. WindowHost does the same for windows shown with show().
 * - "splitter/<id>": state of every splitter given to addSplitter(), written with the geometry.
 * - further entries (last values of the options) through value()/setValue() and the checked readers,
 *   which ignore a stored value of the wrong type or outside the limits of the control.
 *
 * The dialog decides which values are defaults of a new series; a stored project always wins (the dialog
 * applies the memory before it reads a project). Tests isolate the settings with QSettings::setPath and
 * an organisation name of their own. */
class DialogMemory : public QObject {
public:
    /*! \brief Memory of dialog under "dialogs/<name>"; parent = dialog, saves when the dialog is hidden. */
    DialogMemory(QWidget* dialog, const QString& name);

    /*! \brief Full settings key of an entry ("dialogs/<name>/<entry>"). */
    QString key(const QString& entry) const;
    QVariant value(const QString& entry, const QVariant& fallback = QVariant()) const;
    void setValue(const QString& entry, const QVariant& value);
    bool contains(const QString& entry) const;

    /*! \brief Applies the stored geometry if it fits a present screen; true if applied. */
    bool restoreGeometry();
    /*! \brief Writes geometry and splitter states (also done when the dialog is hidden). */
    void save();
    /*! \brief Restores the state of splitter now and keeps it with save(). */
    void addSplitter(QSplitter* splitter, const QString& id);

    /*! \brief Is geometry (screen coordinates of the window) placed so the user can reach it? Its top strip
     * (at least 100 x 30 px, or the whole width if smaller) must lie on one of the available screens. */
    static bool fitsAScreen(const QRect& geometry);

    /*! \brief Stored double within [min, max]; false (out unchanged) if missing, not a number or outside. */
    bool readDouble(const QString& entry, double min, double max, double* out) const;
    /*! \brief Stored integer within [min, max]; false if missing, not an integer or outside. */
    bool readInt(const QString& entry, int min, int max, int* out) const;
    /*! \brief Stored bool ("true"/"false"/"1"/"0" or a bool); false for anything else. */
    bool readBool(const QString& entry, bool* out) const;

    /*! \brief Sets the control to the stored value if it is valid for the control's limits; true if set. */
    bool restore(QDoubleSpinBox* box, const QString& entry) const;
    bool restore(QSpinBox* box, const QString& entry) const;
    /*! \brief Stored index of a combo box (0 .. count-1). */
    bool restore(QComboBox* box, const QString& entry) const;
    bool restore(QCheckBox* box, const QString& entry) const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QPointer<QWidget> m_dialog;
    QString m_name;
    QList<QPair<QPointer<QSplitter>, QString>> m_splitters;
    bool m_shown = false; ///< the dialog was on screen: only then its geometry is worth keeping
};
