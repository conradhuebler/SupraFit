/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QAction>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFontDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

#include "chartconfig.h"

ChartConfigDialog::ChartConfigDialog()
{
    setModal(false);
    QGridLayout *layout = new QGridLayout;
    m_x_axis = new QLineEdit;
    connect(m_x_axis, SIGNAL(textChanged(QString)), this, SLOT(Changed()));
    m_y_axis = new QLineEdit;
    connect(m_y_axis, SIGNAL(textChanged(QString)), this, SLOT(Changed()));
    m_x_min = new QDoubleSpinBox;
    connect(m_x_min, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));
    m_x_max = new QDoubleSpinBox;
    connect(m_x_max, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));
    m_x_step = new QSpinBox;
    connect(m_x_step, SIGNAL(valueChanged(int)), this, SLOT(Changed()));
    m_y_min = new QDoubleSpinBox;
    m_y_min->setMinimum(-1*1e7);
    m_y_min->setMaximum(1e7);
    m_y_min->setSingleStep(0.01);
    m_y_min->setDecimals(5);
    connect(m_y_min, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));
    m_y_max = new QDoubleSpinBox;
    m_y_max->setSingleStep(0.01);
    m_y_max->setMinimum(-1*1e7);
    m_y_max->setMaximum(1*1e7);
    m_y_max->setDecimals(5);
    connect(m_y_max, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));
    m_y_step = new QSpinBox;
    connect(m_y_step, SIGNAL(valueChanged(int)), this, SLOT(Changed()));
    
    m_scaleaxis = new QPushButton(tr("Autoscale X"));
    connect(m_scaleaxis, SIGNAL(clicked()), this, SIGNAL(ScaleAxis()));
    
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    m_legend = new QCheckBox(tr("Show Legend"));
    connect(m_legend, &QCheckBox::stateChanged, this, &ChartConfigDialog::Changed);

    m_lock_scaling = new QCheckBox(tr("Lock Scaling"));
    connect(m_lock_scaling, &QCheckBox::stateChanged, this, &ChartConfigDialog::Changed);
    layout->addWidget(new QLabel(tr("x Label:")), 0, 0);
    layout->addWidget(m_x_axis, 0, 1, 1, 3);
    
    layout->addWidget(new QLabel(tr("Axis")), 1, 0);
    layout->addWidget(m_x_min, 1, 1);
    layout->addWidget(m_x_step, 1, 2);
    layout->addWidget(m_x_max, 1, 3);
    
    layout->addWidget(new QLabel(tr("y Label:")), 3, 0);
    layout->addWidget(m_y_axis, 3, 1, 1, 3);
    layout->addWidget(new QLabel(tr("Axis")), 4, 0);
    layout->addWidget(m_y_min, 4, 1);
    layout->addWidget(m_y_step, 4, 2);
    layout->addWidget(m_y_max, 4, 3);
    layout->addWidget(m_legend, 5, 0);
    layout->addWidget(m_scaleaxis, 5, 1);
    layout->addWidget(m_lock_scaling, 5, 2);

    m_labels = new QPushButton(tr("Label Font"));
    connect(m_labels, &QPushButton::clicked, this, &ChartConfigDialog::setLabelFont);

    m_keys = new QPushButton(tr("Legend Font"));
    connect(m_keys, &QPushButton::clicked, this, &ChartConfigDialog::setKeysFont);

    m_ticks = new QPushButton(tr("Ticks Font"));
    connect(m_ticks, &QPushButton::clicked, this, &ChartConfigDialog::setTicksFont);

    m_alignment = new QPushButton(tr("Align Legend"));
    QMenu *align = new QMenu;
    QAction *left = new QAction(tr("Left"));
    left->setData(  Qt::AlignLeft );
    align->addAction(left);

    QAction *right = new QAction(tr("Right"));
    right->setData(  Qt::AlignRight);
    align->addAction(right);

    QAction *top = new QAction(tr("Top"));
    top->setData(  Qt::AlignTop);
    align->addAction(top);

    QAction *bottom = new QAction(tr("Bottom"));
    bottom->setData(  Qt::AlignBottom);
    align->addAction(bottom);
    m_alignment->setMenu(align);

    connect(align, &QMenu::triggered, [this](QAction *action)
    {
        m_chartconfig.align = Qt::Alignment (action->data().toInt());
        emit ConfigChanged(Config());
    }
    );

    layout->addWidget(m_labels, 7, 0);
    layout->addWidget(m_keys, 7, 1);
    layout->addWidget(m_ticks, 7, 2);
    layout->addWidget(m_alignment, 7, 3);
    layout->addWidget(m_buttons, 10, 0, 1, 3);
    setLayout(layout);
    setWindowTitle("Configure charts ...");
}


ChartConfigDialog::~ChartConfigDialog()
{
}

void ChartConfigDialog::setConfig(const ChartConfig& chartconfig)
{
    m_x_min->setValue(chartconfig.x_min);
    m_x_max->setValue(chartconfig.x_max);
    m_x_step->setValue(chartconfig.x_step);
    m_y_min->setValue(chartconfig.y_min);
    m_y_max->setValue(chartconfig.y_max);
    m_y_step->setValue(chartconfig.y_step);
    
    m_x_axis->setText(chartconfig.x_axis);
    m_y_axis->setText(chartconfig.y_axis);
    m_legend->setChecked(chartconfig.m_legend);
    m_lock_scaling->setChecked(chartconfig.m_lock_scaling);
}

void ChartConfigDialog::Changed()
{
    m_chartconfig.x_axis = m_x_axis->text();
    m_chartconfig.y_axis = m_y_axis->text();
    m_chartconfig.x_min = m_x_min->value();
    m_chartconfig.x_max = m_x_max->value();
    m_chartconfig.x_step = m_x_step->value();

    m_chartconfig.y_min = m_y_min->value();
    m_chartconfig.y_max = m_y_max->value();
    m_chartconfig.y_step = m_y_step->value();
    m_chartconfig.m_legend = m_legend->isChecked();
    m_chartconfig.m_lock_scaling = m_lock_scaling->isChecked();

    emit ConfigChanged(Config());
}

void ChartConfigDialog::setKeysFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_chartconfig.m_keys, this);
    if (ok) {
        m_chartconfig.m_keys = font;
        emit ConfigChanged(Config());
    }
}

void ChartConfigDialog::setLabelFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_chartconfig.m_label, this);
    if (ok) {
        m_chartconfig.m_label = font;
        emit ConfigChanged(Config());
    }

}

void ChartConfigDialog::setTicksFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_chartconfig.m_ticks, this);
    if (ok) {
        m_chartconfig.m_ticks = font;
        emit ConfigChanged(Config());
    }
}
#include "chartconfig.moc"
