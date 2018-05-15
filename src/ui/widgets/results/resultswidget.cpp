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
#include "src/core/AbstractModel.h"
#include "src/core/toolset.h"
#include "src/ui/guitools/chartwrapper.h"

#include "src/ui/dialogs/modeldialog.h"

#include "src/ui/widgets/chartview.h"
#include "src/ui/widgets/listchart.h"
#include "src/ui/widgets/results/contourwidget.h"
#include "src/ui/widgets/results/mcresultswidget.h"
#include "src/ui/widgets/statisticwidget.h"


#include <QtCore/QPointer>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QWidget>

#include "resultswidget.h"

ResultsWidget::ResultsWidget(const QJsonObject& data, QSharedPointer<AbstractModel> model, ChartWrapper* wrapper)
{
    m_data = data;
    m_model = model;

    QJsonObject controller = m_data["controller"].toObject();
    QJsonObject temp = model->ExportModel(false, false);
    for (int i = 0; i < controller["raw"].toObject().size(); ++i) {
        temp["data"] = controller["raw"].toObject()[QString::number(i)];
        m_models << temp;
    }
    m_wrapper = wrapper;
    m_dialog = new ModalDialog;
    m_text = QString("");
    setUi();
    resize(1024, 600);
}

ResultsWidget::~ResultsWidget()
{
    delete m_dialog;
}

void ResultsWidget::setUi()
{
    QSplitter* splitter = new QSplitter(Qt::Vertical);

    QGridLayout* layout = new QGridLayout;

    m_confidence_label = new QLabel();
    m_confidence_label->setTextFormat(Qt::RichText);
    m_confidence_label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    switch (m_data["controller"].toObject()["method"].toInt()) {
    case SupraFit::Statistic::MonteCarlo:
        m_widget = MonteCarloWidget();
        setObjectName("Monte Carlo Simulation for " + m_model->Name());
        break;

    case SupraFit::Statistic::ModelComparison:
        m_widget = ModelComparisonWidget();
        setObjectName("Model Comparison Confidence for " + m_model->Name());
        break;

    case SupraFit::Statistic::WeakenedGridSearch:
        m_widget = GridSearchWidget();
        setObjectName("Weakend Grid Search Confidence for " + m_model->Name());
        break;

    case SupraFit::Statistic::Reduction:
        m_widget = ReductionWidget();
        setObjectName("Reduction Analysis for " + m_model->Name());
        break;

    case SupraFit::Statistic::CrossValidation:
        m_widget = MonteCarloWidget();
        setObjectName("Cross Validation Estimation for " + m_model->Name());
        break;

    default:
        m_widget = new QWidget;
        break;
    }

    QScrollArea* scroll = new QScrollArea;
    scroll->setWidget(m_confidence_label);
    scroll->setWidgetResizable(true);

    splitter->addWidget(m_widget);
    splitter->addWidget(scroll);
    layout->addWidget(splitter, 0, 0, 1, 4);
    m_detailed = new QPushButton(tr("Detailed"));
    connect(m_detailed, &QPushButton::clicked, this, &ResultsWidget::Detailed);
    layout->addWidget(m_detailed, 1, 3);
    setLayout(layout);
    WriteConfidence(m_data);
}

QWidget* ResultsWidget::MonteCarloWidget()
{
    MCResultsWidget* widget = new MCResultsWidget(m_data, m_model, m_wrapper, m_models);
    connect(widget, &MCResultsWidget::ConfidenceUpdated, this, &ResultsWidget::WriteConfidence);
    widget->setUi();
    return widget;
}

