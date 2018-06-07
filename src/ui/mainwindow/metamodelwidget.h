/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/capabilities/reductionanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"

#include <QtCore/QSharedPointer>

#include <QtWidgets/QWidget>

#include "src/core/models.h"

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
    }
    inline QPointer<MetaModel> Model() { return qobject_cast<MetaModel*>(m_model.data()); }

public slots:


private:
    void setUi();

    QSharedPointer<AbstractModel> m_model;
    QPushButton* m_minimize;
    QComboBox* m_type;
    ModelActions* m_actions;
    ModalDialog *m_dialogs, *m_table_result;
    StatisticWidget* m_statistic_widget;
    ResultsDialog* m_results;
    QList<QJsonObject> m_fast_confidence;

    void LoadStatistic(const QJsonObject& data, const QList<QJsonObject>& models);
    void FastConfidence();

private slots:
    void Minimize();

    void WGStatistic(WGSConfig config);
    void MoCoStatistic(MoCoConfig config);
    void MCStatistic(MCConfig config);
    void CVAnalyse(ReductionAnalyse::CVType type);
    void Reduction();

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
