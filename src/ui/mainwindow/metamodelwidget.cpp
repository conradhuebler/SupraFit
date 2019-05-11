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

#include "src/capabilities/jobmanager.h"
#include "src/capabilities/modelcomparison.h"

#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/models.h"

#include "src/ui/dialogs/advancedsearch.h"
#include "src/ui/dialogs/resultsdialog.h"
#include "src/ui/dialogs/statisticdialog.h"

#include "src/ui/dialogs/modaldialog.h"
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
    m_type->setCurrentIndex(Model()->ConnectionType());

    connect(m_type, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        this->Model()->setConnectType(static_cast<MetaModel::ConnectType>(index));
    });

    Model()->setConnectType(static_cast<MetaModel::ConnectType>(m_type->currentIndex()));

    layout->addWidget(m_type, 0, 1);

    m_actions = new ModelActions;
    m_metamodelparameter = new MetaModelParameter(m_model);
    layout->addWidget(m_metamodelparameter, 1, 0, 1, 2);

    m_jobmanager = new JobManager(this);
    m_jobmanager->setModel(m_model);

    connect(m_actions, SIGNAL(NewGuess()), this, SLOT(NewGuess()));
    //connect(m_actions, SIGNAL(LocalMinimize()), this, SLOT(LocalMinimize()));
    //connect(m_actions, SIGNAL(OptimizerSettings()), this, SLOT(OptimizerSettings()));
    connect(m_actions, SIGNAL(ImportConstants()), this, SLOT(ImportConstants()));
    connect(m_actions, SIGNAL(ExportConstants()), this, SLOT(ExportConstants()));
    connect(m_actions, SIGNAL(OpenAdvancedSearch()), this, SLOT(OpenAdvancedSearch()));
    connect(m_actions, SIGNAL(TogglePlot()), this, SLOT(TogglePlot()));
    connect(m_actions, SIGNAL(ToggleStatisticDialog()), this, SLOT(ToggleStatisticDialog()));
    //  connect(m_actions, SIGNAL(Save2File()), this, SLOT(Save2File()));
    //connect(m_actions, SIGNAL(ExportSimModel()), this, SLOT(ExportSimModel()));
    //connect(m_actions, &ModelActions::Restore, this, &ModelWidget::Restore);
    connect(m_actions, &ModelActions::Detailed, this, &MetaModelWidget::Detailed);

    layout->addWidget(m_actions, 2, 0, 1, 2);
    layout->addWidget(m_statistic_widget, 3, 0, 1, 2);
    connect(m_minimize, &QPushButton::clicked, this, &MetaModelWidget::Minimize);
    connect(Model(), &MetaModel::ParameterMoved, this, [this]() { m_type->setCurrentIndex(this->Model()->ConnectionType()); });
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

    //    if (qApp->instance()->property("auto_confidence").toBool())
    //        FastConfidence();
    //    else
    m_statistic_widget->Update();
}

void MetaModelWidget::ToggleStatisticDialog()
{
    StatisticDialog* statistic_dialog = new StatisticDialog(m_model, this);

    connect(m_jobmanager, &JobManager::prepare, statistic_dialog, &StatisticDialog::MaximumSteps);
    connect(m_jobmanager, &JobManager::incremented, statistic_dialog, &StatisticDialog::IncrementProgress);

    connect(m_jobmanager, &JobManager::started, statistic_dialog, &StatisticDialog::ShowWidget);
    connect(m_jobmanager, &JobManager::finished, statistic_dialog, &StatisticDialog::HideWidget);

    connect(statistic_dialog, &StatisticDialog::Interrupt, m_jobmanager, &JobManager::Interrupt); //, Qt::DirectConnection);
    connect(statistic_dialog, &StatisticDialog::RunCalculation, m_jobmanager, [this](const QJsonObject& job) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        this->m_jobmanager->AddJob(job);
        this->m_jobmanager->RunJobs();
    });
    connect(m_jobmanager, &JobManager::ShowResult, this, [this, statistic_dialog](SupraFit::Statistic type, int index) {
        if (type != SupraFit::Statistic::FastConfidence) {
            if (type != SupraFit::Statistic::GlobalSearch)
                statistic_dialog->hide();

            this->m_results->Attention();
            this->m_results->ShowResult(type, index);
        }

        QApplication::restoreOverrideCursor();
    });
    statistic_dialog->exec();

    delete statistic_dialog;
}

void MetaModelWidget::FastConfidence()
{
    Waiter wait;

    QJsonObject job(ModelComparisonConfigBlock);

    job["FastConfidenceSteps"] = qApp->instance()->property("FastConfidenceSteps").toInt();
    job["FastConfidenceScaling"] = qApp->instance()->property("FastConfidenceScaling").toInt();
    qreal f_value = m_model.data()->finv(qApp->instance()->property("p_value").toDouble());
    qreal error = m_model.data()->SumofSquares();
    qDebug() << qApp->instance()->property("p_value").toDouble() << f_value << error << error * (f_value * m_model.data()->Parameter() / (m_model.data()->Points() - m_model.data()->Parameter()) + 1);

    job["MaxError"] = error * (f_value * m_model.data()->Parameter() / (m_model.data()->Points() - m_model.data()->Parameter()) + 1);
    job["confidence"] = qApp->instance()->property("p_value").toDouble();
    job["f_value"] = f_value;
    job["IncludeSeries"] = qApp->instance()->property("series_confidence").toBool();
    job["method"] = SupraFit::Statistic::FastConfidence;

    m_jobmanager->AddJob(job);
    m_jobmanager->RunJobs();
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

void MetaModelWidget::LoadStatistic(const QJsonObject& data)
{
    int index = m_model->UpdateStatistic(data);
    m_results->Attention();
    SupraFit::Statistic type = SupraFit::Statistic(data["controller"].toObject()["method"].toInt());
    m_results->ShowResult(type, index);
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
