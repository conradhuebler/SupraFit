/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifndef MODELWIDGET_H
#define MODELWIDGET_H

#include "src/core/minimizer.h"

#include "src/core/dataclass.h"
#include "src/core/AbstractModel.h"

#include "src/ui/guitools/waiter.h"
#include "src/ui/dialogs/modeldialog.h"
#include "src/ui/mainwindow/chartwidget.h"

#include <QApplication>

#include <QtCore/QJsonObject>
#include <QtCore/QPointer>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QVBoxLayout>

#include <QtCharts/QLineSeries>
#include <QtDataVisualization>

class QSplitter;

class ModalDialog;
class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QVBoxLayout;
class QGridLayout;
class QCheckBox;
class LineSeries;
class AdvancedSearch;
class ChartView;
class _3DChartView;
class OptimizerFlagWidget;
class StatisticWidget;
class StatisticDialog;
class SpinBox;
class ModelElement;

struct ModelHistoryElement;
struct Charts;

class CVConfig;
class MCConfig;
class MoCoConfig;


class ModelWidget : public QWidget
{
    Q_OBJECT
    
public:
    ModelWidget(QSharedPointer< AbstractTitrationModel > model, Charts charts, QWidget *parent = 0);
    ~ModelWidget();
    virtual inline QSize sizeHint() const{ return QSize(250,50*m_sign_layout->count()); }
    QSharedPointer< AbstractTitrationModel > Model() const { return m_model; }
    QSharedPointer< Minimizer > getMinimizer() const { return m_minimizer; }
    
    void CVStatistic(CVConfig config);
    void MoCoStatistic(MoCoConfig config);
    void MCStatistic(MCConfig config);
    inline void setCheckbox(const QPointer<QCheckBox> checkbox) { m_toggled_box = checkbox; }
    inline bool isChecked() const { if(!m_toggled_box) return false; else return m_toggled_box->isChecked(); }
public slots:
    void LoadJson(const QJsonObject &object);
    void CVStatistic();
    void MCStatistic();
    void MoCoStatistic();
    void GlobalMinimize();
    void GlobalMinimizeLoose();
    void LocalMinimize();
    void HideAllWindows();
    
private:
    QSharedPointer< AbstractTitrationModel > m_model;
    QSharedPointer< Minimizer > m_minimizer;
    
    QVector<QPointer<SpinBox> > m_constants;
    QVector<QPointer<ModelElement > > m_model_elements;
    QPointer<AdvancedSearch> m_advancedsearch;
    QPointer<StatisticDialog> m_statistic_dialog;
    QPushButton *m_switch, *m_minimize_all;
    QLabel *m_bc_50, *m_converged_label; 
    QVBoxLayout *m_sign_layout;
    QGridLayout *m_layout;
    bool m_pending;
    QList<int > ActiveSignals();
    void DiscreteUI();
    void EmptyUI();
    void resizeButtons();
    void CollectParameters();
    void Data2Text();
    void Model2Text();
    void MinimizeModel(const OptimizerConfig &config);
    
    QWidget *m_model_widget;
    QSplitter *m_splitter;
    StatisticWidget *m_statistic_widget;
    QPointer<_3DChartView > _3dchart;
    QPointer<OptimizerFlagWidget> m_optim_flags;
    ModalDialog *m_statistic_result, *m_search_result, *m_table_result, *m_concentrations_result;
    bool m_statistic;
    Charts m_charts;
    QString m_logging;
    QList<QJsonObject> m_local_fits;
    QPointer<QCheckBox> m_toggled_box;
    
private slots:
    void Repaint();
    void CollectActiveSignals();
    void NewGuess();
    void ImportConstants();
    void ExportConstants();
    void setParameter();
    void OpenAdvancedSearch();
    void PlotFinished(int runtype);
    void MultiScanFinished();
    void TogglePlot3D();
    void TogglePlot();
    void ToggleConcentrations();
    void ToggleStatisticDialog();
    void ToggleSearchTable();
    void Save2File();
    void ExportSimModel();
    void FastConfidence();
    void SplitterResized();
    
public slots:
    void recalulate();
    void OptimizerSettings();
    void ChangeColor();
    
signals:
    void Update();
    void Warning(const QString &str, int i);
    void AddModel(const QJsonObject &json);
    void ToggleSeries(int);
    void IncrementProgress(int value);
    void Interrupt();
    void ColorChanged(const QColor &color);
};

#endif // MODELWIDGET_H