QWidget* ResultsWidget::ReductionWidget()
{
    QStringList text;
    QString parameter_text;
    QPointer<ListChart> view = new ListChart;

    QVector<qreal> x = ToolSet::String2DoubleVec(m_data["controller"].toObject()["x"].toString());
    text << "  X";
    for (int i = 0; i < x.size(); ++i)
        text << Print::printDouble(x[i]);
    for (int i = 0; i < m_data.count() - 1; ++i) {
        QJsonObject data = m_data[QString::number(i)].toObject();
        if (data.isEmpty())
            continue;

        QString name = data["name"].toString();
        LineSeries* serie = new LineSeries;
        serie->setSize(4);
        QList<QPointF> series;
        QVector<qreal> list = ToolSet::String2DoubleVec(data["data"].toObject()["raw"].toString());
        text[0] += "\t  Param " + QString::number(i);
        parameter_text += "Param " + QString::number(i) + " := " + name + "\n";
        for (int i = 0; i < list.size(); ++i) {
            series << QPointF(x[i], list[i]);
            text[i + 1] += "\t" + Print::printDouble(list[i]);
        }

        if (series.isEmpty())
            continue;

        QColor color;
        int index = 0, jndex = 0;
        if (data["type"].toString() == "Global Parameter")
            color = ChartWrapper::ColorCode(m_model->Color(i));
        else {
            if (data.contains("index")) {
                QStringList lindex = data["index"].toString().split("|");
                index = lindex[1].toInt();
                jndex = lindex[0].toInt();
                color = m_wrapper->Series(index)->color();
            }
        }
        serie->append(series);
        serie->setName(name);
        serie->setColor(color);
        view->addSeries(serie, i, color, name);
        view->setColor(i, color);

        serie = new LineSeries;
        serie->setDashDotLine(true);
        qreal value = 0;
        if (data["type"].toString() == "Global Parameter")
            value = m_model->GlobalParameter(i);
        else
            value = m_model->LocalParameter(jndex, index);

        serie->append(QPointF(series.last().x(), value));
        serie->append(QPointF(series.first().x(), value));
        serie->setColor(color);
        view->addSeries(serie, i, color, name);
        view->setColor(i, color);
        if (data["type"].toString() != "Global Parameter")
            view->HideSeries(i);
    }
    for (const QString& string : text)
        m_text += string + "\n";

    m_text += "\n" + parameter_text;
    return view;
}

QWidget* ResultsWidget::ModelComparisonWidget()
{
    /*
    QJsonObject controller = m_data["controller"].toObject();
    ListChart* view = new ListChart;
    QVector<qreal> values(m_data.keys().size() - 1, 0);
    for (int a = 0; a < m_data.keys().size() - 1; ++a) {
        values[a] = m_data[QString::number(a)].toObject()["value"].toDouble();
        for (int b = a + 1; b < m_data.keys().size() - 1; ++b) {
            QColor color = ChartWrapper::ColorCode(m_model->Color(a + b - 1));
            int index = a + b - 1;

            QString name = m_data[QString::number(a)].toObject()["name"].toString() + " vs. " + m_data[QString::number(b)].toObject()["name"].toString();
            QtCharts::QScatterSeries* xy_series = new QtCharts::QScatterSeries;

            QList<qreal> x = ToolSet::String2DoubleList(m_data[QString::number(a)].toObject()["data"].toString());
            QList<qreal> y = ToolSet::String2DoubleList(m_data[QString::number(b)].toObject()["data"].toString());
            if (x.size() > 1e4)
                xy_series->setUseOpenGL(true);
            for (int j = 0; j < x.size(); ++j)
                xy_series->append(QPointF(x[j], y[j]));

            xy_series->setMarkerSize(5);
            xy_series->setName(name);
            view->addSeries(xy_series, index, color, name);
            xy_series->setColor(color);

            QList<QList<QPointF>> series;

            QJsonObject box = controller["box"].toObject();
            QList<QPointF> val1 = ToolSet::String2Points(box[QString::number(a)].toString());
            QList<QPointF> val2 = ToolSet::String2Points(box[QString::number(b)].toString());

            if (!val1.isEmpty() && !val2.isEmpty()) {
                QPointF range1 = val1[0];
                QPointF range2 = val2[0];
                QList<QPointF> serie;
                serie << QPointF(values[a], range2.x()) << QPointF(values[a], range2.y());
                series << serie;
                serie.clear();
                serie << QPointF(range1.x(), values[b]) << QPointF(range1.y(), values[b]);
                series << serie;
            }

            for (const QList<QPointF>& serie : qAsConst(series)) {
                LineSeries* xy_serie = new LineSeries;
                xy_serie->append(serie);
                xy_serie->setName(name);
                view->addSeries(xy_serie, index, color, name);
                xy_serie->setColor(color);
            }
        }
    }

    view->setXAxis("Parameter 1");
    view->setYAxis("Parameter 2");

    return view;*/

    ContourWidget* widget = new ContourWidget(m_models, m_model);
    QJsonObject controller = m_data["controller"].toObject();
    QVector<int> global = ToolSet::String2IntVec(controller["global_parameter"].toString());
    QVector<int> local = ToolSet::String2IntVec(controller["local_parameter"].toString());
    QVector<int> param = QVector<int>() << global << local;
    QVector<int> checked;

    for (int i = 0; i < param.size(); ++i) {
        if (param[i] == 0)
            emit widget->HideBox(i);
        else
            checked << i;
    }

    if (checked.size() >= 2) {
        widget->CheckParameterBox(checked.first());
        widget->CheckParameterBox(checked[1]);
    }

    return widget;
}

