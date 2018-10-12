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

#include "src/capabilities/abstractsearchclass.h"
#include "src/capabilities/montecarlostatistics.h"

#include "src/core/AbstractModel.h"
#include "src/core/toolset.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/waiter.h"
#include "src/ui/widgets/chartview.h"
#include "src/ui/widgets/listchart.h"
#include "src/ui/widgets/results/contourwidget.h"
#include "src/ui/widgets/statisticwidget.h"

#include <QtCore/QHash>
#include <QtCore/QPointer>

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>

#include <QtCharts/QAreaSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBoxPlotSeries>
#include <QtCharts/QBoxSet>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>

#include "mcresultswidget.h"

MCResultsWidget::MCResultsWidget(const QJsonObject& data, QSharedPointer<AbstractModel> model, ChartWrapper* wrapper, const QList<QJsonObject>& models)
{
    m_data = data;
    m_model = model;
    m_wrapper = wrapper;
    m_models = models;

    has_boxplot = false;
    has_histogram = false;
    has_contour = false;
}

MCResultsWidget::~MCResultsWidget()
{
}

void MCResultsWidget::setUi()
{
    m_bins = new QSpinBox;
    m_bins->setMinimum(1);
    m_bins->setMaximum(1e3);
    m_bins->setValue(30);
    m_bins->setSingleStep(50);

    QWidget* widget = new QWidget;
    QTabWidget* tabs = new QTabWidget;
    tabs->setTabPosition(QTabWidget::South);
    QGridLayout* layout = new QGridLayout;
    layout->addWidget(tabs, 0, 0, 1, 7);

    m_histgram = MakeHistogram();
    //m_box = MakeBoxPlot();
    if (has_histogram)
        tabs->addTab(m_histgram, tr("Histogram"));

    // if (has_boxplot)
    //     tabs->addTab(m_box, tr("Boxplot"));

    m_contour = MakeContour();
    tabs->addTab(m_contour, tr("Contour Plot"));

    m_save = new QPushButton(tr("Export Results"));
    connect(m_save, SIGNAL(clicked()), this, SLOT(ExportResults()));

    m_error = new QDoubleSpinBox;
    m_error->setValue(95);
    m_error->setSingleStep(0.5);
    m_error->setSuffix(tr("%"));
    m_error->setMaximum(100);
    connect(m_error, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MCResultsWidget::GenerateConfidence);


    connect(m_bins, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MCResultsWidget::GenerateHistogram);

    layout->addWidget(new QLabel(tr("Confidence Intervall")), 1, 0);
    layout->addWidget(m_error, 1, 1);

    layout->addWidget(new QLabel(tr("Bins in Histogram")), 1, 2);
    layout->addWidget(m_bins, 1, 3);

    /*
    if (m_models.size())
        layout->addWidget(m_save, 1, 2);*/

    widget->setLayout(layout);

    setLayout(layout);
    GenerateConfidence(95);
}

