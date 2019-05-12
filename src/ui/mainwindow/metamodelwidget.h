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

#include <QtCore/QSharedPointer>

#include <QtWidgets/QWidget>

#include "src/core/models.h"

class JobManager;
class MetaModelParameter;
class ModalDialog;
class ModelActions;
class ResultsDialog;
class StatisticWidget;

class QComboBox;
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

public slots:
    void LoadJson(const QJsonObject& object);

private:
    void setUi();

    QSharedPointer<AbstractModel> m_model;
    QPushButton* m_minimize;
    QComboBox* m_type;
    ModelActions* m_actions;
    ModalDialog *m_dialogs, *m_table_result;
    StatisticWidget* m_statistic_widget;
    ResultsDialog* m_results;
    MetaModelParameter* m_metamodelparameter;
    QList<QJsonObject> m_fast_confidence;
    JobManager* m_jobmanager;

    void LoadStatistic(const QJsonObject& data);
    void FastConfidence();

private slots:
    void Minimize();
    void OpenAdvancedSearch();
    void NewGuess();
    void ImportConstants();
    void ExportConstants();
    void ToggleStatisticDialog();
    void TogglePlot();
    void Detailed();

signals:
    void Interrupt();
    void Finished();
    void IncrementProgress(int progess);
    void MaximumSteps(int steps);
};
