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
#include "src/capabilities/landscapesearch.h"
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

#include "src/core/phasetiming.h"

#include "resultswidget.h"

ResultsWidget::ResultsWidget(const QJsonObject& data, QSharedPointer<AbstractModel> model, ChartWrapper* wrapper)
{
    m_data = data;
    m_model = model;

    QJsonObject controller = m_data["controller"].toObject();
    const QJsonObject raw = controller["raw"].toObject();
    for (int i = 0; i < raw.size(); ++i)
        m_models << raw[QString::number(i)].toObject();
    PhaseTiming::Mark(QStringLiteral("  results: copy %1 raw MC models / %2 parameter blocks")
                          .arg(m_models.size())
                          .arg(qMax(0, m_data.count() - 1)));
    m_wrapper = wrapper;
    m_dialog = new ModalDialog;
    m_text = QString("");
    setUi();
    PhaseTiming::Mark(QStringLiteral("  results: build charts + widget"));
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

    case SupraFit::Method::LandscapeSampling:
        m_widget = LandscapeWidget();
        setObjectName("Landscape Sampling for " + m_model.toStrongRef().data()->Name());
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

/*! \brief Profile of the sum of squares along every sampled constant, with the limit of the confidence region.
 * The intervals are the stretches of each curve below the dashed line. The sums of squares are drawn relative to
 * the one of the fit, so that the axis reads the same for every data set. Claude Generated. */
/*! \brief Surface of the 2D profile of the first two sampled constants (SSE relative to the fit), with contour lines.
 * The profile comes from the points the walkers recorded; without it the widget only says so. Claude Generated. */
QWidget* ResultsWidget::LandscapeSurface()
{
    const QJsonObject result = m_data["0"].toObject();
    const QJsonArray surface = result["surface"].toArray();
    QWidget* host = new QWidget;
    QVBoxLayout* box = new QVBoxLayout;
    host->setLayout(box);

    const QJsonArray variables = result["variables"].toArray();
    if (surface.isEmpty() || variables.size() < 2) {
        QLabel* none = new QLabel(tr("No surface: sampling with \"2D surface\" records the points of the walk."));
        none->setWordWrap(true);
        box->addWidget(none);
        return host;
    }

    QPointer<ListChart> view = new ListChart;
    view->setName(QStringLiteral("landscapesurface"));
    view->setObjectName(QStringLiteral("landscapesurface")); // for the tests and findChild
    connect(view, &ListChart::lastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    Instance::GlobalInstance()->MakeChartConnections(view);
    // the interesting region is around the fit; zooming over a mostly empty box is more a trap than a feature
    view->chart()->setZoomStrategy(ZoomStrategy::None);

    const double fit = result["sse_min"].toDouble() > 0 ? result["sse_min"].toDouble() : 1.0;
    const double level = result["level"].toDouble() / fit;
    const double hi = result["contour_max"].toDouble(4); // contour range of the surface, above the level by construction

    // contour lines: a line of the profile that reaches the level, drawn in steps of 1/40 of the range, coloured from cool to warm
    struct Level {
        double value;
        QColor color;
    };
    // lines from the centre outwards: the level of the region and one step above it (to see it is closed), then the
    // wall's tail in coarser steps
    QList<Level> levels;
    int seriesIndex = 0;
    auto hue = [](double t) { return QColor::fromHsv(int(220 * (1.0 - t)), 200, 220); };
    // one line every 1/10 of the distance level -> contour_max: close around the minimum, coarser outside
    levels << Level{ level, hue(0.0) };
    for (int k = 1; k <= 10; ++k)
        levels << Level{ level + (hi - level) * k / 10.0, hue(0.08 * k) };


    // collect the bins per (x, y), the lowest SSE wins
    const int bins = int(std::sqrt(double(surface.size())));
    std::vector<std::vector<double>> sse(size_t(bins), std::vector<double>(size_t(bins), INFINITY));
    double xmin = 1e30, xmax = -1e30, ymin = 1e30, ymax = -1e30;
    for (const QJsonValue& v : surface) {
        const QJsonArray p = v.toArray();
        const double x = p[0].toDouble(), y = p[1].toDouble();
        xmin = qMin(xmin, x);
        xmax = qMax(xmax, x);
        ymin = qMin(ymin, y);
        ymax = qMax(ymax, y);
        // the grid index follows from the position between the bounds of the variables
        const QJsonObject vx = variables[0].toObject(), vy = variables[1].toObject();
        const int a = int((x - vx["box_lower"].toDouble()) / (vx["box_upper"].toDouble() - vx["box_lower"].toDouble()) * bins);
        const int b = int((y - vy["box_lower"].toDouble()) / (vy["box_upper"].toDouble() - vy["box_lower"].toDouble()) * bins);
        if (a >= 0 && a < bins && b >= 0 && b < bins)
            sse[size_t(a)][size_t(b)] = qMin(sse[size_t(a)][size_t(b)], p[2].toDouble() / fit);
    }

    const double dx = (xmax - xmin) / std::max(1, bins - 1), dy = (ymax - ymin) / std::max(1, bins - 1);
    for (const Level& l : levels) {
        LineSeries* line = new LineSeries;
        line->setColor(l.color);
        line->setLineWidth(2);
        line->setName(QStringLiteral("%1").arg(l.value, 0, 'g', 3));
        // march the squares: every edge that crosses the level contributes the midpoint of the edge
        for (int a = 0; a < bins - 1; ++a)
            for (int b = 0; b < bins - 1; ++b) {
                const double v[4] = { sse[size_t(a)][size_t(b)], sse[size_t(a + 1)][size_t(b)], sse[size_t(a + 1)][size_t(b + 1)], sse[size_t(a)][size_t(b + 1)] };
                const bool below[4] = { v[0] <= l.value, v[1] <= l.value, v[2] <= l.value, v[3] <= l.value };
                const QPointF p[4] = { { xmin + a * dx, ymin + b * dy }, { xmin + (a + 1) * dx, ymin + b * dy }, { xmin + (a + 1) * dx, ymin + (b + 1) * dy }, { xmin + a * dx, ymin + (b + 1) * dy } };
                QList<QPointF> crossings;
                for (int edge = 0; edge < 4; ++edge) {
                    const QPointF p0 = p[edge], p1 = p[(edge + 1) % 4];
                    const double v0 = v[edge], v1 = v[(edge + 1) % 4];
                    if ((v0 <= l.value) == (v1 <= l.value))
                        continue;
                    const double t = (l.value - v0) / (v1 - v0);
                    crossings << QPointF(p0.x() + t * (p1.x() - p0.x()), p0.y() + t * (p1.y() - p0.y()));
                }
                if (crossings.size() == 2) {
                    line->append(crossings[0]);
                    line->append(crossings[1]);
                }
            }
        if (!line->points().isEmpty()) {
            line->setShowInLegend(false);
            const int index = seriesIndex++;
            // the lines of the region the data can limit, without the tail of the wall
            if (l.value > level * 1.02 && l.value < level * 0.5 * 1.001)
                continue;
            view->addSeries(line, index, l.color, line->name());
            if (std::abs(l.value - level) < 0.02 * level)
                line->setLineWidth(4); // the limit of the confidence region stands out
        }
    }

    // the fit itself, as a cross
    QScatterSeries* marker = new QScatterSeries;
    marker->setColor(Qt::black);
    marker->setMarkerSize(10);
    marker->append(variables[0].toObject()["value"].toDouble(), variables[1].toObject()["value"].toDouble());
    view->addSeries(marker, seriesIndex++, Qt::black, tr("Fit"));

    view->setTitle(tr("Surface of %1 and %2 (%3, relative to the fit)").arg(variables[0].toObject()["name"].toString(), variables[1].toObject()["name"].toString(), m_model.toStrongRef().data()->Name()));
    view->setXAxis(variables[0].toObject()["name"].toString());
    view->setYAxis(variables[1].toObject()["name"].toString());
    box->addWidget(view);
    box->addWidget(new QLabel(tr("Contour lines of SSE/SSE of the fit; the limit of the confidence region is drawn as a thicker line. The intervals of the list above are the profile of this surface.")));
    return host;
}

QWidget* ResultsWidget::LandscapeWidget()
{
    const QJsonObject result = m_data["0"].toObject();
    QPointer<ListChart> view = new ListChart;
    view->setName(QStringLiteral("landscapechart"));
    view->setObjectName(QStringLiteral("landscapechart")); // for the tests and findChild
    connect(view, &ListChart::lastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    Instance::GlobalInstance()->MakeChartConnections(view);
    view->chart()->setZoomStrategy(ZoomStrategy::None); // one parameter per axis: a window, not a free move

    double xmin = 0, xmax = 0;
    bool first = true;
    const double fit = result["sse_min"].toDouble() > 0 ? result["sse_min"].toDouble() : 1.0;
    const QJsonArray variables = result["variables"].toArray();
    for (int i = 0; i < variables.size(); ++i) {
        const QJsonObject v = variables[i].toObject();
        QList<QPointF> points;
        for (const QJsonValue& p : v["profile"].toArray())
            points << QPointF(p.toArray()[0].toDouble(), p.toArray()[1].toDouble() / fit);
        std::sort(points.begin(), points.end(), [](const QPointF& a, const QPointF& b) { return a.x() < b.x(); });
        if (points.isEmpty())
            continue;
        xmin = first ? points.first().x() : qMin(xmin, points.first().x());
        xmax = first ? points.last().x() : qMax(xmax, points.last().x());
        first = false;

        const QColor color = ChartWrapper::ColorCode(m_model.toStrongRef().data()->Color(v["index"].toInt()));
        LineSeries* serie = new LineSeries;
        serie->append(points);
        serie->setName(v["name"].toString());
        serie->setColor(color);
        serie->setLineWidth(3);
        view->addSeries(serie, i, color, v["name"].toString());
        view->setColor(i, color);
    }

    LineSeries* level = new LineSeries;
    level->setDashDotLine(true);
    const double margin = 0.1 * (xmax - xmin);
    level->append(QPointF(xmin - margin, result["level"].toDouble() / fit));
    level->append(QPointF(xmax + margin, result["level"].toDouble() / fit));
    level->setName(tr("Limit of the confidence region"));
    const QColor grey = Qt::gray;
    level->setColor(grey);
    view->addSeries(level, variables.size(), grey, tr("Limit of the confidence region"));
    view->setColor(variables.size(), grey);

    view->setTitle(tr("Landscape Sampling for %1").arg(m_data["controller"].toObject()["title"].toString()));
    view->setXAxis(tr("parameter value"));
    view->setYAxis(tr("SSE / SSE of the fit"));
    if (result["surface"].toArray().size() > 10 && result["variables"].toArray().size() >= 2) {
        QSplitter* splitter = new QSplitter(Qt::Vertical);
        splitter->addWidget(view);
        splitter->addWidget(LandscapeSurface());
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 1);
        return splitter;
    }
    return view;
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
    connect(view, &ListChart::lastDirChanged, this, [](const QString& str) {
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
            view->hideSeries(i);
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
    connect(view, &ListChart::lastDirChanged, this, [](const QString& str) {
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
    } else if (AccessCI(controller, "Method").toInt() == SupraFit::Method::LandscapeSampling) {
        text += LandscapeSearch::Summary(data["0"].toObject());
    } else {
        text += Print::TextFromStatistic(controller["raw"].toObject());
        PhaseTiming::Mark(QStringLiteral("    confidence: TextFromStatistic(raw) -> %1 chars").arg(text.size()));
        text += m_model.toStrongRef().data()->AnalyseStatistic(m_data, false);
        PhaseTiming::Mark(QStringLiteral("    confidence: AnalyseStatistic -> %1 chars total").arg(text.size()));
    }
    m_confidence_label->setText(text);
    PhaseTiming::Mark(QStringLiteral("    confidence: QLabel::setText (rich text)"));
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