QPointer<ListChart> MCResultsWidget::MakeHistogram()
{
    QPointer<ListChart> view = new ListChart;
    view->setXAxis("parameter");
    view->setYAxis("relative rate");
    view->setMinimumSize(300, 400);
    bool formated = false;


    QJsonObject controller = m_data["controller"].toObject();
    int bins = controller["bins"].toInt(30);
    m_bins->setValue(bins);

    for (int i = 0; i < m_data.count() - 1; ++i) {
        QJsonObject data = m_data[QString::number(i)].toObject();
        if (data.isEmpty())
            continue;
        QString name = data["name"].toString();
        qreal x_0 = data["value"].toDouble();
        QVector<qreal> list = ToolSet::String2DoubleVec(data["data"].toObject()["raw"].toString());

        QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(list, bins);
        ToolSet::Normalise(histogram);
        LineSeries* xy_series = new LineSeries;
        m_linked_data.insert(xy_series, list);

        if (data["type"] == "Local Parameter") {
            if (!data.contains("index"))
                continue;
            int index = data["index"].toString().split("|")[1].toInt();
            /*xy_series->setColor(m_wrapper->Series(index)->color());
            connect(m_wrapper->Series(index), &QtCharts::QXYSeries::colorChanged, xy_series, &LineSeries::setColor);
            connect(m_wrapper->Series(index), &QtCharts::QXYSeries::colorChanged, this, [i, view](const QColor& color) { view->setColor(i, color); });
            connect(m_wrapper->Series(index), &QtCharts::QXYSeries::colorChanged, this, [index, this](const QColor& color) { this->setAreaColor(index, color); });
            */
        } else {
            //xy_series->setColor(ChartWrapper::ColorCode(m_model->Color(i)));
        }

        qreal diff = (histogram.last().first-histogram.first().first)/double(histogram.size());
        xy_series->append(QPointF(histogram.first().first - diff, 0));
        for (int j = 0; j < histogram.size(); ++j) {
            xy_series->append(QPointF(histogram[j].first, histogram[j].second));
        }
        xy_series->append(QPointF(histogram.last().first + diff, 0));

        if (histogram.size())
            has_histogram = true;
        xy_series->setName(name);
        view->addSeries(xy_series, i, xy_series->color(), name);
        view->setColor(i, xy_series->color());
        if (!formated)
            view->formatAxis();
        formated = true;

        LineSeries* current_constant = new LineSeries;
        connect(xy_series, &QtCharts::QXYSeries::colorChanged, current_constant, &LineSeries::setColor);
        current_constant->setDashDotLine(true);
        *current_constant << QPointF(x_0, 0) << QPointF(x_0, 1.25);
        current_constant->setColor(xy_series->color());
        current_constant->setName(name);
        view->addSeries(current_constant, i, xy_series->color(), name, true);

        QJsonObject confidenceObject = data["confidence"].toObject();
        if (view) {
            QtCharts::QAreaSeries* area_series = AreaSeries(xy_series->color());
            view->addSeries(area_series, i, area_series->color(), name);
            m_area_series << area_series;
        }
        m_colors << xy_series->color();
    }

    return view;
}

QPointer<ListChart> MCResultsWidget::MakeBoxPlot()
{
    QPointer<ListChart> boxplot = new ListChart;
    double min = 10, max = 0;

    for (int i = 0; i < m_data.count() - 1; ++i) {
        QJsonObject data = m_data[QString::number(i)].toObject();
        if (data.isEmpty())
            continue;

        has_boxplot = true;

        SupraFit::BoxWhisker bw = ToolSet::Object2Whisker(data["boxplot"].toObject());
        min = qMin(bw.lower_whisker, min);
        max = qMax(bw.upper_whisker, max);
        BoxPlotSeries* series = new BoxPlotSeries(bw);
        series->setName(data["name"].toString());

        if (data["type"] == "Local Parameter") {
            if (!data.contains("index"))
                continue;
            int index = data["index"].toString().split("|")[1].toInt();
            series->setBrush(m_wrapper->Series(index)->color());
            connect(m_wrapper->Series(index), &QtCharts::QXYSeries::colorChanged, series, &BoxPlotSeries::setColor);
            connect(m_wrapper->Series(index), &QtCharts::QXYSeries::colorChanged, this, [i, boxplot](const QColor& color) {if(boxplot) boxplot->setColor(i, color); });
        } else
            series->setBrush(ChartWrapper::ColorCode(m_model.data()->Color(i)));

        boxplot->addSeries(series, i, series->color(), data["name"].toString());
        boxplot->setColor(i, series->color());
        m_box_object << ToolSet::Box2Object(bw);
    }

    if (has_boxplot) {
        QtCharts::QValueAxis* y_axis = qobject_cast<QtCharts::QValueAxis*>(boxplot->Chart()->axisY());
        y_axis->setMin(min * 0.99);
        y_axis->setMax(max * 1.01);
    }
    return boxplot;
}

QPointer<QWidget> MCResultsWidget::MakeContour()
{
    ContourWidget* widget = new ContourWidget;
    widget->setData(m_models, m_model);
    QJsonObject controller = m_data["controller"].toObject();

    return widget;
}

