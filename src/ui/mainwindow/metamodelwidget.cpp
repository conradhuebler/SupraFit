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
    connect(m_minimize, &QPushButton::clicked, this, &MetaModelWidget::Minimize);

    setLayout(layout);
}

void MetaModelWidget::Minimize()
{
    NonLinearFitThread* thread = new NonLinearFitThread(false);

    thread->setModel(m_model, false);
    thread->run();
    bool converged = thread->Converged();
    QJsonObject model;
    if (converged)
        model = thread->ConvergedParameter();
    else
        model = thread->BestIntermediateParameter();
    qreal new_error = thread->SumOfError();
    qDebug() << model;
    m_model->ImportModel(model);
    delete thread;
}
