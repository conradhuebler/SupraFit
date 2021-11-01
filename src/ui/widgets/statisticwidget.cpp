/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/models.h"

#include "src/ui/widgets/textwidget.h"

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "statisticwidget.h"

StatisticWidget::StatisticWidget(const QSharedPointer<AbstractModel> model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{

    QVBoxLayout* m_layout = new QVBoxLayout;
    m_overview = new TextWidget;
    QPalette p = m_overview->palette();

    p.setColor(QPalette::Active, QPalette::Base, Qt::lightGray);
    p.setColor(QPalette::Inactive, QPalette::Base, Qt::lightGray);
    p.setColor(QPalette::Inactive, QPalette::Text, Qt::black);
    p.setColor(QPalette::Inactive, QPalette::Window, Qt::black);
    m_overview->setPalette(p);
    m_layout->addWidget(m_overview);

    connect(m_model.toStrongRef().data(), SIGNAL(StatisticChanged()), this, SLOT(Update()));
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
    overview += "<tr><td>Parameter fitted:</t><td><b>" + Print::printDouble(m_model.toStrongRef().data()->Parameter()) + "</b></td></tr>\n";
    overview += "<tr><td>Number of used Points:</t><td><b>" + Print::printDouble(m_model.toStrongRef().data()->Points()) + "</b></td></tr>\n";
    overview += "<tr><td>Degrees of Freedom:</t><td><b>" + Print::printDouble(m_model.toStrongRef().data()->Points() - m_model.toStrongRef().data()->Parameter()) + "</b></td></tr>\n";
    overview += "<tr><td>Error: (squared / absolute)</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->SSE()) + "/" + Print::printDouble(m_model.toStrongRef().data()->SAE()) + "</b></td></tr>\n";
    overview += "<tr><td>Error Threshold (f-Test)</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->ErrorfTestThreshold(qApp->instance()->property("p_value").toDouble())) + "</b></td></tr>\n";
    overview += "<tr><td>R<sup>2</sup></td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->RSquared()) + "</b></td></tr>\n";
    overview += "<tr><td>f-Value</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->finv(qApp->instance()->property("p_value").toDouble())) + "</b></td></tr>\n";
    overview += "<tr><td>Mean Error in Model:</td><td><b> " + Print::printDouble(m_model.toStrongRef().data()->MeanError()) + "</b></td></tr>\n";
    overview += "<tr><td>Variance of Error:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->Variance()) + "</b></td></tr>\n";
    overview += "<tr><td>Standard deviation &sigma;<sub>fit</sub>:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->StdDeviation()) + "</b></td></tr>\n";
    overview += "<tr><td>Standard Error:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->StdError()) + "</b></td></tr>\n";
    overview += "<tr><td>SE<sub>y</sub>:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->SEy()) + "</b></td></tr>\n";
    overview += "<tr><td>&chi;:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->ChiSquared()) + "</b></td></tr>\n";
    //overview += "<tr><td>cov<sub>fit</sub>:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->CovFit()) + "</b></td></tr>\n";

    overview += "</table></br>";
    overview += m_model.toStrongRef().data()->ModelInfo() + "</br>";
    // overview += m_model.toStrongRef().data()->AdditionalOutput();

    m_short = overview;
    overview.clear();
    QString moco;

    QJsonObject result = m_model.toStrongRef().data()->getFastConfidence();
    if (!m_model.toStrongRef().data()->getFastConfidence().isEmpty()) {
        moco += "<p><b>Fast Confidence Results:</b></p>\n";
        moco += "<table>\n";

        for (int i = 0; i < result.count() - 1; ++i) {
            QJsonObject data = result.value(QString::number(i)).toObject();
            if (data.isEmpty())
                continue;
            moco += Print::TextFromConfidence(data, result["controller"].toObject());
        }
        moco += "</table>\n";
        overview += moco;
    }
    m_overview->setText(m_short + overview);
    m_statistics = overview;
}

#include "statisticwidget.moc"