void MCResultsWidget::UpdateBoxes()
{
    int elements = m_data.count() - 1;
    for (int i = 0; i < elements; ++i) {
        QJsonObject data = m_data[QString::number(i)].toObject();
        if (data.isEmpty())
            continue;
        QJsonObject confidenceObject = data["confidence"].toObject();
        if (m_histgram && i < m_area_series.size()) {
            QtCharts::QAreaSeries* area_series = m_area_series[i];
            QtCharts::QLineSeries* series1 = area_series->lowerSeries();
            QtCharts::QLineSeries* series2 = area_series->upperSeries();

            series1->clear();
            series2->clear();

            *series1 << QPointF(confidenceObject["lower"].toVariant().toDouble(), 0) << QPointF(confidenceObject["lower"].toVariant().toDouble(), 0.66);
            *series2 << QPointF(confidenceObject["upper"].toVariant().toDouble(), 0) << QPointF(confidenceObject["upper"].toVariant().toDouble(), 0.66);

            area_series->setLowerSeries(series1);
            area_series->setUpperSeries(series2);
            area_series->setName(m_model.data()->GlobalParameterName(i));
#warning why is this always a global parameter?
        }
    }
}

void MCResultsWidget::ExportResults()
{
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)"));
    if (str.isEmpty())
        return;
    Waiter wait;
    setLastDir(str);
    ToolSet::ExportResults(str, m_models);
}

QtCharts::QAreaSeries* MCResultsWidget::AreaSeries(const QColor& color) const
{
    QtCharts::QLineSeries* series1 = new QtCharts::QLineSeries();
    QtCharts::QLineSeries* series2 = new QtCharts::QLineSeries();
    QtCharts::QAreaSeries* area_series = new QtCharts::QAreaSeries(series1, series2);
    QPen pen(0x059605);
    pen.setWidth(3);
    area_series->setPen(pen);

    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
    gradient.setColorAt(0.0, color);
    gradient.setColorAt(1.0, 0x26f626);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    area_series->setBrush(gradient);
    area_series->setOpacity(0.4);
    return area_series;
}

void MCResultsWidget::setAreaColor(int index, const QColor& color)
{
    if (index >= m_area_series.size())
        return;
    QtCharts::QAreaSeries* area_series = m_area_series[index];

    QPen pen(0x059605);
    pen.setWidth(3);
    area_series->setPen(pen);

    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
    gradient.setColorAt(0.0, color);
    gradient.setColorAt(1.0, 0x26f626);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    area_series->setBrush(gradient);
    area_series->setOpacity(0.4);
}

void MCResultsWidget::GenerateConfidence(double error)
{
    for (int i = 0; i < m_data.count() - 1; ++i) {
        QJsonObject data = m_data[QString::number(i)].toObject();
        if (data.isEmpty())
            continue;
        QList<qreal> list = ToolSet::String2DoubleList(data["data"].toObject()["raw"].toString());
        SupraFit::ConfidenceBar bar = ToolSet::Confidence(list, 100 - error);
        QJsonObject confidence;
        confidence["lower"] = bar.lower;
        confidence["upper"] = bar.upper;
        confidence["error"] = error;
        data["confidence"] = confidence;
        m_data[QString::number(i)] = data;
    }
    UpdateBoxes();
    m_model.data()->UpdateStatistic(m_data);
    emit ConfidenceUpdated(m_data);
}

void MCResultsWidget::GenerateHistogram()
{

    QHash<LineSeries*, QVector<qreal>>::iterator i = m_linked_data.begin();

    while (i != m_linked_data.end()) {
        LineSeries* xy_series = i.key();

        QVector<qreal> list = i.value();
        QVector<QPair<qreal, qreal>> histogram = ToolSet::List2Histogram(list, m_bins->value());
        ToolSet::Normalise(histogram);

        xy_series->clear();

        qreal diff = (histogram.last().first-histogram.first().first)/double(histogram.size());

        xy_series->append(QPointF(histogram.first().first - diff, 0));
        for (int j = 0; j < histogram.size(); ++j) {
            xy_series->append(QPointF(histogram[j].first, histogram[j].second));
        }
        xy_series->append(QPointF(histogram.last().first + diff, 0));


        ++i;
    }
    QJsonObject controller = m_data["controller"].toObject();
    controller["bins"] = m_bins->value();
    m_data["controller"] = controller;

    emit ConfidenceUpdated(m_data);
}

#include "mcresultswidget.moc"
