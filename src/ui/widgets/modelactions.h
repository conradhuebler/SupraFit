/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

class PushButton : public QPushButton {
    Q_OBJECT
public:
    inline PushButton(const QString& str)
        : QPushButton(str)
    {
        setStyleSheet("background-color: #77d740; color: black;");
    }
    inline ~PushButton() {}
};

class ModelActions : public QWidget {
    Q_OBJECT
public:
    ModelActions(QWidget* parent = 0);
    ~ModelActions();

    void EnableCharts(bool enable);
    void LocalEnabled(bool local_enabled);
    void setSimulation(bool simulation);
    /*! \brief The owner has a page list that reaches Statistics, Results and Charts, and a Fit menu that holds the
     * global search: the buttons for them are not needed (the model pane). A owner without such a list (the
     * meta model) keeps them. Claude Generated. */
    void setPageNavigation(bool pages);

private:
    void setUi();
    void resizeButtons();
    PushButton *m_switch, *m_minimize_single, *m_new_guess, *m_optim_config, *m_export, *m_import, *m_advanced;
    PushButton *m_statistics, *m_save, *m_simulate, *m_plots, *m_restore, *m_detailed, *m_charts;
    QWidget* m_second;
    bool m_simulation = false;
    bool m_page_nav = false;

signals:
    void LocalMinimize();
    void OptimizerSettings();
    void ImportConstants();
    void ExportConstants();
    void OpenAdvancedSearch();
    void TogglePlot3D();
    void TogglePlot();
    void ToggleStatisticDialog();
    void Save2File();
    void ToggleSearch();
    void NewGuess();
    void ExportSimModel();
    void Restore();
    void Detailed();
    void Charts();
};
