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

#include <QtCore/QPointer>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>

#include "comparedialog.h"

CompareDialog::CompareDialog(QWidget* parent)
    : QDialog(parent)
    , m_cutoff(1)
{
    setUi();
}

void CompareDialog::setUi()
{

    m_overview = new QTextEdit;
    m_overview->setReadOnly(true);

    QGroupBox* reduction = new QGroupBox(tr("Reduction Analysis"));
    QGridLayout* layout = new QGridLayout;
    m_reduction = new QPushButton(tr("Compare Reduction Analysis"));
    connect(m_reduction, &QPushButton::clicked, this, &CompareDialog::CompareReduction);

    m_cutoff_box = new QDoubleSpinBox;
    m_cutoff_box->setMinimum(0);
    m_cutoff_box->setMaximum(1e9);
    m_cutoff_box->setDecimals(2);
    m_cutoff_box->setValue(m_cutoff);

    m_local = new QCheckBox(tr("Include local Parameter"));

    reduction->setLayout(layout);
    layout->addWidget(m_cutoff_box, 0, 0);
    layout->addWidget(m_local, 0, 1);
    layout->addWidget(m_reduction, 1, 0, 1, 2);

    layout = new QGridLayout;
    QGroupBox* aic = new QGroupBox(tr("Akaike’s Information Criterion"));
    m_aic = new QPushButton(tr("Compare AIC"));
    connect(m_aic, &QPushButton::clicked, this, &CompareDialog::CompareAIC);

    layout->addWidget(m_aic, 0, 0);
    aic->setLayout(layout);

    layout = new QGridLayout;
    //layout->addWidget(reduction, 0, 0, 1, 2);
    //layout->addWidget(aic, 1, 0, 1, 2);

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(reduction);
    hlayout->addWidget(aic);

    m_hide = new QPushButton(tr("Hide Dialog"));
    connect(m_hide, &QPushButton::clicked, this, &QDialog::hide);
    layout->addWidget(m_overview, 0, 0);
    layout->addLayout(hlayout, 1, 0);
    layout->addWidget(m_hide, 2, 0);

    setLayout(layout);
}

void CompareDialog::setCutoff(qreal cutoff)
{
    m_cutoff = cutoff;
    m_cutoff_box->setValue(m_cutoff);
}
