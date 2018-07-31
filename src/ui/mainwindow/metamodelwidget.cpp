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

#include "src/ui/dialogs/advancedsearch.h"
#include "src/ui/dialogs/resultsdialog.h"
#include "src/ui/dialogs/statisticdialog.h"

#include "src/ui/dialogs/modeldialog.h"
#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/waiter.h"
#include "src/ui/widgets/metamodelparameter.h"
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
    connect(m_results, &ResultsDialog::LoadModel, this, &MetaModelWidget::LoadJson);

    m_statistic_widget = new StatisticWidget(m_model, this);

    QGridLayout* layout = new QGridLayout;

    m_minimize = new QPushButton(tr("Minimize"));
    layout->addWidget(m_minimize, 0, 0);

    m_type = new QComboBox;
    m_type->addItems(QStringList() << "None"
                                   << "All"
                                   << "Custom");
    connect(m_type, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        this->Model()->setConnectType(static_cast<MetaModel::ConnectType>(index));
    });

    Model()->setConnectType(static_cast<MetaModel::ConnectType>(m_type->currentIndex()));

    layout->addWidget(m_type, 0, 1);

    m_actions = new ModelActions;
    m_metamodelparameter = new MetaModelParameter(m_model);
    layout->addWidget(m_metamodelparameter, 1, 0, 1, 2);

    connect(m_actions, SIGNAL(NewGuess()), this, SLOT(NewGuess()));
    //connect(m_actions, SIGNAL(LocalMinimize()), this, SLOT(LocalMinimize()));
    connect(m_actions, SIGNAL(OptimizerSettings()), this, SLOT(OptimizerSettings()));
    connect(m_actions, SIGNAL(ImportConstants()), this, SLOT(ImportConstants()));
    connect(m_actions, SIGNAL(ExportConstants()), this, SLOT(ExportConstants()));
    connect(m_actions, SIGNAL(OpenAdvancedSearch()), this, SLOT(OpenAdvancedSearch()));
    connect(m_actions, SIGNAL(TogglePlot()), this, SLOT(TogglePlot()));
    connect(m_actions, SIGNAL(ToggleStatisticDialog()), this, SLOT(ToggleStatisticDialog()));
    //connect(m_actions, SIGNAL(ToggleStatisticDialog()), this, SLOT(MonteCarlo()));
    connect(m_actions, SIGNAL(Save2File()), this, SLOT(Save2File()));
    //connect(m_actions, SIGNAL(ExportSimModel()), this, SLOT(ExportSimModel()));
    //connect(m_actions, &ModelActions::Restore, this, &ModelWidget::Restore);
    connect(m_actions, &ModelActions::Detailed, this, &MetaModelWidget::Detailed);

    layout->addWidget(m_actions, 2, 0, 1, 2);
    layout->addWidget(m_statistic_widget, 3, 0, 1, 2);
    connect(m_minimize, &QPushButton::clicked, this, &MetaModelWidget::Minimize);
    connect(Model(), &MetaModel::ParameterMoved, this, [this]() { m_type->setCurrentIndex(2); });
    setLayout(layout);
}

void MetaModelWidget::Minimize()
{
    Waiter wait;

    NonLinearFitThread* thread = new NonLinearFitThread(false);
    thread->setModel(m_model, false);

    thread->run();
    bool converged = thread->Converged();
    QJsonObject model;
    if (converged)
        model = thread->ConvergedParameter();
    else
        model = thread->BestIntermediateParameter();

    if (!m_model->ImportModel(model))
        qDebug() << "something went wrong";

    m_model->setFast(false);
    m_model->Calculate();

    delete thread;

    if (qApp->instance()->property("auto_confidence").toBool())
        FastConfidence();
    else
        m_statistic_widget->Update();
}

void MetaModelWidget::ToggleStatisticDialog()
{
    StatisticDialog* statistic_dialog = new StatisticDialog(m_model, this);
    connect(statistic_dialog, &StatisticDialog::MCStatistic, this, &MetaModelWidget::MCStatistic);
    connect(statistic_dialog, &StatisticDialog::WGStatistic, this, &MetaModelWidget::WGStatistic);
    connect(statistic_dialog, &StatisticDialog::MoCoStatistic, this, &MetaModelWidget::MoCoStatistic);
    connect(statistic_dialog, &StatisticDialog::CrossValidation, this, &MetaModelWidget::CVAnalyse);
    connect(statistic_dialog, &StatisticDialog::Reduction, this, &MetaModelWidget::Reduction);
    connect(statistic_dialog, &StatisticDialog::Reduction, this, &MetaModelWidget::Interrupt);
    connect(this, &MetaModelWidget::IncrementProgress, statistic_dialog, &StatisticDialog::IncrementProgress);
    connect(this, &MetaModelWidget::MaximumSteps, statistic_dialog, &StatisticDialog::MaximumSteps);
    connect(this, &MetaModelWidget::Finished, statistic_dialog, &StatisticDialog::accept);

    statistic_dialog->exec();

    delete statistic_dialog;
}

void MetaModelWidget::OpenAdvancedSearch()
{
    AdvancedSearch* advancedsearch = new AdvancedSearch(this);
    advancedsearch->setModel(m_model);
    advancedsearch->exec();
    LoadStatistic(advancedsearch->globalSearch()->Result());
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
    SupraFit::Statistic type = SupraFit::Statistic(data["controller"].toObject()["method"].toInt());
    m_results->ShowResult(type, index);
}

