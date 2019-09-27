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

#include "src/capabilities/abstractsearchclass.h"
#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/resampleanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"

#include "src/core/dataclass.h"
#include "src/core/models.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/mainwindow/datawidget.h"

#include <QtCore/QPointer>

#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

#include <QtCharts/QChart>

struct OptimizerConfig;

class QToolButton;
class MetaModelWidget;
class ModelWidget;
class DataWidget;
class QTabWidget;
class QPushButton;
class ChartWidget;
class QPlainTextEdit;
class QLabel;
class StatisticDialog;
class CompareDialog;

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
    void setMetaTab(QPointer<MetaModelWidget> datawidget);

    void addModelsTab(QPointer<ModelWidget> modelwidget);

private:
    QPointer<DataWidget> m_datawidget;
    QPointer<MetaModelWidget> m_metamodelwidget;
};

class MDHDockTitleBar : public QWidget {
    Q_OBJECT

public:
    MDHDockTitleBar();
    // ~MDHDockTitleBar();

    inline void setEnabled(bool enabled) { m_buttons->setEnabled(enabled); }

    void EnableBatch(bool enabled);
    void addToMenu(int IndependetCount);
    void HideModelTools();

    const QAction* lastAction() const { return m_last_action; }
    int Model() const { return m_model_choosen; }

private:
    QWidget* m_buttons;
    QPointer<QPushButton> m_edit_data, m_add_nmr, m_add_kinetics, m_add_itc, m_any_model, m_optimize, m_statistics, m_close_all, m_hide, m_analyse;
    QVector<QPointer<QAction>> m_nmr_model, m_fl_model, m_kinetcs_model, m_itc_fixed_model, m_itc_flex_model;

    QAction *m_script_action, *m_last_action;
    int m_model_choosen = 404;
private slots:
    void PrepareAddModel();

signals:
    void AddModel();
    void CloseAll();
    void ShowStatistics();
    void OptimizeAll();
    void Compare();
    void EditData();

};

class ModelDataHolder : public QWidget {
    Q_OBJECT

public:
    ModelDataHolder();
    ~ModelDataHolder();

    void setData(QSharedPointer<DataClass> data, QSharedPointer<ChartWrapper> wrapper);
    inline void setChartWidget(const QPointer<ChartWidget> chart) { m_charts = chart; }

    void setSettings(const QJsonObject& config);
    /*
     * Export currently open models to file
     */
    void SaveCurrentModels(const QString& file);
    /*
     * Export currently open models and the data table to file
     */
    QJsonObject SaveWorkspace();
    QJsonObject SaveModel(int index);

    virtual QSize sizeHint() const { return QSize(800, 600); }
    MDHDockTitleBar* TitleBarWidget() const { return m_TitleBarWidget; }

    inline int ModelCount() const { return m_models.size(); }
    inline QPointer<AbstractModel> Model(int index) const { return m_models[index].data(); }

    void setCurrentTab(int index);
    void addMetaModel(QSharedPointer<AbstractModel> t);

    inline QWeakPointer<ChartWrapper> getChartWrapper() const { return m_wrapper; }

    QString Compare() const;

    inline QPointer<ModelWidget> RecentModel() { return m_last_modelwidget; }
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
    // inline void EditTableAction(bool checked) { m_datawidget->setEditable(checked); }

    void RemoveTab(int i);

    /* Close all open projects */
    void CloseAll();

    /* Close all open projects */
    void CloseAllForced();

private:
    QPointer<DataWidget> m_datawidget;
    QPointer<TabWidget> m_modelsWidget;
    QPointer<MetaModelWidget> m_metamodelwidget;

    QPointer<MDHDockTitleBar> m_TitleBarWidget;
    QPointer<ChartWidget> m_charts;
    QWeakPointer<ChartWrapper> m_wrapper;
    QWeakPointer<DataClass> m_data;
    QVector<QWeakPointer<AbstractModel>> m_models;
    QPointer<StatisticDialog> m_statistic_dialog;
    QPointer<CompareDialog> m_compare_dialog;
    QVector<QPointer<ModelWidget>> m_model_widgets;
    void AddModel(int model);
    void ActiveBatch();

    QJsonObject m_config;

    int m_last_tab = 0;

    void Json2Model(const QJsonObject& object);
    void ActiveModel(QSharedPointer<AbstractModel> t, const QJsonObject& object = QJsonObject(), bool readonly = false);
    bool m_history, m_allow_loop;
    double m_ReductionCutoff = 0;

    QPointer<ModelWidget> m_last_modelwidget;
private slots:
    void AddModel();
    void SetProjectTabName();
    void RunJobs(const QJsonObject& job);

    void OptimizeAll();

    void CompareAIC();
    void CompareCV();
    void CompareReduction();
    void CompareMC();

    void EditData();
    void HideSubWindows(int index);
    inline void Interrupt() { m_allow_loop = false; }

signals:
    // void ModelAdded(AbstractModel* model);
    void ModelAdded();
    void ModelRemoved();
    void MessageBox(const QString& str, int priority);
    void InsertModel(const QJsonObject& model, int active);
    void InsertModel(const QJsonObject& model);
    void nameChanged();
    void recalculate();
    void Message(const QString& str, int priority);
    void Warning(const QString& str, int priority);
};