QWidget* ResultsWidget::GridSearchWidget()
{
    ListChart* view = new ListChart;
    view->setXAxis("constant");
    view->setYAxis("Sum of Squares");
    int series_int = 0;
    int old_index = 0;
    int rank = 0;
    for (int i = 0; i < m_data.count() - 1; ++i) {
        QJsonObject data = m_data[QString::number(i)].toObject();
        if (data.isEmpty())
            continue;
        rank = i;
        QString name = data["name"].toString();
        qreal x_0 = data["value"].toDouble();

        QtCharts::QLineSeries* xy_series = new QtCharts::QLineSeries;
        QList<qreal> x = ToolSet::String2DoubleList(data["data"].toObject()["x"].toString());
        QList<qreal> y = ToolSet::String2DoubleList(data["data"].toObject()["y"].toString());

        for (int j = 0; j < x.size(); ++j)
            xy_series->append(QPointF(x[j], y[j]));

        if (data["type"] == "Local Parameter") {
            if (!data.contains("index"))
                continue;

            int index = data["index"].toString().split("|")[1].toInt();
            rank = series_int + 2;
            if (index != old_index)
                series_int = 0;
            if (m_model.data()->SupportSeries())
                xy_series->setColor(m_wrapper->Series(series_int)->color());
            else
                xy_series->setColor(m_wrapper->ColorCode(i));
            /*connect(m_wrapper->Series(series_int), &QtCharts::QXYSeries::colorChanged, xy_series, &LineSeries::setColor);
            connect(m_wrapper->Series(series_int), &QtCharts::QXYSeries::colorChanged, this, [series_int, view]( const QColor &color ) { view->setColor(series_int, color); });*/
            name = QString::number(series_int + 1) + " " + name;
            series_int++;
            old_index = index;
        } else {
            xy_series->setColor(ChartWrapper::ColorCode(m_model->Color(i)));
            name = "0 " + name;
        }

        view->addSeries(xy_series, rank, xy_series->color(), name);

        LineSeries* current_constant = new LineSeries;
        *current_constant << QPointF(x_0, m_model->SumofSquares()) << QPointF(x_0, m_model->SumofSquares() * 1.1);
        current_constant->setColor(xy_series->color());
        current_constant->setName(m_model->GlobalParameterName(i));
        view->addSeries(current_constant, rank, xy_series->color(), name);
    }
    return view;
}

QWidget* ResultsWidget::SearchWidget()
{
    QWidget* widget = new QWidget;
    return widget;
}

void ResultsWidget::WriteConfidence(const QJsonObject& data)
{
    QString text;
    m_data = data;
    QJsonObject controller = m_data["controller"].toObject();
    for (int i = 0; i < m_data.count() - 1; ++i) {
        QJsonObject data = m_data[QString::number(i)].toObject();
        if (data.isEmpty())
            continue;
        text += Print::TextFromConfidence(data, m_model.data(), controller);
    }
    m_confidence_label->setText(text);
}

void ResultsWidget::Detailed()
{
    QTextEdit* text = new QTextEdit;
    text->setText("<html><pre> " + m_text + "</pre></html>");
    m_dialog->setWidget(text, tr("Details on Statistic Analysis"));
    m_dialog->show();
}
#include "resultswidget.moc"
