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
    StatisticDialog(QWidget *parent = 0);
    ~StatisticDialog();
    
    MCConfig getMCConfig() const;
    CVConfig getCVConfig() const;
    
private:
    void setUi();
    QWidget *MonteCarloWidget();
    QWidget *ContinuousVariationWidget();
    
    QDoubleSpinBox *m_varianz_box, *m_cv_increment;
    QSpinBox *m_mc_steps, *m_cv_steps;
    QCheckBox *m_original;
    QPushButton *m_mc, *m_cv, *m_interrupt;
    QProgressBar *m_progress;
    
    OptimizerFlagWidget *m_optim_flags;
    
    QSharedPointer<AbstractTitrationModel> m_model;
    
signals:
    void CVStatistic();
    void MCStatistic();
};

#endif // STATISTICDIALOG_H
