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

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>

class QPushButton;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QProgressBar;
class OptimizerFlagWidget;
class QRadioButton;
class QTabWidget;

class WGSConfig;
class MCConfig;
class MoCoConfig;

class StatisticDialog : public QDialog
{
    Q_OBJECT
public:
    StatisticDialog(QSharedPointer<AbstractTitrationModel> m_model, QWidget *parent = 0);
    StatisticDialog(QWidget *parent = 0);
    ~StatisticDialog();
    
    MCConfig getMCConfig();
    WGSConfig getWGSConfig();
    MoCoConfig getMoCoConfig();
    
    inline void setRuns(int runs) { m_runs = runs; }
    virtual void setVisible(bool visible) override;
    inline bool UseChecked() const { return m_use_checked->isChecked(); }
    void updateUI();
    
public slots:
    void IncrementProgress(int time);
    void HideWidget();
    void ShowWidget();
    
private:
    void setUi();
    QString FOutput() const;
    
    QWidget *MonteCarloWidget();
    QWidget *ContinuousVariationWidget();
    QWidget *ModelComparison();
    
    QWidget *m_hide_widget;
    QTabWidget *m_tab_widget;
    QDoubleSpinBox *m_varianz_box, *m_cv_increment, *m_cv_maxerror, *m_moco_maxerror, *m_moco_box_multi, *m_moco_f_value, *m_cv_f_value;
    QSpinBox *m_mc_steps, *m_cv_steps, *m_moco_mc_steps;
    QCheckBox *m_original, *m_bootstrap, *m_cv_f_test, *m_moco_f_test, *m_use_checked;
    QPushButton *m_mc, *m_cv, *m_interrupt, *m_hide, *m_moco;
    QGroupBox *m_moco_global, *m_moco_monte_carlo;
    QProgressBar *m_progress;
    QLabel *m_time_info, *m_cv_error_info, *m_moco_error_info;
    OptimizerFlagWidget *m_optim_flags;
    QWidget *m_moco_widget;
    QMutex mutex;
    
    QWeakPointer<AbstractTitrationModel> m_model;
    
    int m_time, m_runs;
    quint64 m_time_0;
    qreal m_f_value, m_moco_max, m_cv_max;
    bool m_hidden;
    
private slots:
     void Update();
     void EnableWidgets();
     void CalculateError();
     
signals:
    void WGStatistic();
    void MCStatistic();
    void MoCoStatistic();
    void Interrupt();
    void setMaximumSteps(int steps);
};

#endif // STATISTICDIALOG_H