void MetaModelWidget::FastConfidence()
{
    MoCoConfig config;

    qreal f_value = m_model.data()->finv(qApp->instance()->property("p_value").toDouble() / 100);
    qreal error = m_model.data()->SumofSquares();
    config.maxerror = error * (f_value * m_model.data()->Parameter() / (m_model.data()->Points() - m_model.data()->Parameter()) + 1);
    config.optimizer_config = m_model->getOptimizerConfig();
    config.fisher_statistic = true;
    config.confidence = qApp->instance()->property("p_value").toDouble();
    ModelComparison* statistic = new ModelComparison(config, this);
    statistic->setModel(m_model);
    bool series = qApp->instance()->property("series_confidence").toBool();
    statistic->FastConfidence(series);
    m_model->UpdateStatistic(statistic->Result());
    m_fast_confidence = statistic->Results();
    delete statistic;
}

void MetaModelWidget::MCStatistic(MCConfig config)
{
    Waiter wait;

    QPointer<MonteCarloStatistics> statistic = new MonteCarloStatistics(config, this);
    connect(this, &MetaModelWidget::Interrupt, statistic, &AbstractSearchClass::Interrupt);
    connect(statistic, &AbstractSearchClass::IncrementProgress, this, &MetaModelWidget::IncrementProgress);

    statistic->setModel(m_model);
    statistic->Evaluate();

    LoadStatistic(statistic->Result(), statistic->Models());
    emit Finished();
    delete statistic;
}

void MetaModelWidget::Reduction()
{
    Waiter wait;

    ReductionAnalyse* statistic = new ReductionAnalyse(m_model->getOptimizerConfig());

    connect(this, &MetaModelWidget::Interrupt, statistic, &AbstractSearchClass::Interrupt);
    connect(statistic, &AbstractSearchClass::IncrementProgress, this, &MetaModelWidget::IncrementProgress);
    connect(statistic, &AbstractSearchClass::setMaximumSteps, this, &MetaModelWidget::MaximumSteps);

    statistic->setModel(m_model);
    statistic->PlainReduction();
    LoadStatistic(statistic->Result(), statistic->Models());

    emit Finished();
    delete statistic;
}

void MetaModelWidget::CVAnalyse(ReductionAnalyse::CVType type)
{
    Waiter wait;

    ReductionAnalyse* statistic = new ReductionAnalyse(m_model->getOptimizerConfig());
    connect(this, &MetaModelWidget::Interrupt, statistic, &AbstractSearchClass::Interrupt);
    connect(statistic, &AbstractSearchClass::IncrementProgress, this, &MetaModelWidget::IncrementProgress);
    connect(statistic, &AbstractSearchClass::setMaximumSteps, this, &MetaModelWidget::MaximumSteps, Qt::DirectConnection);

    statistic->setModel(m_model);
    statistic->CrossValidation(type);
    LoadStatistic(statistic->Result(), statistic->Models());
    emit Finished();

    delete statistic;
}

void MetaModelWidget::MoCoStatistic(MoCoConfig config)
{
    Waiter wait;

    config.optimizer_config = m_model->getOptimizerConfig();

    if (config.maxerror < 1E-8)
        config.maxerror = m_model->Error(config.confidence, config.fisher_statistic);

    ModelComparison* statistic = new ModelComparison(config, this);
    if (m_fast_confidence.size())
        statistic->setResults(m_fast_confidence);
    connect(this, &MetaModelWidget::Interrupt, statistic, &AbstractSearchClass::Interrupt);
    connect(statistic, &AbstractSearchClass::IncrementProgress, this, &MetaModelWidget::IncrementProgress);

    QJsonObject json = m_model->ExportModel(false);
    statistic->setModel(m_model);
    bool result = statistic->Confidence();
    if (result)
        LoadStatistic(statistic->Result(), statistic->Models());
    // else
    //     QMessageBox::information(this, tr("Not done"), tr("No calculation where done, because there is only one parameter of interest."));
    //m_statistic_dialog->HideWidget();
    emit Finished();
    delete statistic;
}

void MetaModelWidget::WGStatistic(WGSConfig config)
{
    Waiter wait;

    config.optimizer_config = m_model->getOptimizerConfig();

    if (config.maxerror < 1E-8)
        config.maxerror = m_model->Error(config.confidence, config.fisher_statistic);

    WeakenedGridSearch* statistic = new WeakenedGridSearch(config, this);

    connect(this, &MetaModelWidget::Interrupt, statistic, &AbstractSearchClass::Interrupt);
    connect(statistic, &AbstractSearchClass::IncrementProgress, this, &MetaModelWidget::IncrementProgress);

    QJsonObject json = m_model->ExportModel(false);
    statistic->setModel(m_model);
    statistic->setParameter(json);

    if (!statistic->ConfidenceAssesment()) {
        // emit Warning("The optimization seems not to be converged with respect to at least one constants!\nShowing the results anyway.", 1);
    }

    LoadStatistic(statistic->Result(), statistic->Models());

    emit Finished();
    delete statistic;
}

void MetaModelWidget::ImportConstants()
{
    QString str = QFileDialog::getOpenFileName(this, tr("Open File"), getDir(), tr("Json File (*.json);;Binary (*.suprafit);;All files (*.*)"));
    if (!str.isEmpty()) {
        setLastDir(str);
        QJsonObject object;

        if (JsonHandler::ReadJsonFile(object, str))
            LoadJson(object);
        else
            qDebug() << "loading failed";
    }
}

void MetaModelWidget::LoadJson(const QJsonObject& object)
{
    m_model->ImportModel(object);
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

void MetaModelWidget::TogglePlot()
{
    m_results->Attention();
}
