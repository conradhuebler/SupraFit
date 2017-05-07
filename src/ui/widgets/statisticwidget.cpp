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
#include "src/core/models.h"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QTextEdit>
#include "statisticwidget.h"


StatisticWidget::StatisticWidget(const QSharedPointer<AbstractTitrationModel> model, QWidget *parent) : m_model(model), QWidget(parent)
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

QString StatisticWidget::TextFromConfidence(const QJsonObject &result)
{
    QString text;
    qreal value = result["value"].toDouble();
    QString pot;
    QString nr;
    QString const_name;
    if(result["type"] == "Complexation Constant")
    {
        nr = QString::number(qPow(10,value));
        pot = "10^";
        const_name = " complexation constant ";
    }
    QJsonObject confidence = result["confidence"].toObject();
    qreal upper = confidence["upper"].toDouble();
    qreal lower = confidence["lower"].toDouble();
    text = "<p><table> <tr><td><b>" + result["name"].toString() + const_name + ":</b></td><td> <b>" + pot + QString::number(value) + " = " + nr + " -- " + pot + "(+ " + QString::number(upper-value) + "/" + QString::number(lower-value) + ") -- </b></td></tr> ";
    text += "<tr><td>95% Confidence Intervall=</td><td> <b>" +pot + QString::number(lower) + " -" + pot + QString::number(upper) + "</b></td></tr></p>\n"; 
    text += "</table>";
    return text;    
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
    overview += "</table>";
    
    m_short = overview;
    overview.clear();
    QString moco;
    moco += "<p><b>Model Comparison Results:</b></p>\n";
    for(int i = 0; i < m_model->getMoCoStatisticResult(); ++i)
    {
        QJsonObject result = m_model->getMoCoStatisticResult(i);   
        QJsonObject confidence = result["confidence"].toObject();
        if(result["method"].toString() == "model comparison" && !i)
            moco += "<tr><td>Approximated area of the confidence ellipse: <b>" + QString::number(result["moco_area"].toDouble()) + "</b></td></tr></p>\n";
        if(!result["controller"].toObject()["fisher"].toBool() && !i)
            moco += "<font color =\'red\'>Please be aware, that these values don't base on F-statistics!</font>\n";
        moco += TextFromConfidence(result);
 
    }
    if(m_model->getMoCoStatisticResult())
        overview += moco;
    
    QString cv;
    cv += "<p><b>Continuouse Variation:</b></p>\n";    
    for(int i = 0; i < m_model->getCVStatisticResult(); ++i)
    {
        QJsonObject result = m_model->getCVStatisticResult(i);   
        QJsonObject confidence = result["confidence"].toObject();
        if(!result["controller"].toObject()["fisher"].toBool() && i == 0)
            cv += "<font color =\'red\'>Please be aware, that these values don't base on F-statistics!</font>\n";
        cv += TextFromConfidence(result);
    }
    if(m_model->getCVStatisticResult())
        overview += cv;
    
    
    QString mc;
    mc += "<p><b>Monte Carlo Simulation Results:</b></p>\n";
    for(int i = 0; i < m_model->getMCStatisticResult(); ++i)
    {
        QJsonObject result = m_model->getMCStatisticResult(i);   
        QJsonObject confidence = result["confidence"].toObject();
        
        mc += TextFromConfidence(result);
    }
    if(m_model->getMCStatisticResult())
        overview += mc;   
    m_overview->setText(m_short + overview);
    m_statistics = overview;
}

#include "statisticwidget.moc"
