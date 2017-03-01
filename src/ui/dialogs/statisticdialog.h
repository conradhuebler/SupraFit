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
    ~StatisticDialog();
    
    MCConfig getMCConfig();
    CVConfig getCVConfig();
    
public slots:
    void IncrementProgress(int time);
    
private:
    void setUi();
   
    
    QWidget *MonteCarloWidget();
    QWidget *ContinuousVariationWidget();
    
    QDoubleSpinBox *m_varianz_box, *m_cv_increment;
    QSpinBox *m_mc_steps, *m_cv_steps;
    QCheckBox *m_original;
    QPushButton *m_mc, *m_cv, *m_interrupt, *m_hide;
    QProgressBar *m_progress;
    QLabel *m_time_info;
    OptimizerFlagWidget *m_optim_flags;
    
    QMutex mutex;
    
    QWeakPointer<AbstractTitrationModel> m_model;
    
    int m_time;
    quint64 m_time_0;
     
    
private slots:
     void Pending();
     void Update();
     
signals:
    void CVStatistic();
    void MCStatistic();
    void Interrupt();
};

#endif // STATISTICDIALOG_H
