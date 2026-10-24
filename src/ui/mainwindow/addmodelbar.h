/*
 * Add-Model bar: the technique and the models that are added most, one click from the main window.
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

#include <QtWidgets/QToolBar>

#include "modelcatalog.h"

class MDHDockTitleBar;
class QToolButton;
class QButtonGroup;

/*! \brief A toolbar with the technique (NMR | UV/Vis | Fluorescence | ITC | Kinetics) as buttons side by side and the
 * models the operator adds most as buttons next to it: one click adds the model of the chosen technique to the
 * project on screen. "More..." opens the whole Model menu, the gear chooses what the bar shows.
 *
 * The bar can be moved, docked at any edge or floated, and follows the project on screen like the Data, Model and
 * Evaluation menus: it is off without a project that offers its tools. What it shows comes from ModelUsage (the most
 * used entries or a fixed list), what a click does from ModelCatalog; the buttons are rebuilt when the project or the
 * technique changes, not after every click, so they do not jump under the pointer. Claude Generated. */
class AddModelBar : public QToolBar {
    Q_OBJECT
public:
    explicit AddModelBar(QWidget* parent = nullptr);

    /*! \brief Follow the workspace \p workspace (null: none, the bar is off). */
    void setWorkspace(MDHDockTitleBar* workspace);

    ModelCatalog::Technique technique() const { return m_technique; }
    /*! \brief Choose the technique (it must fit the data of the workspace). */
    void setTechnique(ModelCatalog::Technique technique);
    /*! \brief Read the entries again (after the preferences or the counts changed). */
    void Refresh();

private:
    void RebuildTechniques();
    void RebuildQuick();
    void Add(const ModelCatalog::Entry& entry);

    QPointer<MDHDockTitleBar> m_workspace;
    ModelCatalog::Technique m_technique = ModelCatalog::Technique::NMR;
    int m_columns = 0;
    QWidget* m_technique_box = nullptr; ///< the technique buttons
    QWidget* m_quick_box = nullptr; ///< the quick entries
    QButtonGroup* m_technique_group = nullptr;
    QToolButton* m_more = nullptr;
    QToolButton* m_gear = nullptr;
};
