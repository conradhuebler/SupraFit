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
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
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

    m_results = new ResultsDialog(m_model, new ChartWrapper(this), this);
    connect(m_results, &ResultsDialog::LoadModel, this, &MetaModelWidget::LoadJson);

    m_statistic_widget = new StatisticWidget(m_model, this);

    QGridLayout* layout = new QGridLayout;

    m_project_name = new QLineEdit(Model()->ProjectTitle());
    layout->addWidget(new QLabel(tr("<h4>Project Title</h4>")), 0, 0, 1, 1);
    layout->addWidget(m_project_name, 0, 1, 1, 3);
    connect(m_project_name, &QLineEdit::textChanged, this, [this](const QString& str) {
        if (Model())
            Model()->setProjectTitle(str);
    });

    m_calculate = new QPushButton(tr("Calculate"));
    layout->addWidget(m_calculate, 1, 0);
    m_calculate->setStyleSheet("background-color: #77d740;");

    m_minimize = new QPushButton(tr("Minimize"));
    layout->addWidget(m_minimize, 1, 1);
    m_minimize->setStyleSheet("background-color: #77d740;");
    layout->addWidget(new QLabel(tr("Connection Strategy for Parameters:")), 1, 2);

    m_type = new QComboBox;
    m_type->setStyleSheet("background-color: #77d740;");
    m_type->addItems(QStringList() << "None"
                                   << "All"
                                   << "Custom");
    m_type->setCurrentIndex(Model()->ConnectionType());

    connect(m_type, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        this->Model()->setConnectType(static_cast<MetaModel::ConnectType>(index));
    });

    //Model()->setConnectType(static_cast<MetaModel::ConnectType>(m_type->currentIndex()));

    layout->addWidget(m_type, 1, 3);

    m_actions = new ModelActions;
    m_metamodelparameter = new MetaModelParameter(m_model);
    layout->addWidget(m_metamodelparameter, 2, 0, 1, 4);

    m_jobmanager = new JobManager(this);
    m_jobmanager->setModel(m_model);

    connect(m_jobmanager, &JobManager::ShowResult, this, [this](SupraFit::Method type, int index) {
        if (type != SupraFit::Method::FastConfidence) {
            this->m_results->Attention();
            this->m_results->ShowResult(type, index);
        }

        QApplication::restoreOverrideCursor();
    });

    connect(m_actions, SIGNAL(NewGuess()), this, SLOT(NewGuess()));
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

    layout->addWidget(m_actions, 3, 0, 1, 4);
    layout->addWidget(m_statistic_widget, 4, 0, 1, 4);
    connect(m_minimize, &QPushButton::clicked, this, &MetaModelWidget::Minimize);
    connect(m_calculate, &QPushButton::clicked, this, &MetaModelWidget::Calculate);

    connect(Model(), &MetaModel::ParameterMoved, this, [this]() { m_type->setCurrentIndex(this->Model()->ConnectionType()); });
    connect(Model(), &DataClass::Message, this, &MetaModelWidget::Message);
    connect(Model(), &DataClass::Warning, this, &MetaModelWidget::Warning);
    connect(Model(), &AbstractModel::Recalculated, m_statistic_widget, &StatisticWidget::Update);

    setLayout(layout);
}

void MetaModelWidget::Calculate()
{
    m_model->CalculateStatistics(true);
    m_model->setFast(false);
    m_model->Calculate();
}

void MetaModelWidget::Minimize()
{
    Waiter wait;
    qint64 t0 = QDateTime::currentMSecsSinceEpoch();

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

    m_model->CalculateStatistics(true);
    m_model->setFast(false);
    m_model->Calculate();

    delete thread;

    if (qApp->instance()->property("auto_confidence").toBool())
        FastConfidence();

    qint64 t1 = QDateTime::currentMSecsSinceEpoch();

    Model()->Message(QString("Optimisation took %2 msecs for %3 in %4").arg(t1 - t0).arg(m_model->Name()).arg(m_model->ProjectTitle()), 5);
}

void MetaModelWidget::ToggleStatisticDialog()
{
    StatisticDialog* statistic_dialog = new StatisticDialog(m_model, this);

    connect(m_jobmanager, &JobManager::prepare, statistic_dialog, &StatisticDialog::MaximumSteps);
    connect(m_jobmanager, &JobManager::incremented, statistic_dialog, &StatisticDialog::IncrementProgress);

    connect(m_jobmanager, &JobManager::started, statistic_dialog, &StatisticDialog::ShowWidget);
    connect(m_jobmanager, &JobManager::finished, statistic_dialog, &StatisticDialog::HideWidget);
    connect(m_jobmanager, &JobManager::Message, statistic_dialog, &StatisticDialog::Message, Qt::DirectConnection);

    connect(statistic_dialog, &StatisticDialog::Interrupt, m_jobmanager, &JobManager::Interrupt);
    connect(statistic_dialog, &StatisticDialog::RunCalculation, m_jobmanager, [this](const QJsonObject& job) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        this->m_jobmanager->AddJob(job);
        this->m_jobmanager->RunJobs();
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
    job["MaxError"] = error * (f_value * m_model.data()->Parameter() / (m_model.data()->Points() - m_model.data()->Parameter()) + 1);
    job["confidence"] = qApp->instance()->property("p_value").toDouble();
    job["f_value"] = f_value;
    job["IncludeSeries"] = qApp->instance()->property("series_confidence").toBool();
    job["method"] = SupraFit::Method::FastConfidence;

    m_jobmanager->AddJob(job);
    m_jobmanager->RunJobs();
}

void MetaModelWidget::OpenAdvancedSearch()
{
    AdvancedSearch* advancedsearch = new AdvancedSearch(this);
    advancedsearch->setModel(m_model);

    connect(advancedsearch, &AdvancedSearch::RunCalculation, m_jobmanager, [this](const QJsonObject& job) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        this->m_jobmanager->AddJob(job);
        this->m_jobmanager->RunJobs();
    });

    advancedsearch->exec();
    delete advancedsearch;
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
    SupraFit::Method type = SupraFit::Method(data["controller"].toObject()["method"].toInt());
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
