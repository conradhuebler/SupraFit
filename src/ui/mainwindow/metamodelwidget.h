/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/minimizer.h"

#include "src/ui/guitools/chartwrapper.h"

#include <QtCore/QSharedPointer>

#include <QtWidgets/QWidget>

#include "src/core/models/models.h"

class JobManager;
class MetaModelParameter;
class ModalDialog;
class ModelActions;
class ResultsDialog;
class StatisticWidget;

class QComboBox;
class QLineEdit;
class QPushButton;

class MetaModelWidget : public QWidget {
    Q_OBJECT

public:
    MetaModelWidget(QWidget* parent = 0);
    inline void setMetaModel(QSharedPointer<AbstractModel> model)
    {
        m_model = model;
        setUi();
        Model()->setFast(false);
        Model()->Calculate();
    }
    inline QPointer<MetaModel> Model() { return qobject_cast<MetaModel*>(m_model.data()); }

    void LinkModel(QSharedPointer<AbstractModel> model, QColor color)
    {
        m_linked_charts.insert(model, color);
    }

    void UpdateColor(QSharedPointer<AbstractModel> model, QColor color)
    {
        m_linked_charts[model] = color;
    }
public slots:
    void LoadJson(const QJsonObject& object);

private:
    void setUi();
    void History();

    QHash<QSharedPointer<AbstractModel>, QColor> m_linked_charts;

    QSharedPointer<AbstractModel> m_model;
    QPushButton *m_minimize, *m_calculate, *m_history;
    QComboBox* m_type;
    ModelActions* m_actions;
    ModalDialog *m_dialogs, *m_table_result;
    StatisticWidget* m_statistic_widget;
    ResultsDialog* m_results;
    MetaModelParameter* m_metamodelparameter;
    QList<QJsonObject> m_fast_confidence;
    JobManager* m_jobmanager;
    QLineEdit* m_project_name;
    ChartWrapper* m_wrapper;

    void LoadStatistic(const QJsonObject& data);
    void FastConfidence();
    QVector<OptimisationHistory> m_optimisationhistory;

private slots:
    void Calculate();
    void Minimize();
    void OpenAdvancedSearch();
    void NewGuess();
    void ImportConstants();
    void ExportConstants();
    void ToggleStatisticDialog();
    void TogglePlot();
    void Detailed();
    void OptimizerSettings();

signals:
    void Interrupt();
    void Finished();
    void IncrementProgress(int progess);
    void MaximumSteps(int steps);
    void Message(const QString& str, int priority);
    void Warning(const QString& str, int priority);
};
