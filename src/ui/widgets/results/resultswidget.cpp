/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <charts.h>

#include "src/capabilities/abstractsearchclass.h"
#include "src/core/toolset.h"

#include "src/core/models/AbstractModel.h"

#include "src/ui/instance.h"

#include "src/ui/dialogs/modaldialog.h"
#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/guitools.h"

#include "src/ui/widgets/results/mcresultswidget.h"
#include "src/ui/widgets/results/scatterwidget.h"
#include "src/ui/widgets/results/searchresultwidget.h"
#include "src/ui/widgets/statisticwidget.h"
#include "src/ui/widgets/textwidget.h"

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
        m_models << controller["raw"].toObject()[QString::number(i)].toObject();
    }
    m_wrapper = wrapper;
    m_dialog = new ModalDialog;
    m_text = QString("");
    setUi();
    resize(600, 400);
}

ResultsWidget::~ResultsWidget()
{
    m_model.clear();
    delete m_dialog;
}

void ResultsWidget::setUi()
{
    QSplitter* splitter = new QSplitter(Qt::Vertical);

    QGridLayout* layout = new QGridLayout;

    m_confidence_label = new QLabel();
    m_confidence_label->setTextFormat(Qt::RichText);
    m_confidence_label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    switch (AccessCI(m_data["controller"].toObject(), "Method").toInt()) {
    case SupraFit::Method::MonteCarlo:
        m_widget = MonteCarloWidget();
        setObjectName("Monte Carlo Simulation for " + m_model.toStrongRef().data()->Name());
        break;

    case SupraFit::Method::ModelComparison:
        m_widget = ModelComparisonWidget();
        setObjectName("Model Comparison Confidence for " + m_model.toStrongRef().data()->Name());
        break;

    case SupraFit::Method::WeakenedGridSearch:
        m_widget = GridSearchWidget();
        setObjectName("Weakend Grid Search Confidence for " + m_model.toStrongRef().data()->Name());
        break;

    case SupraFit::Method::Reduction:
        m_widget = ReductionWidget();
        setObjectName("Reduction Analysis for " + m_model.toStrongRef().data()->Name());
        break;

    case SupraFit::Method::CrossValidation:
        m_widget = MonteCarloWidget();
        setObjectName("Cross Validation Estimation for " + m_model.toStrongRef().data()->Name());
        break;

    case SupraFit::Method::GlobalSearch:
        m_widget = SearchWidget();
        setObjectName("Global Search for " + m_model.toStrongRef().data()->Name());
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
    view->setName("reductionchart");
    connect(view, &ListChart::LastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    Instance::GlobalInstance()->MakeChartConnections(view);

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
#pragma message("actually the other way around, FIXME!")
        serie->setShowInLegend(false);
        serie->setLineWidth(4);
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
        if (data["type"].toString() == "Global Parameter") {
            color = ChartWrapper::ColorCode(m_model.toStrongRef().data()->Color(i));
            serie->setColor(color);
        } else {
            if (data.contains("index")) {
                QStringList lindex = data["index"].toString().split("|");
                index = lindex[1].toInt();
                jndex = lindex[0].toInt();
                if (m_model.toStrongRef().data()->SupportSeries()) {
                    if (index < m_wrapper->SeriesSize()) {
                        serie->setColor(m_wrapper->Series(index)->color());
                        color = m_wrapper->Series(index)->color();
                        connect(m_wrapper->Series(index), &QXYSeries::colorChanged, serie, &LineSeries::setColor);
                    }
                }
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
            value = m_model.toStrongRef().data()->GlobalParameter(i);
        else
            value = m_model.toStrongRef().data()->LocalParameter(jndex, index);

        serie->append(QPointF(series.last().x(), value));
        serie->append(QPointF(series.first().x(), value));
        serie->setColor(color);

#pragma message("actually the other way around, FIXME!")
        serie->setShowInLegend(true);

        view->addSeries(serie, i, color, name);
        view->setColor(i, color);
        if (data["type"].toString() != "Global Parameter")
            view->HideSeries(i);
    }
    for (const QString& string : text)
        m_text += string + "\n";

    m_text += "\n" + parameter_text;
    view->setTitle(QString("Reduction Analysis for %1").arg(m_data["controller"].toObject()["title"].toString()));
    view->setXAxis(m_data["controller"].toObject()["xlabel"].toString());
    view->setYAxis("parameter value");
    return view;
}

QWidget* ResultsWidget::ModelComparisonWidget()
{
    ScatterWidget* widget = new ScatterWidget;
    widget->setConverged(false);
    widget->setValid(false);
    widget->setData(m_models, m_model);
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
    QTabWidget* tabwidget = new QTabWidget;

    ListChart* view = new ListChart;
    view->setAutoScaleStrategy(AutoScaleStrategy::QtNiceNumbers);
    connect(view, &ListChart::LastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    Instance::GlobalInstance()->MakeChartConnections(view);

    tabwidget->addTab(view, "Grid Search Result");

    /* If we have intermediate models available, lets plot them as scatter plot */
    if (m_models.size() != 0) {
        ScatterWidget* scatterwidget = new ScatterWidget;
        scatterwidget->setData(m_models, m_model);
        tabwidget->addTab(scatterwidget, "Scatter Plot");
    }

    for (int i = 0; i < m_data.count() - 1; ++i) {
        QJsonObject data = m_data[QString::number(i)].toObject();
        if (data.isEmpty())
            continue;
        QString name = data["name"].toString();
        qreal x_0 = data["value"].toDouble();

        LineSeries* xy_series = new LineSeries;
        QList<qreal> x = ToolSet::String2DoubleList(data["data"].toObject()["x"].toString());
        QList<qreal> y = ToolSet::String2DoubleList(data["data"].toObject()["y"].toString());

        for (int j = 0; j < x.size(); ++j)
            xy_series->append(QPointF(x[j], y[j]));

        if (data["type"] == "Local Parameter") {
            if (!data.contains("index"))
                continue;

            int index = data["index"].toString().split("|")[1].toInt();
            if (m_model.toStrongRef().data()->SupportSeries()) {
                if (index < m_wrapper->SeriesSize()) {
                    xy_series->setColor(m_wrapper->Series(index)->color());
                    connect(m_wrapper->Series(index), &QXYSeries::colorChanged, xy_series, &LineSeries::setColor);
                }
            }

        } else {
            xy_series->setColor(ChartWrapper::ColorCode(m_model.toStrongRef().data()->Color(i)));
        }

        view->addSeries(xy_series, i, xy_series->color(), name, true);

        LineSeries* current_constant = new LineSeries;
        *current_constant << QPointF(x_0, m_model.toStrongRef().data()->SSE()) << QPointF(x_0, m_model.toStrongRef().data()->SSE() * 1.1);
        current_constant->setDashDotLine(true);
        current_constant->setColor(xy_series->color());
        current_constant->setName(name);
        view->addSeries(current_constant, i, xy_series->color(), name, true);
    }

    view->setName("gridchart");
    view->setXAxis("parameter value");
    view->setYAxis(m_data["controller"].toObject()["ylabel"].toString());

    view->setTitle(tr("Grid Search for %1").arg(m_model.toStrongRef().data()->Name()));

    return tabwidget;
}

QWidget* ResultsWidget::SearchWidget()
{
    SearchResultWidget* table = new SearchResultWidget(m_data, m_model, this);
    connect(table, &SearchResultWidget::LoadModel, this, &ResultsWidget::LoadModel);
    connect(table, &SearchResultWidget::AddModel, this, &ResultsWidget::AddModel);
    return table;
}

void ResultsWidget::WriteConfidence(const QJsonObject& data)
{
    QString text;
    m_data = data;
    QJsonObject controller = m_data["controller"].toObject();
    if (AccessCI(controller, "Method").toInt() == SupraFit::Method::GlobalSearch) {
        text += Print::TextFromStatistic(data);
    } else {
        text += Print::TextFromStatistic(controller["raw"].toObject());
        text += m_model.toStrongRef().data()->AnalyseStatistic(m_data, false);
    }
    m_confidence_label->setText(text);
    m_model.toStrongRef().data()->UpdateStatistic(m_data);
}

void ResultsWidget::Detailed()
{
    TextWidget* text = new TextWidget;
    text->setText("<html><pre> " + m_text + "</br> " + m_model.toStrongRef().data()->AnalyseStatistic(m_data, true) + "</pre></html>");

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(text);

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Details on Statistic Analysis"));
    dialog.setLayout(layout);
    dialog.resize(1024, 800);
    dialog.exec();
}
#include "resultswidget.moc"
