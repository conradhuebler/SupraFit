/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#include <QDebug>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>

#include <QtCore/QMap>
#include <QtCore/QMultiHash>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTextEdit>

#include "src/core/libmath.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/widgets/listchart.h"

#include "regressionanalysisdialog.h"

RegressionAnalysisDialog::RegressionAnalysisDialog(QWeakPointer<DataClass > data,  QWeakPointer<ChartWrapper> wrapper, QWidget *parent) : QDialog(parent), m_data(data), m_wrapper(wrapper)
{
    setUI();
    resize(800,600);
}

RegressionAnalysisDialog::~RegressionAnalysisDialog()
{
}

void RegressionAnalysisDialog::setUI()
{
    m_fit = new QPushButton(tr("Fit linear"));
    m_functions = new QSpinBox;
    m_functions->setRange(1,2);
    
    m_chart = new ListChart;
    
    m_output = new QTextEdit;
    
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_chart);
    splitter->addWidget(m_output);
    
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel(tr("Number of functions")), 0, 0);
    layout->addWidget(m_functions, 0, 1);
    layout->addWidget(m_fit, 0, 2);
    layout->addWidget(splitter, 1, 0, 1, 3);
    setLayout(layout);
    
    connect(m_fit, &QPushButton::clicked, this, &RegressionAnalysisDialog::FitFunctions);
    connect(m_wrapper.data(), &ChartWrapper::ModelTransformed, this, &RegressionAnalysisDialog::UpdatePlots);
}

void RegressionAnalysisDialog::UpdatePlots()
{
    m_chart->Clear();
    m_output->clear();
    m_series = m_wrapper.data()->CloneSeries();
    for(int i = 0; i < m_series.size(); ++i)
        m_chart->addSeries(m_series[i], i, m_series[i]->color(), QString::number(i + 1));
}


void RegressionAnalysisDialog::FitFunctions()
{
    QString output("");
    for(int i = 0; i < m_series.size(); ++i)
    {
        QVector<qreal> x, y;
        QList<QPointF> points = m_series[i]->points();
        for(int j = 0; j < points.size(); ++j)
        {
                x << points[j].x();
                y << points[j].y();
        }
        QMap<qreal, MultiRegression> result = LeastSquares(x, y, m_functions->value());
        
        /*        
        QMap<qreal, MultiRegression>::const_iterator iter = result.constBegin();
        while (iter != result.constEnd()) {
            qDebug() << iter.key() << ": " << iter.value().regressions.size();
            ++iter;
        }
        */
        MultiRegression regression = result.first();
        for(int m = 0; m < regression.regressions.size(); ++m)
        {
            QtCharts::QLineSeries *series = m_chart->addLinearSeries(regression.regressions[m].m, regression.regressions[m].n, x[regression.start[2*m]], x[regression.start[2*m+1]], i);
            series->setColor(m_series[i]->color());
            m_linear_series.insert(i, series);
        }
        /*
         * Sometimes some sophisticated will be placed here
         */
        if(m_functions->value() == 1)
        {
            output += "<p>Series " + QString::number(i + 1) + ": y = " + QString::number(regression.regressions[0].m) + "x + " + QString::number(regression.regressions[0].n) + " ( R<sup>2</sup>=" + QString::number(regression.regressions[0].R)+ ")</ p>";
        }else if(m_functions->value() == 2)
        {
            qreal x = (regression.regressions[0].n-regression.regressions[1].n)/(regression.regressions[1].m-regression.regressions[0].m);
            qreal y = regression.regressions[1].m*x+regression.regressions[1].n;
            output += "<p>Series " + QString::number(i + 1) + ": y1 = " + QString::number(regression.regressions[0].m) + "x + " + QString::number(regression.regressions[0].n) + " (Points: " + QString::number(regression.regressions[0].x.size()) +", R<sup>2</sup>=" + QString::number(regression.regressions[0].R)+ "): y2 = " + QString::number(regression.regressions[1].m)+ "x + " + QString::number(regression.regressions[1].n) + " (Points: " + QString::number(regression.regressions[1].x.size())+", R<sup>2</sup>=" + QString::number(regression.regressions[1].R) + ") </ p>";
            output += "<p>Intersection at (<font color='red'>" +  QString::number(x) + "," + QString::number(y) + "</font>)</p>";
        }
    }
    m_output->append(output);
}
