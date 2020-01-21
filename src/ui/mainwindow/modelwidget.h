/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

#include "src/core/minimizer.h"

#include "src/ui/dialogs/modaldialog.h"
#include "src/ui/guitools/waiter.h"
#include "src/ui/mainwindow/chartwidget.h"

#include <QtCharts/QLineSeries>

#include <QtCore/QJsonObject>
#include <QtCore/QPointer>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>


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
class JobManager;
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
    ModelWidget(QSharedPointer<AbstractModel> model, Charts charts, bool readonly = false, QWidget* parent = NULL);
    virtual ~ModelWidget() override;
    virtual inline QSize sizeHint() const override { return QSize(125, 25); }
    QSharedPointer<AbstractModel> Model() const { return m_model; }
    QSharedPointer<Minimizer> getMinimizer() const { return m_minimizer; }

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
    QColor ActiveColor() const;
    void setJob(const QJsonObject& job);
    JobManager* Jobs() const { return m_jobmanager; }

public slots:
    void LoadJson(const QJsonObject& object);

    void GlobalMinimize();
    void GlobalMinimizeLoose();
    void LocalMinimize();
    void HideAllWindows();

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
    QLabel* m_converged_label;
    OptionsWidget* m_model_options_widget;
    QVBoxLayout* m_layout;
    bool m_pending;
    QList<int> ActiveSignals();
    void resizeButtons();
    void CollectParameters();
    void Data2Text();
    void Model2Text();
    void MinimizeModel(const QJsonObject& config);
    void LoadStatistic(const QJsonObject& data);

    QVBoxLayout* m_sign_layout;
    QLineEdit* m_model_name;
    QWidget* m_model_widget;
    QSplitter* m_splitter;
    StatisticWidget* m_statistic_widget;
    ModalDialog *m_dialogs, *m_charts_dialogs;
    ResultsDialog* m_results;

    bool m_statistic, m_val_readonly, m_SetUpFinished = false;
    Charts m_charts;
    QString m_logging;
    QPointer<QCheckBox> m_toggled_box, m_global_box, m_local_box;
    QPointer<LocalParameterWidget> m_local_parameter;
    QJsonObject m_last_model;
    QList<QJsonObject> m_fast_confidence;

    JobManager* m_jobmanager;

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
    void Save2File();
    void ExportSimModel();
    void FastConfidence();
    void SplitterResized();
    void Restore();
    void Detailed();
    void ChartUpdated(const QString& str);

public slots:
    void recalculate();
    void OptimizerSettings();
    void ChangeColor();
    void setColor(const QColor& color);
    void Interrupt();

signals:
    void Update();
    void Warning(const QString& str, int i);
    void AddModel(const QJsonObject& json);
    void ToggleSeries(int);
    void IncrementProgress(int value);
    void MaximumSteps(int value);
    void ColorChanged(const QColor& color);
    void started();
    void finished();
    void Message(const QString& str);
};
