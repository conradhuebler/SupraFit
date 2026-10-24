/*
 * Add-Model bar: the models that are added most, one click away, the technique in a menu.
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#pragma once

#include <QtCore/QList>
#include <QtCore/QPointer>

#include <QtWidgets/QWidget>

#include "modelcatalog.h"

class MDHDockTitleBar;
class QMenu;
class QToolButton;

/*! \brief The models that are added most as buttons, for the project on screen: a click adds the model, the arrow
 * next to a button offers the other techniques.
 *
 * Each button is an entry (1:1, 1:1 / 1:2, ...). A click adds the model for the technique the entry was used with most
 * (NMR while it never was used, see ModelUsage::TechniqueFor); the menu of the arrow lists the techniques that
 * offer the entry for the data (NMR, UV/Vis, Fluorescence for two or more independent columns). An entry that has
 * no technique (a scripted preset, a model file, a model of one technique alone) is a plain button. The buttons are
 * rebuilt when the project changes or Refresh() is called, not after every click, so they do not jump under the pointer.
 *
 * The widget sits in the middle of the main toolbar and follows the project on screen like the Data, Model and
 * Evaluation menus: it shows nothing without a project that offers its tools. Claude Generated. */
class AddModelBar : public QWidget {
    Q_OBJECT
public:
    explicit AddModelBar(QWidget* parent = nullptr);

    /*! \brief Follow the workspace \p workspace (null: none, the bar shows nothing). */
    void setWorkspace(MDHDockTitleBar* workspace);
    /*! \brief Read the entries again (after the preferences or the counts changed). */
    void Refresh();

    /*! \brief The menu of a right click on the button of entry \p entryKey: customise the bar, remove the entry from a
     * fixed list, or fix the bar as it is. The caller shows it and deletes it. */
    QMenu* ContextMenu(const QString& entryKey);
    /*! \brief Open the dialog that customises the bar and apply what it returns. */
    void Customise();
    /*! \brief Turn what the bar shows now into a fixed list. */
    void FixAsItIs();

private:
    void Rebuild();
    void Add(const ModelCatalog::Entry& entry, ModelCatalog::Technique technique);

    QPointer<MDHDockTitleBar> m_workspace;
    int m_columns = 0;
    QWidget* m_quick_box = nullptr;
    QToolButton* m_gear = nullptr;
};
