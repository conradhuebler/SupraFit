/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/AbstractModel.h"

#include <QtCore/QPointer>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>

#include "optionswidget.h"

OptionsWidget::OptionsWidget(QSharedPointer<AbstractModel > model) : m_model(model)
{
    QVBoxLayout *layout = new QVBoxLayout;
    for(const QString &str : m_model->getAllOptions())
    {
        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->addWidget(new QLabel(str));
        QPointer<QComboBox > box = new QComboBox;
        box->addItems(m_model->getSingleOptionValues(str));
        box->setCurrentText(m_model->getOption(str));
        hlayout->addWidget(box);
        m_options[str] = box;
        connect(box, SIGNAL(currentIndexChanged(QString)), this, SLOT(setOption()));
        layout->addLayout(hlayout);
    }
    setLayout(layout);
    setTitle("Model Option");
}

OptionsWidget::~OptionsWidget()
{
}


void OptionsWidget::setOption()
{
    for(const QString &str : m_model->getAllOptions())
    {
            QString value = m_options[str]->currentText();
            m_model->setOption(str, value);
    }
}