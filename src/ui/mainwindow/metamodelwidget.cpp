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

#include "src/capabilities/abstractsearchclass.h"
#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/reductionanalyse.h"
#include "src/capabilities/weakenedgridsearch.h"

#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/models.h"

#include "src/ui/dialogs/resultsdialog.h"

#include "src/ui/dialogs/modeldialog.h"
#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/waiter.h"
#include "src/ui/widgets/modelactions.h"
#include "src/ui/widgets/results/resultswidget.h"
#include "src/ui/widgets/statisticwidget.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>

#include "metamodelwidget.h"

MetaModelWidget::MetaModelWidget(QWidget* parent)
    : QWidget(parent)
{
}

void MetaModelWidget::setUi()
{
    m_dialogs = new ModalDialog;
    m_dialogs->setWindowTitle("Information " + m_model->Name() + " | " + qApp->instance()->property("projectname").toString());

    m_results = new ResultsDialog(m_model, new ChartWrapper(false), this);

    QGridLayout* layout = new QGridLayout;

    m_minimize = new QPushButton(tr("Minimize"));
    layout->addWidget(m_minimize, 0, 0);

    m_type = new QComboBox;
    m_type->addItems(QStringList() << "None"
                                   << "All");
    layout->addWidget(m_type, 0, 1);

    m_actions = new ModelActions;

    connect(m_actions, SIGNAL(NewGuess()), this, SLOT(NewGuess()));
    //connect(m_actions, SIGNAL(LocalMinimize()), this, SLOT(LocalMinimize()));
    connect(m_actions, SIGNAL(OptimizerSettings()), this, SLOT(OptimizerSettings()));
    connect(m_actions, SIGNAL(ImportConstants()), this, SLOT(ImportConstants()));
    connect(m_actions, SIGNAL(ExportConstants()), this, SLOT(ExportConstants()));
    connect(m_actions, SIGNAL(OpenAdvancedSearch()), this, SLOT(OpenAdvancedSearch()));
    connect(m_actions, SIGNAL(TogglePlot()), this, SLOT(TogglePlot()));
    //connect(m_actions, SIGNAL(ToggleStatisticDialog()), this, SLOT(ToggleStatisticDialog()));
    connect(m_actions, SIGNAL(ToggleStatisticDialog()), this, SLOT(MonteCarlo()));
    connect(m_actions, SIGNAL(Save2File()), this, SLOT(Save2File()));
    connect(m_actions, SIGNAL(ToggleSearch()), this, SLOT(ToggleSearchTable()));
    //connect(m_actions, SIGNAL(ExportSimModel()), this, SLOT(ExportSimModel()));
    //connect(m_actions, &ModelActions::Restore, this, &ModelWidget::Restore);
    connect(m_actions, &ModelActions::Detailed, this, &MetaModelWidget::Detailed);

    layout->addWidget(m_actions, 1, 0, 1, 2);

    connect(m_minimize, &QPushButton::clicked, this, &MetaModelWidget::Minimize);

    setLayout(layout);
}

void MetaModelWidget::Minimize()
{
    Waiter wait;

    NonLinearFitThread* thread = new NonLinearFitThread(false);

    Model()->setConnectType((MetaModel::ConnectType)m_type->currentIndex());

    thread->setModel(m_model, false);
    thread->run();
    bool converged = thread->Converged();
    QJsonObject model;
    if (converged)
        model = thread->ConvergedParameter();
    else
        model = thread->BestIntermediateParameter();
    qreal new_error = thread->SumOfError();
    if (!m_model->ImportModel(model))
        qDebug() << "something went wrong";
    delete thread;
}

void MetaModelWidget::ToggleStatisticDialog()
{
}

void MetaModelWidget::OpenAdvancedSearch()
{
}

void MetaModelWidget::Detailed()
{
    QTextEdit* text = new QTextEdit;
    text->setText("<html><pre>" + m_model->Data2Text() + "\n" + m_model->Model2Text() + "</ br>" + m_statistic_widget->Statistic() + "</pre></html>");
    m_dialogs->setWidget(text, tr("Detailed Information on Calculation Results"));
    m_dialogs->Attention();
}

void MetaModelWidget::NewGuess()
{
    m_model->InitialGuess();
}

void MetaModelWidget::LoadStatistic(const QJsonObject& data, const QList<QJsonObject>& models)
{
    int index = m_model->UpdateStatistic(data);
    m_results->Attention();
    //m_statistic_dialog->HideWidget();
    SupraFit::Statistic type = SupraFit::Statistic(data["controller"].toObject()["method"].toInt());
    m_results->ShowResult(type, index);
}

void MetaModelWidget::MonteCarlo()
{
    Waiter wait;

    MCConfig config;
    config.maxsteps = 1000;
    config.optimizer_config = m_model->getOptimizerConfig();
    config.optimizer_config.MaxIter = 75;
    QPointer<MonteCarloStatistics> statistic = new MonteCarloStatistics(config, this);
    //connect(m_statistic_dialog, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    connect(this, SIGNAL(Interrupt()), statistic, SLOT(Interrupt()), Qt::DirectConnection);
    //connect(statistic, SIGNAL(IncrementProgress(int)), m_statistic_dialog, SLOT(IncrementProgress(int)), Qt::DirectConnection);
    connect(statistic, SIGNAL(IncrementProgress(int)), this, SIGNAL(IncrementProgress(int)), Qt::DirectConnection);
    statistic->setModel(m_model);
    statistic->Evaluate();

    LoadStatistic(statistic->Result(), statistic->Models());
    delete statistic;
}

void MetaModelWidget::Reduction()
{
}

void MetaModelWidget::CrossValidation()
{
}

void MetaModelWidget::ModelComparison()
{
}

void MetaModelWidget::WeakendGridSearch()
{
}

void MetaModelWidget::ImportConstants()
{
    QString str = QFileDialog::getOpenFileName(this, tr("Open File"), getDir(), tr("Json File (*.json);;Binary (*.suprafit);;All files (*.*)"));
    if (!str.isEmpty()) {
        setLastDir(str);
        QJsonObject object;
        /*
        if (JsonHandler::ReadJsonFile(object, str))
            LoadJson(object);
        else
            qDebug() << "loading failed";*/
    }
}

void MetaModelWidget::ExportConstants()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("Json File (*.json);;Binary (*.suprafit);;All files (*.*)"));
    if (!str.isEmpty()) {
        setLastDir(str);
        qobject_cast<MetaModel*>(m_model)->PrepareTables();

        QJsonObject gameObject = m_model->ExportModel();
        JsonHandler::WriteJsonFile(gameObject, str);
    }
}
