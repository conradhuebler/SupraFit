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

#include <QtCore/QPointer>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
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
    QGridLayout* layout = new QGridLayout;

    QGroupBox* reduction = new QGroupBox(tr("Reduction Analysis"));
    m_reduction = new QPushButton(tr("Compare Reduction Analysis"));
    connect(m_reduction, &QPushButton::clicked, this, &CompareDialog::CompareReduction);

    m_cutoff_box = new QDoubleSpinBox;
    m_cutoff_box->setMinimum(0);
    m_cutoff_box->setMaximum(1e9);
    m_cutoff_box->setDecimals(2);
    m_cutoff_box->setValue(m_cutoff);

    m_red_local = new QCheckBox(tr("Include local Parameter"));
    reduction->setLayout(layout);
    layout->addWidget(m_cutoff_box, 0, 0);
    layout->addWidget(m_red_local, 0, 1);
    layout->addWidget(m_reduction, 1, 0, 1, 2);

    layout = new QGridLayout;
    QGroupBox* aic = new QGroupBox(tr("Akaike’s Information Criterion"));
    m_aic = new QPushButton(tr("Compare AIC"));
    connect(m_aic, &QPushButton::clicked, this, &CompareDialog::CompareAIC);
    layout->addWidget(m_aic, 0, 0);
    aic->setLayout(layout);

    layout = new QGridLayout;
    QGroupBox* crossvalidation = new QGroupBox(tr("Cross Validation & Monte Carlo"));
    m_crossvalidation = new QPushButton(tr("Compare Cross Validation"));
    m_montecarlo = new QPushButton(tr("Monte Carlo"));

    m_cv_loo = new QRadioButton(tr("LOO CV"));
    connect(m_cv_loo, &QPushButton::clicked, this, [this]() {
        this->m_cvtype = 1;
    });
    m_cv_l2o = new QRadioButton(tr("L2O CV"));
    connect(m_cv_l2o, &QPushButton::clicked, this, [this]() {
        this->m_cvtype = 2;
    });
    m_cv_loo->setChecked(true);

    m_cv_lxo = new QRadioButton(tr("LXO CV"));
    connect(m_cv_lxo, &QPushButton::clicked, this, [this]() {
        this->m_cvtype = 3;
    });

    m_cv_local = new QCheckBox(tr("Include Local Parameter"));

    m_cv_x = new QSpinBox;
    m_cv_x->setValue(3);
    m_cv_x->setPrefix(tr("X = "));

    connect(m_crossvalidation, &QPushButton::clicked, this, &CompareDialog::CompareCV);
    connect(m_montecarlo, &QPushButton::clicked, this, &CompareDialog::CompareMC);

    layout->addWidget(m_cv_loo, 0, 0);
    layout->addWidget(m_cv_l2o, 0, 1);
    layout->addWidget(m_cv_lxo, 0, 3);
    layout->addWidget(m_cv_local, 1, 0, 1, 2);
    layout->addWidget(m_cv_x, 1, 3);

    layout->addWidget(m_crossvalidation, 2, 0, 1, 2);
    layout->addWidget(m_montecarlo, 2, 3);

    crossvalidation->setLayout(layout);

    layout = new QGridLayout;

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(reduction);
    hlayout->addWidget(aic);
    hlayout->addWidget(crossvalidation);

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
