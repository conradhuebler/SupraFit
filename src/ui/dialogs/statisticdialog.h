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

#ifndef STATISTICDIALOG_H
#define STATISTICDIALOG_H

#include <QtCore/QMutex>

#include <QtWidgets/QDialog>

class QPushButton;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QProgressBar;
class OptimizerFlagWidget;

struct CVConfig;
struct MCConfig;


class StatisticDialog : public QDialog
{
    Q_OBJECT
public:
    StatisticDialog(QSharedPointer<AbstractTitrationModel> m_model, QWidget *parent = 0);
    StatisticDialog(QWidget *parent = 0);
    ~StatisticDialog();
    
    MCConfig getMCConfig();
    CVConfig getCVConfig();
    CVConfig getMoCoConfig();
    
    inline void setRuns(int runs) { m_runs = runs; }
public slots:
    void IncrementProgress(int time);
    void setMaximumSteps(int steps);
    
private:
    void setUi();
   
    
    QWidget *MonteCarloWidget();
    QWidget *ContinuousVariationWidget();
    QWidget *ModelComparison();
    
    QDoubleSpinBox *m_varianz_box, *m_cv_increment, *m_cv_maxerror, *m_moco_increment, *m_moco_maxerror;
    QSpinBox *m_mc_steps, *m_cv_steps, *m_moco_steps;
    QCheckBox *m_original, *m_bootstrap, *m_f_test, *m_moco_f_test;
    QPushButton *m_mc, *m_cv, *m_interrupt, *m_hide, *m_moco;
    QProgressBar *m_progress;
    QLabel *m_time_info, *m_error_info, *m_moco_error_info;
    OptimizerFlagWidget *m_optim_flags;
    
    QMutex mutex;
    
    QWeakPointer<AbstractTitrationModel> m_model;
    
    int m_time, m_runs;
    quint64 m_time_0;
     
    
private slots:
     void Pending();
     void Update();
     void EnableWidgets();
     void CalculateError();
     
signals:
    void CVStatistic();
    void MCStatistic();
    void MoCoStatistic();
    void Interrupt();
};

#endif // STATISTICDIALOG_H
