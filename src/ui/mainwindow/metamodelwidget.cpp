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

#include "src/core/minimizer.h"
#include "src/core/models.h"

#include "src/ui/widgets/modelactions.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>

#include "metamodelwidget.h"

MetaModelWidget::MetaModelWidget(QWidget* parent)
    : QWidget(parent)
{
    setUi();
}

void MetaModelWidget::setUi()
{
    QGridLayout* layout = new QGridLayout;

    m_minimize = new QPushButton(tr("Minimize"));
    layout->addWidget(m_minimize, 0, 0);

    m_type = new QComboBox;
    m_type->addItems(QStringList() << "None"
                                   << "All");
    layout->addWidget(m_type, 0, 1);

    m_actions = new ModelActions;
    /*
    connect(m_actions, SIGNAL(NewGuess()), this, SLOT(NewGuess()));
    connect(m_actions, SIGNAL(LocalMinimize()), this, SLOT(LocalMinimize()));
    connect(m_actions, SIGNAL(OptimizerSettings()), this, SLOT(OptimizerSettings()));
    connect(m_actions, SIGNAL(ImportConstants()), this, SLOT(ImportConstants()));
    connect(m_actions, SIGNAL(ExportConstants()), this, SLOT(ExportConstants()));
    connect(m_actions, SIGNAL(OpenAdvancedSearch()), this, SLOT(OpenAdvancedSearch()));
    connect(m_actions, SIGNAL(TogglePlot()), this, SLOT(TogglePlot()));
    connect(m_actions, SIGNAL(ToggleStatisticDialog()), this, SLOT(ToggleStatisticDialog()));
    connect(m_actions, SIGNAL(Save2File()), this, SLOT(Save2File()));
    connect(m_actions, SIGNAL(ToggleSearch()), this, SLOT(ToggleSearchTable()));
    connect(m_actions, SIGNAL(ExportSimModel()), this, SLOT(ExportSimModel()));
    connect(m_actions, &ModelActions::Restore, this, &ModelWidget::Restore);
    connect(m_actions, &ModelActions::Detailed, this, &ModelWidget::Detailed);
*/
    layout->addWidget(m_actions, 1, 0, 1, 2);

    connect(m_minimize, &QPushButton::clicked, this, &MetaModelWidget::Minimize);

    setLayout(layout);
}

void MetaModelWidget::Minimize()
{
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
