/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/AbstractModel.h"

#include "src/core/toolset.h"

#include <QtCore/QPointer>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>

#include "optionswidget.h"

OptionsWidget::OptionsWidget(QSharedPointer<AbstractModel> model)
    : m_model(model)
{
    QVBoxLayout* layout = new QVBoxLayout;

    QHBoxLayout* hlayout = new QHBoxLayout;
    layout->setAlignment(Qt::AlignTop);
    QHBoxLayout* smalllayout;
    int counter = 1;

    int rows = ToolSet::NiceRows(m_model.toStrongRef().data()->getAllOptions().size(), qApp->instance()->property("ModelParameterColums").toInt());

    for (int index : m_model.toStrongRef().data()->getAllOptions()) {
        QString str = m_model.toStrongRef().data()->getOptionName(index);

        if (m_model.toStrongRef().data()->getSingleOptionValues(index).size() == 0)
            continue;

        smalllayout = new QHBoxLayout;

        smalllayout->addWidget(new QLabel(str));
        QPointer<QComboBox> box = new QComboBox;
        box->setMaximumWidth(200);
        box->setMinimumWidth(200);
        box->addItems(m_model.toStrongRef().data()->getSingleOptionValues(index));
        box->setCurrentText(m_model.toStrongRef().data()->getOption(index));
        smalllayout->addWidget(box);
        m_options[index] = box;
        connect(box, SIGNAL(currentIndexChanged(QString)), this, SLOT(setOption()));

        hlayout->addLayout(smalllayout);
        if (counter % rows == 0) {
            layout->addLayout(hlayout);
            hlayout = new QHBoxLayout;
        }
        counter++;
    }
    layout->addLayout(hlayout);
    setLayout(layout);
    setTitle("Model Option");
}

OptionsWidget::~OptionsWidget()
{
    m_model.clear();
}

void OptionsWidget::setOption()
{
    for (int index : m_model.toStrongRef().data()->getAllOptions()) {
        QString value = m_options[index]->currentText();
        m_model.toStrongRef().data()->setOption(index, value);
    }
    m_model.toStrongRef().data()->Calculate();
}
