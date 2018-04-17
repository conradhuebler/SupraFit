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

#include "src/core/dataclass.h"
#include "src/core/models.h"

#include <QtCore/QPointer>

#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

#include <QtCharts/QChart>

struct OptimizerConfig;

class QToolButton;
class ModelWidget;
class DataWidget;
class QTabWidget;
class QPushButton;
class ChartWidget;
class QPlainTextEdit;
class QLabel;
class StatisticDialog;

class ToolButton : public QToolButton {
    Q_OBJECT

public slots:
    void ChangeColor(const QColor& color);
};

class TabWidget : public QTabWidget {
    Q_OBJECT

public:
    TabWidget(QWidget* parent = 0);
    inline ~TabWidget()
    {
        if (m_datawidget)
            delete m_datawidget;
    }
    void setDataTab(QPointer<DataWidget> datawidget);
    void addModelsTab(QPointer<ModelWidget> modelwidget);

private:
    QPointer<DataWidget> m_datawidget;
};

class MDHDockTitleBar : public QWidget {
    Q_OBJECT

public:
    MDHDockTitleBar();
    // ~MDHDockTitleBar();

    inline void setEnabled(bool enabled) { m_buttons->setEnabled(enabled); }
    void EnableBatch(bool enabled);
    void addToMenu(int IndependetCount);
    const QAction* lastAction() const { return m_last_action; }

private:
    QWidget* m_buttons;
    QPointer<QPushButton> m_add_nmr, m_add_kinetics, m_add_itc, m_optimize, m_statistics, m_close_all, m_hide, m_analyse;
    QVector<QPointer<QAction>> m_nmr_model, m_fl_model, m_kinetcs_model, m_itc_fixed_model, m_itc_flex_model;

    QAction *m_script_action, *m_last_action;

private slots:
    void PrepareAddModel();

signals:
    void AddModel();
    void CloseAll();
    void WGStatistic();
    void MCStatistic();
    void MoCoStatistic();
    void ReductionStatistic();
    void CVStatistic();
    void ShowStatistics();
    void OptimizeAll();
    void Compare();
};

class ModelDataHolder : public QWidget {
    Q_OBJECT

public:
    ModelDataHolder();
    ~ModelDataHolder();

    void setData(QSharedPointer<DataClass> data, QSharedPointer<ChartWrapper> wrapper);
    inline void setChartWidget(const QPointer<ChartWidget> chart) { m_charts = chart; }

    void setSettings(const OptimizerConfig& config);
    /*
     * Export currently open models to file
     */
    void SaveCurrentModels(const QString& file);
    /*
     * Export currently open models and the data table to file
     */
    void SaveWorkspace(const QString& file);

    bool CheckCrashFile();
    virtual QSize sizeHint() const { return QSize(800, 600); }
    MDHDockTitleBar* TitleBarWidget() const { return m_TitleBarWidget; }
public slots:
    /*
     * Add a new model to the workspace
     */
    void AddToWorkspace(const QJsonObject& object);
    /*
     * Overrides the very current model (opened tabe) with this model, if compatible
     */
    void LoadCurrentProject(const QJsonObject& object);
    /*
     * Make Datatable editable 
     */
    inline void EditTableAction(bool checked) { m_datawidget->setEditable(checked); }

private:
    QPointer<DataWidget> m_datawidget;
    QPointer<TabWidget> m_modelsWidget;

    QPointer<MDHDockTitleBar> m_TitleBarWidget;
    QPointer<ChartWidget> m_charts;
    QSharedPointer<DataClass> m_data;
    QWeakPointer<ChartWrapper> m_wrapper;
    QVector<QWeakPointer<AbstractModel>> m_models;
    QPointer<StatisticDialog> m_statistic_dialog;
    QVector<QPointer<ModelWidget>> m_model_widgets;
    void AddModel(int model);
    void ActiveBatch();

    OptimizerConfig m_config;

    int m_last_tab;

    void Json2Model(const QJsonObject& object);
    void Json2Model(const QJsonObject& object, SupraFit::Model model);
    void ActiveModel(QSharedPointer<AbstractModel> t, const QJsonObject& object = QJsonObject());
    int Runs(bool moco = false) const;
    bool m_history, m_allow_loop;

private slots:
    void AddModel();

    void RemoveTab(int i);
    void CreateCrashFile();
    void RemoveCrashFile();
    void SetProjectTabName();
    void CloseAll();
    void WGStatistic();
    void MCStatistic();
    void MoCoStatistic();
    void ReductionStatistic();
    void CVStatistic();
    void OptimizeAll();
    void Compare();
    void HideSubWindows(int index);
    inline void Interrupt() { m_allow_loop = false; }

signals:
    void ModelAdded(AbstractModel* model);
    void Message(const QString& str, int priority);
    void MessageBox(const QString& str, int priority);
    void InsertModel(const QJsonObject& model, int active);
    void InsertModel(const QJsonObject& model);
    void nameChanged();
    void recalculate();
};
