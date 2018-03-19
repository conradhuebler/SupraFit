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

#include "src/global.h"

#include "src/core/toolset.h"
#include "src/core/dataclass.h"
#include "src/core/AbstractModel.h"

#include "src/ui/widgets/buttons/spinbox.h"
#include "src/ui/widgets/buttons/hovercheckbox.h"
#include "src/ui/mainwindow/chartwidget.h"


#include <QtMath>
#include <QApplication>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QTimer>


#include <QtWidgets/QGroupBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColorDialog>

#include <QtCharts/QXYSeries>
#include <QtCharts/QLineSeries>

#include <QDebug>

#include "modelelement.h"


ModelElement::ModelElement(QSharedPointer<AbstractModel> model, Charts charts, int no, QWidget* parent) : QGroupBox(parent), m_model(model), m_no(no), m_charts(charts)
{
    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *shifts = new QHBoxLayout;
    for(int i = 0; i < m_model->LocalParameterSize(); ++i)
    {
        QPointer<QWidget > widget = new QWidget;

        QPointer<SpinBox >constant = new SpinBox;
        m_constants << constant;
        constant->setMinimum(-1e9);
        constant->setSingleStep(1e-2);
        constant->setDecimals(4);
        constant->setMaximum(1e9);
        constant->setSuffix(m_model->LocalParameterSuffix(i));
        constant->setValue(m_model->LocalParameter(i, m_no));
        constant->setToolTip(m_model->LocalParameterDescription(i));
        constant->setMaximumWidth(130);
        connect(constant, SIGNAL(valueChangedNotBySet(double)), this, SIGNAL(ValueChanged()));
        connect(m_model.data(), &AbstractModel::Recalculated, this, [i, widget, this](  )
        {
            if(this->m_model && widget)
                widget->setEnabled(this->m_model->LocalEnabled(i ));
        });
        QVBoxLayout *vlayout = new QVBoxLayout;
        widget->setLayout(vlayout);
        vlayout->addWidget(constant);
        vlayout->addWidget(new QLabel(m_model.data()->LocalParameterName(i)));
        shifts->addWidget(widget, 0);
    }
    
    if(m_model->Type() != 3)
    {
        m_error = new QLabel;
        shifts->addStretch(150);
        shifts->addWidget(m_error); 
    } 
    layout->addLayout(shifts);
    QHBoxLayout *tools = new QHBoxLayout;
    m_include = new QCheckBox(this);
    m_include->setText("Include");
    m_include->setToolTip(tr("If checked, this signal will be included in model generation. "));
    m_include->setChecked(m_model->ActiveSignals()[m_no]);
    connect(m_include, SIGNAL(clicked()), this, SLOT(toggleActive()));
    tools->addWidget(m_include);

    m_signal_series = qobject_cast<LineSeries *>(m_charts.signal_wrapper->Series(m_no));
    m_error_series = qobject_cast<LineSeries *>(m_charts.error_wrapper->Series(m_no));
    m_name = new QLineEdit;
    m_name->setPlaceholderText( m_signal_series->name());
    m_name->setClearButtonEnabled(true);
    tools->addWidget(m_name);
    connect(m_name, &QLineEdit::textChanged, this, [this](  )
    {
        if(this->m_name && this->m_signal_series)
        {
            if(this->m_name->text() != this->m_signal_series->name())
            {
                this->m_signal_series->setName(this->m_name->text());
                this->m_error_series->setName(this->m_name->text());
            }
        }
    });

    connect(m_signal_series, &LineSeries::nameChanged, this,  [this](  )
    {
        if(this->m_name && this->m_signal_series)
        {
            if(this->m_name->isModified())
            {
                this->m_signal_series->setName(this->m_name->text());
                this->m_error_series->setName(this->m_name->text());
            }
            else
            {
                if(this->m_name->text() != this->m_signal_series->name())
                    this->m_name->setText(this->m_signal_series->name());
            }
        }
    });

    m_error_series->setVisible(m_model->ActiveSignals()[m_no]);
    m_signal_series->setVisible(m_model->ActiveSignals()[m_no]);
    m_show = new HoverCheckBox;
    m_show->setText(tr("Show in Plot"));
    m_show->setToolTip(tr("Show this Curve in Model and Error Plot"));
    m_show->setChecked(m_model->ActiveSignals()[m_no]);
    tools->addWidget(m_show);
    
    m_plot = new QPushButton;
    m_plot->setText(tr("Color"));
    m_plot->setFlat(true);
    m_plot->setMaximumSize(70,30);
    tools->addWidget(m_plot);
    setLayout(layout);
    
    m_toggle = new QPushButton(tr("Single Plot"));
    m_toggle->setFlat(true);
    m_toggle->setCheckable(true);
    m_toggle->setMaximumSize(90,30);
    tools->addWidget(m_toggle);
    connect(m_toggle, SIGNAL(clicked()), this, SLOT(togglePlot()));
    layout->addLayout(tools);
    setMaximumHeight(110);
    setMinimumHeight(110);
    ChangeColor(m_charts.data_wrapper->color(m_no));
    connect(m_charts.data_wrapper->Series(m_no), SIGNAL(colorChanged(QColor)), this, SLOT(ChangeColor(QColor)));
    connect(m_plot, SIGNAL(clicked()), this, SLOT(chooseColor()));
    connect(m_show, SIGNAL(stateChanged(int)), m_signal_series, SLOT(ShowLine(int)));
    connect(m_show, SIGNAL(stateChanged(int)), m_error_series, SLOT(ShowLine(int)));  
    connect(m_charts.data_wrapper, SIGNAL(ShowSeries(int)), this, SLOT(UnCheckToggle(int)));
    toggleActive();
    Update();
}

