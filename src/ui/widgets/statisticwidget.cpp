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
#include "src/core/toolset.h"

#include "src/core/models.h"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QTextEdit>
#include "statisticwidget.h"


StatisticWidget::StatisticWidget(const QSharedPointer<AbstractModel> model, QWidget *parent) : QWidget(parent), m_model(model)
{
    
    QVBoxLayout *m_layout = new QVBoxLayout;    
    m_overview = new QTextEdit;
    m_overview->setReadOnly(true);
    QPalette p = m_overview->palette();
    
    p.setColor(QPalette::Active, QPalette::Base, Qt::gray);
    p.setColor(QPalette::Inactive, QPalette::Base, Qt::gray);
    p.setColor(QPalette::Inactive, QPalette::Text, Qt::black);
    p.setColor(QPalette::Inactive, QPalette::Window, Qt::black);
    m_overview->setPalette(p);
    m_layout->addWidget(m_overview);
    
    
    connect(m_model.data(), SIGNAL(StatisticChanged()), this, SLOT(Update()));
    setLayout(m_layout);
    Update();
}

StatisticWidget::~StatisticWidget()
{
    
}

void StatisticWidget::toggleView()
{
    bool hidden = m_subwidget->isHidden();
    m_subwidget->setHidden(!hidden);
}

void StatisticWidget::Update()
{
    QString overview("<table style=\'width:100%\'>");
    overview +=  "<tr><td>Parameter fitted:</t><td><b>" + QString::number(m_model.data()->Parameter()) + "</b></td></tr>\n";
    overview +=  "<tr><td>Number of used Points:</t><td><b>" + QString::number(m_model.data()->Points()) + "</b></td></tr>\n";
    overview +=  "<tr><td>Degrees of Freedom:</t><td><b>" + QString::number(m_model.data()->Points()-m_model.data()->Parameter()) + "</b></td></tr>\n";
    overview +=  "<tr><td>Error: (squared / absolute)</td><td><b>" + QString::number(m_model->SumofSquares()) + "/" + QString::number(m_model->SumofAbsolute()) + "</b></td></tr>\n";
    overview +=  "<tr><td>Mean Error in Model:</td><td><b> " + QString::number(m_model.data()->MeanError()) + "</b></td></tr>\n";
    overview +=  "<tr><td>Variance of Error:</td><td><b>" + QString::number(m_model.data()->Variance())  +"</b></td></tr>\n";
    overview +=  "<tr><td>Standard deviation:</td><td><b>"+  QString::number(m_model.data()->StdDeviation()) +"</b></td></tr>\n";
    overview +=  "<tr><td>Standard Error:</td><td><b>"  + QString::number(m_model.data()->StdError()) + "</b></td></tr>\n";
    overview += "<tr><td>SE<sub>y</sub>:</td><td><b>"  + QString::number(m_model.data()->SEy()) + "</b></td></tr>\n";
    overview += "<tr><td>&chi;:</td><td><b>"  + QString::number(m_model.data()->ChiSquared()) + "</b></td></tr>\n";
    overview += "<tr><td>cov<sub>fit</sub>:</td><td><b>"  + QString::number(m_model.data()->CovFit()) + "</b></td></tr>\n";

    overview += "</table>";
    
    m_short = overview;
    overview.clear();
    QString moco;
    moco += "<p><b>Model Comparison Results:</b></p>\n";
    moco+= "<table>\n";
    /*
    for(int i = 0; i < m_model->getMoCoStatisticResult(); ++i)
    {
        QJsonObject result = m_model->getMoCoStatisticResult(i);   
        QJsonObject confidence = result["confidence"].toObject();
        if(result["method"].toString() == "model comparison" && !i)
            moco += "<tr><td>Approximated area of the confidence ellipse: <b>" + QString::number(result["moco_area"].toDouble()) + "</b></td></tr></p>\n";
        else
            if(m_model->GlobalParameterSize() > 1 && !i)
                moco += "*** Obtained from Automatic Confidence Calculation ***\n";
        if(!result["controller"].toObject()["fisher"].toBool() && !i)
            moco += "<font color =\'red\'>Please be aware, that these values don't base on F-statistics!</font>\n";
        moco += Print::TextFromConfidence(result,m_model.data());
 
    }
    moco += "</table>\n";
    if(m_model->getMoCoStatisticResult())
        overview += moco;
    
    QString cv;
    cv += "<p><b>Weakened Grid Search:</b></p>\n";  
    cv += "<table>\n"; 
    for(int i = 0; i < m_model->getWGStatisticResult(); ++i)
    {
        QJsonObject result = m_model->getWGStatisticResult(i);   
        QJsonObject confidence = result["confidence"].toObject();
        if(!result["controller"].toObject()["fisher"].toBool() && i == 0)
            cv += "<tr><th colspan=2><font color =\'red\'>Please be aware, that these values don't base on F-statistics!</font></th></tr>\n";
        cv += Print::TextFromConfidence(result, m_model.data());
    }
    cv += "</table>\n"; 
    if(m_model->getWGStatisticResult())
        overview += cv;
    
    
    QString mc;
    mc += "<p><b>Monte Carlo Simulation Results:</b></p>\n";
    mc += "<table>\n";
    for(int i = 0; i < m_model->getMCStatisticResult(); ++i)
    {
        QJsonObject result = m_model->getMCStatisticResult(i);   
        QJsonObject confidence = result["confidence"].toObject();
        
        mc += Print::TextFromConfidence(result, m_model.data());
    }
    mc += "</table>\n";
    if(m_model->getMCStatisticResult())
        overview += mc;   
    */
    m_overview->setText(m_short + overview);
    m_statistics = overview;
}

#include "statisticwidget.moc"
