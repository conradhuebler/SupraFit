/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "statisticwidget.h"
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

StatisticWidget::StatisticWidget(const QSharedPointer<AbstractModel> model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{

    QVBoxLayout* m_layout = new QVBoxLayout;
    m_overview = new QTextEdit;
    m_overview->setReadOnly(true);
    QPalette p = m_overview->palette();

    p.setColor(QPalette::Active, QPalette::Base, Qt::lightGray);
    p.setColor(QPalette::Inactive, QPalette::Base, Qt::lightGray);
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
    overview += "<tr><td>Parameter fitted:</t><td><b>" + Print::printDouble(m_model.data()->Parameter()) + "</b></td></tr>\n";
    overview += "<tr><td>Number of used Points:</t><td><b>" + Print::printDouble(m_model.data()->Points()) + "</b></td></tr>\n";
    overview += "<tr><td>Degrees of Freedom:</t><td><b>" + Print::printDouble(m_model.data()->Points() - m_model.data()->Parameter()) + "</b></td></tr>\n";
    overview += "<tr><td>Error: (squared / absolute)</td><td><b>" + Print::printDouble(m_model->SumofSquares()) + "/" + Print::printDouble(m_model->SumofAbsolute()) + "</b></td></tr>\n";
    overview += "<tr><td>Mean Error in Model:</td><td><b> " + Print::printDouble(m_model.data()->MeanError()) + "</b></td></tr>\n";
    overview += "<tr><td>Variance of Error:</td><td><b>" + Print::printDouble(m_model.data()->Variance()) + "</b></td></tr>\n";
    overview += "<tr><td>Standard deviation &sigma;:</td><td><b>" + Print::printDouble(m_model.data()->StdDeviation()) + "</b></td></tr>\n";
    overview += "<tr><td>Standard Error:</td><td><b>" + Print::printDouble(m_model.data()->StdError()) + "</b></td></tr>\n";
    overview += "<tr><td>SE<sub>y</sub>:</td><td><b>" + Print::printDouble(m_model.data()->SEy()) + "</b></td></tr>\n";
    overview += "<tr><td>&chi;:</td><td><b>" + Print::printDouble(m_model.data()->ChiSquared()) + "</b></td></tr>\n";
    overview += "<tr><td>cov<sub>fit</sub>:</td><td><b>" + Print::printDouble(m_model.data()->CovFit()) + "</b></td></tr>\n";

    overview += "</table></br>";
    overview += m_model.data()->ModelInfo() + "</br>";
    overview += m_model.data()->AdditionalOutput();

    m_short = overview;
    overview.clear();
    QString moco;

    QJsonObject result = m_model->getFastConfidence();
    if (!m_model->getFastConfidence().isEmpty()) {
        moco += "<p><b>Fast Confidence Results:</b></p>\n";
        moco += "<table>\n";

        if (!result["controller"].toObject()["fisher"].toBool())
            moco += "<font color =\'red\'>Please be aware, that these values don't base on F-statistics!</font>\n";

        for (int i = 0; i < result.count() - 1; ++i) {
            QJsonObject data = result.value(QString::number(i)).toObject();
            if (data.isEmpty())
                continue;
            moco += Print::TextFromConfidence(data, m_model.data(), result["controller"].toObject());
        }
        moco += "</table>\n";
        overview += moco;
    }
    /*moco = QString();

    if (!m_model->getMoCoStatisticResult().isEmpty()) {
        moco += "<p><b>Model Comparison Results:</b></p>\n";
        moco += "<table>\n";

        result = m_model->getMoCoStatisticResult();
        moco += "<tr><td>Approximated area of the confidence ellipse: <b>" + QString::number(result["moco_area"].toDouble()) + "</b></td></tr></p>\n";

        if (!result["controller"].toObject()["fisher"].toBool())
            moco += "<font color =\'red\'>Please be aware, that these values don't base on F-statistics!</font>\n";

        for (int i = 0; i < result.count() - 1; ++i) {
            QJsonObject data = result.value(QString::number(i)).toObject();
            if (data.isEmpty())
                continue;
            moco += Print::TextFromConfidence(data, m_model.data(), result["controller"].toObject());
        }
        moco += "</table>\n";
        overview += moco;
    }

    if (!m_model->getWGStatisticResult().isEmpty()) {
        QString cv;

        cv += "<p><b>Weakened Grid Search:</b></p>\n";
        cv += "<table>\n";

        result = m_model->getWGStatisticResult();
        if (!result["controller"].toObject()["fisher"].toBool())
            cv += "<tr><th colspan=2><font color =\'red\'>Please be aware, that these values don't base on F-statistics!</font></th></tr>\n";

        for (int i = 0; i < result.count() - 1; ++i) {
            QJsonObject data = result.value(QString::number(i)).toObject();
            if (data.isEmpty())
                continue;
            cv += Print::TextFromConfidence(data, m_model.data(), result["controller"].toObject());
        }
        cv += "</table>\n";
        overview += cv;
    }

    if (!m_model->getReduction().isEmpty()) {
        QString cv;

        cv += "<p><b>Reduction Analysis:</b></p>\n";
        cv += "<table>\n";

        result = m_model->getReduction();
        for (int i = 0; i < result.count() - 1; ++i) {
            QJsonObject data = result.value(QString::number(i)).toObject();
            if (data.isEmpty())
                continue;
            cv += Print::TextFromConfidence(data, m_model.data(), result["controller"].toObject());
        }
        cv += "</table>\n";
        overview += cv;
    }

    QString mc;
    if (m_model->getMCStatisticResult()) {
        mc += "<p><b>Monte Carlo Simulation Results:</b></p>\n";
        mc += "<table>\n";
        for (int i = 0; i < m_model->getMCStatisticResult(); ++i) {
            QJsonObject result = m_model->getMCStatisticResult(i);

            for (int i = 0; i < result.count() - 1; ++i) {
                QJsonObject data = result.value(QString::number(i)).toObject();
                if (data.isEmpty())
                    continue;
                mc += Print::TextFromConfidence(data, m_model.data(), result["controller"].toObject());
            }
        }

        mc += "</table>\n";
        overview += mc;
    }*/
    m_overview->setText(m_short + overview);
    m_statistics = overview;
}

#include "statisticwidget.moc"