ModelElement::~ModelElement()
{
    
}

void ModelElement::setReadOnly(bool readonly)
{
    for(int i = 0; i < m_constants.size(); ++i)
        m_constants[i]->setReadOnly(readonly);
}

void ModelElement::toggleActive()
{
    int state = m_include->isChecked();
    DisableSignal(state);
    emit ActiveSignalChanged();
}

void ModelElement::DisableSignal(int state)
{
    m_show->setChecked(state);
    m_error_series->ShowLine(state);
    m_signal_series->ShowLine(state);
    for(int i = 0; i < m_constants.size(); ++i)
    {
        m_constants[i]->setEnabled(m_include->isChecked());
    }
}

bool ModelElement::Include() const
{
    return m_include->isChecked();
}


QVector<double > ModelElement::D() const
{
    QVector<double > numbers;
    for(int i = 0; i < m_constants.size(); ++i)
        numbers << m_constants[i]->value();
    return numbers;
}

void ModelElement::Update()
{
    m_include->setChecked(m_model->ActiveSignals()[m_no]);
    DisableSignal(m_model->ActiveSignals()[m_no]);
    if(!m_include->isChecked())
        return;
    for(int i = 0; i < m_model->LocalParameterSize(); ++i)
    {
        m_constants[i]->setValue(m_model->LocalParameter(i, m_no));
    }
    if(m_model->Type() != 3)
        m_error->setText("Sum of Squares: <b>" + QString::number(m_model->SumOfErrors(m_no), 'e', 2) + "</b>");
    
}

void ModelElement::ChangeColor(const QColor &color)
{
    m_signal_series->setColor(color);
    m_error_series->setColor(color);
    #ifdef _WIN32
    setStyleSheet("background-color:" + color.name()+ ";");
    #else
    QPalette pal = palette();
    pal.setColor(QPalette::Background,color);
    setPalette(pal); 
    #endif
    m_color = color;
}

void ModelElement::setLabel(const QString &str)
{
    if(str.isEmpty() || str.isNull())
        return;
    m_name->setText(str);
    m_name->setModified(true);
}

void ModelElement::chooseColor()
{
    QColor color = QColorDialog::getColor(m_color, this, tr("Choose Color for Series"));
    if(!color.isValid())
        return;
    
    ChangeColor(color);
}

void ModelElement::ToggleSeries(int i)
{
    m_signal_series->setVisible(i);
    m_error_series->setVisible(i);
    m_show->setChecked(i);
}

void ModelElement::togglePlot()
{
    m_show->setChecked(true);
    m_error_series->setVisible(true);
    m_signal_series->setVisible(true);
    if(m_toggle->isChecked())
        m_charts.data_wrapper->showSeries(m_no); 
    else
        m_charts.data_wrapper->showSeries(-1);
}

void ModelElement::UnCheckToggle(int i)
{
    if(i != m_no)
       m_toggle->setChecked(false);
}


#include "modelelement.moc"
