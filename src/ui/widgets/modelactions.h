/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifndef MODELACTIONS_H
#define MODELACTIONS_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>

class PushButton : public QPushButton
{
     Q_OBJECT
public:
    inline PushButton(const QString &str) : QPushButton(str)
    {
        setStyleSheet("background-color: #77d740;");
    }
    
};

class ModelActions : public QWidget
{
    Q_OBJECT
public:
    ModelActions(QWidget *parent = 0);
    ~ModelActions();
private:
    void setUi();
    void resizeButtons();
    PushButton *m_switch, *m_minimize_single, *m_new_guess, *m_optim_config, *m_export, *m_import, *m_advanced, *m_plot_3d, *m_statistics, *m_concentration, *m_save; 
    
signals:
    void LocalMinimize();
    void OptimizerSettings();
    void ImportConstants();
    void ExportConstants();
    void OpenAdvancedSearch();
    void triggerPlot3D();
    void toggleStatisticDialog();
    void Save2File();
    void toggleConcentrations();
    void NewGuess();
};

#endif // MODELACTIONS_H
