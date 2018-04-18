/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/capabilities/reductionanalyse.h"

#include "src/core/AbstractModel.h"
#include "src/core/dataclass.h"
#include "src/core/minimizer.h"

#include "src/ui/dialogs/modeldialog.h"
#include "src/ui/guitools/waiter.h"
#include "src/ui/mainwindow/chartwidget.h"

#include <QApplication>

#include <QtCore/QJsonObject>
#include <QtCore/QPointer>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <QtCharts/QLineSeries>

class AbstractSearchClass;

class QCheckBox;
class QDoubleSpinBox;
class QGridLayout;
class QLineEdit;
class QSplitter;
class QPushButton;
class QVBoxLayout;

class AdvancedSearch;
class ChartView;
class LineSeries;
class LocalParameterWidget;
class ModelActions;
class ModalDialog;
class ModelElement;
class OptimizerFlagWidget;
class OptionsWidget;
class ResultsDialog;
class SpinBox;
class StatisticWidget;
class StatisticDialog;
class SystemParameterWidget;

struct ModelHistoryElement;
struct Charts;

class WGSConfig;
class MCConfig;
class MoCoConfig;

class ModelWidget : public QWidget {
    Q_OBJECT

public:
    ModelWidget(QSharedPointer<AbstractModel> model, Charts charts, QWidget* parent = 0);
    virtual ~ModelWidget();
    virtual inline QSize sizeHint() const { return QSize(250, 50 * m_sign_layout->count()); }
    QSharedPointer<AbstractModel> Model() const { return m_model; }
    QSharedPointer<Minimizer> getMinimizer() const { return m_minimizer; }

    void WGStatistic(WGSConfig config);
    void MoCoStatistic(MoCoConfig config);
    void MCStatistic(MCConfig config);
    void CVAnalyse(ReductionAnalyse::CVType type);
    inline void setCheckbox(const QPointer<QCheckBox> checkbox) { m_toggled_box = checkbox; }
    inline bool isChecked() const
    {
        if (!m_toggled_box)
            return false;
        else
            return m_toggled_box->isChecked();
    }
    inline Charts Chart() const { return m_charts; }
    void setColorList(const QString& str);
    QString Keys() const;
    void setKeys(const QString& str);

public slots:
    void LoadJson(const QJsonObject& object);
    void WGStatistic();
    void MCStatistic();
    void MoCoStatistic();
    void CVAnalyse();
    void GlobalMinimize();
    void GlobalMinimizeLoose();
    void LocalMinimize();
    void HideAllWindows();
    void DoReductionAnalyse();

private:
    QSharedPointer<AbstractModel> m_model;
    QSharedPointer<Minimizer> m_minimizer;

    QVector<QPointer<SpinBox>> m_constants;
    QVector<QPointer<ModelElement>> m_model_elements;
    QPointer<AdvancedSearch> m_advancedsearch;
    QPointer<StatisticDialog> m_statistic_dialog;
    ModelActions* m_actions;
    QPushButton* m_minimize_all;
    QCheckBox* m_readonly;
    QLabel *m_bc_50, *m_converged_label;
    OptionsWidget* m_model_options_widget;
    QGridLayout* m_layout;
    bool m_pending;
    QList<int> ActiveSignals();
    void DiscreteUI();
    void EmptyUI();
    void resizeButtons();
    void CollectParameters();
    void Data2Text();
    void Model2Text();
    void MinimizeModel(const OptimizerConfig& config);
    void LoadStatistics();
    void LoadStatistic(const QJsonObject& data, const QList<QJsonObject>& models = QList<QJsonObject>());

    QVBoxLayout* m_sign_layout;

    QWidget* m_model_widget;
    QSplitter* m_splitter;
    StatisticWidget* m_statistic_widget;
    QPointer<OptimizerFlagWidget> m_optim_flags;
    ModalDialog *m_statistic_result, *m_search_result, *m_table_result;
    ResultsDialog* m_results;

    bool m_statistic;
    Charts m_charts;
    QString m_logging;
    QList<QJsonObject> m_local_fits;
    QPointer<QCheckBox> m_toggled_box;
    QPointer<LocalParameterWidget> m_local_parameter;
    QJsonObject m_last_model;
    QList<QJsonObject> m_fast_confidence;

private slots:
    void Repaint();
    void CollectActiveSignals();
    void NewGuess();
    void ImportConstants();
    void ExportConstants();
    void setParameter();
    void OpenAdvancedSearch();
    void MultiScanFinished();
    void TogglePlot();
    void ToggleStatisticDialog();
    void ToggleSearchTable();
    void Save2File();
    void ExportSimModel();
    void FastConfidence();
    void SplitterResized();
    void Restore();
    void Detailed();

public slots:
    void recalculate();
    void OptimizerSettings();
    void ChangeColor();
    void setColor(const QColor& color);

signals:
    void Update();
    void Warning(const QString& str, int i);
    void AddModel(const QJsonObject& json);
    void ToggleSeries(int);
    void IncrementProgress(int value);
    void Interrupt();
    void ColorChanged(const QColor& color);
};
