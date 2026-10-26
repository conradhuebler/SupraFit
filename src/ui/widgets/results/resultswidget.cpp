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
#include <QtCore/QTimer>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtGui/QTextDocumentFragment>
#include <QtWidgets/QComboBox>
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

/*! \brief The result of a landscape sampling in one chart that shows what is chosen: the profile of the sum of squares
 * of all sampled constants, of a single one, or the 2D profile (height lines) of a pair. One line above the chart holds
 * the choice; with one sampled constant there is nothing to choose and no line. Claude Generated. */
QWidget* ResultsWidget::LandscapeWidget()
{
    const QJsonObject result = m_data["0"].toObject();
    const QJsonArray variables = result["variables"].toArray();
    const double fit = result["sse_min"].toDouble() > 0 ? result["sse_min"].toDouble() : 1.0;
    const double level = result["level"].toDouble() / fit;
    const double hi = result["contour_max"].toDouble(4); // upper end of the height lines, above the level by construction
    auto plain = [](const QString& html) { return QTextDocumentFragment::fromHtml(html).toPlainText(); };

    // the 2D profiles: `surfaces` of the pairs, or the single `surface` of a result stored by an earlier version
    QJsonArray surfaces = result["surfaces"].toArray();
    if (surfaces.isEmpty() && !result["surface"].toArray().isEmpty()) {
        QJsonObject old;
        old["x"] = 0;
        old["y"] = 1;
        old["bins"] = result["settings"].toObject()["bins"].toInt(60);
        old["points"] = result["surface"].toArray();
        surfaces << old;
    }

    QPointer<ListChart> view = new ListChart;
    view->setName(QStringLiteral("landscapechart"));
    view->setObjectName(QStringLiteral("landscapechart")); // for the tests and findChild
    connect(view, &ListChart::lastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    Instance::GlobalInstance()->MakeChartConnections(view);
    view->chart()->setZoomStrategy(ZoomStrategy::None); // the region around the fit is the point, a free zoom over the box a trap

    // all profiles, or the one of variable `only`
    auto drawProfiles = [=](int only) {
        double xmin = 0, xmax = 0;
        bool first = true;
        int index = 0;
        for (int i = 0; i < variables.size(); ++i) {
            if (only >= 0 && i != only)
                continue;
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
            view->addSeries(serie, index, color, v["name"].toString());
            view->setColor(index, color);
            ++index;
        }
        LineSeries* limit = new LineSeries;
        limit->setDashDotLine(true);
        const double margin = 0.1 * (xmax - xmin);
        limit->append(QPointF(xmin - margin, level));
        limit->append(QPointF(xmax + margin, level));
        limit->setName(tr("Limit of the confidence region"));
        limit->setColor(Qt::gray);
        view->addSeries(limit, index, Qt::gray, tr("Limit of the confidence region"));
        view->setColor(index, Qt::gray);

        view->setTitle(only >= 0 ? tr("Profile of %1").arg(plain(variables[only].toObject()["name"].toString()))
                                 : tr("Profiles for %1").arg(m_data["controller"].toObject()["title"].toString()));
        view->setXAxis(only >= 0 ? plain(variables[only].toObject()["name"].toString()) : tr("parameter value"));
        view->setYAxis(tr("SSE / SSE of the fit"));
    };

    // the 2D profile of a pair as height lines: the limit of the confidence region thick, ten lines above it
    auto drawSurface = [=](const QJsonObject& surface) -> QRectF {
        const int xi = surface["x"].toInt(), yi = surface["y"].toInt(), bins = qMax(2, surface["bins"].toInt(60));
        const QJsonObject vx = variables[xi].toObject(), vy = variables[yi].toObject();
        const double xlo = vx["box_lower"].toDouble(), xw = vx["box_upper"].toDouble() - xlo;
        const double ylo = vy["box_lower"].toDouble(), yw = vy["box_upper"].toDouble() - ylo;

        std::vector<double> sse(size_t(bins) * size_t(bins), INFINITY);
        for (const QJsonValue& v : surface["points"].toArray()) {
            const QJsonArray p = v.toArray();
            const int a = int((p[0].toDouble() - xlo) / xw * bins), b = int((p[1].toDouble() - ylo) / yw * bins);
            if (a >= 0 && a < bins && b >= 0 && b < bins)
                sse[size_t(a) * size_t(bins) + size_t(b)] = qMin(sse[size_t(a) * size_t(bins) + size_t(b)], p[2].toDouble() / fit);
        }
        auto at = [&](int a, int b) { return sse[size_t(a) * size_t(bins) + size_t(b)]; };
        auto centre = [&](int a, int b) { return QPointF(xlo + (a + 0.5) * xw / bins, ylo + (b + 0.5) * yw / bins); };

        auto hue = [](double t) { return QColor::fromHsv(int(220 * (1.0 - t)), 200, 220); }; // blue (low) to red (high)

        // A height line is a set of segments, one per square it crosses; segments that meet on a grid edge are joined into
        // polylines, otherwise a series would draw straight lines from the end of one segment to the start of the next.
        struct Segment {
            int id[2]; ///< grid edges the segment ends on (the same edge is the end of the neighbouring square's segment)
            QPointF end[2];
        };
        double xmin = vx["value"].toDouble(), xmax = xmin, ymin = vy["value"].toDouble(), ymax = ymin;
        int index = 0;
        for (int k = 0; k <= 10; ++k) {
            const double value = level + (hi - level) * k / 10.0;
            const QColor color = hue(0.08 * k);

            QList<Segment> segments;
            auto edgeId = [bins](int a, int b, bool vertical) { return (b * (bins + 1) + a) * 2 + (vertical ? 1 : 0); };
            for (int a = 0; a + 1 < bins; ++a)
                for (int b = 0; b + 1 < bins; ++b) {
                    const double v[4] = { at(a, b), at(a + 1, b), at(a + 1, b + 1), at(a, b + 1) };
                    // squares with a corner nobody visited carry no information
                    if (!std::isfinite(v[0]) || !std::isfinite(v[1]) || !std::isfinite(v[2]) || !std::isfinite(v[3]))
                        continue;
                    const QPointF corner[4] = { centre(a, b), centre(a + 1, b), centre(a + 1, b + 1), centre(a, b + 1) };
                    const int edge[4] = { edgeId(a, b, false), edgeId(a + 1, b, true), edgeId(a, b + 1, false), edgeId(a, b, true) };
                    Segment s;
                    int found = 0;
                    for (int e = 0; e < 4; ++e) {
                        const double v0 = v[e], v1 = v[(e + 1) % 4];
                        if ((v0 <= value) == (v1 <= value))
                            continue;
                        const double f = (value - v0) / (v1 - v0);
                        const QPointF p0 = corner[e], p1 = corner[(e + 1) % 4];
                        if (found < 2) {
                            s.id[found] = edge[e];
                            s.end[found] = QPointF(p0.x() + f * (p1.x() - p0.x()), p0.y() + f * (p1.y() - p0.y()));
                        }
                        ++found;
                    }
                    if (found == 2) // four crossings (a saddle) are left out: two segments would be ambiguous
                        segments << s;
                }
            if (segments.isEmpty())
                continue;

            QHash<int, QList<int>> meeting; // grid edge -> the segments ending on it
            for (int i = 0; i < segments.size(); ++i) {
                meeting[segments[i].id[0]] << i;
                meeting[segments[i].id[1]] << i;
            }
            QList<bool> used(segments.size(), false);
            for (int first = 0; first < segments.size(); ++first) {
                if (used[first])
                    continue;
                used[first] = true;
                QList<QPointF> chain{ segments[first].end[0], segments[first].end[1] };
                // grow at both ends: the end edge is shared with at most one unused segment
                for (int side = 0; side < 2; ++side) {
                    int edge = segments[first].id[side == 0 ? 1 : 0];
                    while (true) {
                        int next = -1;
                        for (int candidate : meeting.value(edge))
                            if (!used[candidate])
                                next = candidate;
                        if (next < 0)
                            break;
                        used[next] = true;
                        const int arrive = segments[next].id[0] == edge ? 0 : 1;
                        const QPointF far = segments[next].end[1 - arrive];
                        if (side == 0)
                            chain.append(far);
                        else
                            chain.prepend(far);
                        edge = segments[next].id[1 - arrive];
                    }
                    if (side == 0) // the second pass grows from the start of the chain
                        edge = segments[first].id[0];
                }

                LineSeries* line = new LineSeries;
                line->setColor(color);
                line->setLineWidth(k == 0 ? 4 : 2); // the limit of the confidence region stands out
                line->setShowInLegend(false);
                line->setName(QString::number(value, 'g', 3));
                line->append(chain);
                for (const QPointF& point : chain) {
                    xmin = qMin(xmin, point.x());
                    xmax = qMax(xmax, point.x());
                    ymin = qMin(ymin, point.y());
                    ymax = qMax(ymax, point.y());
                }
                view->addSeries(line, index, color, line->name()); // all lines of a level share one entry of the list
            }
            ++index;
        }

        QScatterSeries* marker = new QScatterSeries; // the fit
        marker->setColor(Qt::white);
        marker->setBorderColor(Qt::black);
        marker->setMarkerSize(10);
        marker->append(vx["value"].toDouble(), vy["value"].toDouble());
        view->addSeries(marker, index, Qt::white, tr("Fit"));

        // the axes follow the lines, not the box the walkers were allowed to explore (set by the caller once the chart has
        // finished scaling itself)
        const double padX = 0.05 * (xmax - xmin > 0 ? xmax - xmin : 1.0), padY = 0.05 * (ymax - ymin > 0 ? ymax - ymin : 1.0);
        const QRectF bounds(xmin - padX, ymin - padY, xmax - xmin + 2 * padX, ymax - ymin + 2 * padY);

        view->setTitle(tr("SSE / SSE of the fit over %1 and %2").arg(plain(vx["name"].toString()), plain(vy["name"].toString())));
        view->setXAxis(plain(vx["name"].toString()));
        view->setYAxis(plain(vy["name"].toString()));
        return bounds;
    };

    // the choice: nothing to choose with one constant and no 2D profile
    struct Entry {
        QString text;
        int kind; ///< 0 all profiles, 1 one profile, 2 surface
        int index;
    };
    QList<Entry> entries;
    entries << Entry{ variables.size() > 1 ? tr("Profiles") : tr("Profile"), 0, -1 };
    if (variables.size() > 1)
        for (int i = 0; i < variables.size(); ++i)
            entries << Entry{ tr("Profile: %1").arg(plain(variables[i].toObject()["name"].toString())), 1, i };
    for (int s = 0; s < surfaces.size(); ++s)
        entries << Entry{ tr("Surface: %1 × %2").arg(plain(variables[surfaces[s].toObject()["x"].toInt()].toObject()["name"].toString()),
                              plain(variables[surfaces[s].toObject()["y"].toInt()].toObject()["name"].toString())),
            2, s };

    auto show = [=](const Entry& entry) {
        view->clear();
        if (entry.kind == 2) {
            const QRectF bounds = drawSurface(surfaces[entry.index].toObject());
            // after the chart has scaled itself for the new series, which it does again when it is shown
            QTimer::singleShot(0, view, [view, bounds]() {
                view->chart()->setXRange(bounds.left(), bounds.right());
                view->chart()->setYRange(bounds.top(), bounds.bottom());
            });
        } else
            drawProfiles(entry.kind == 1 ? entry.index : -1);
    };
    show(entries.first());

    if (entries.size() == 1)
        return view;

    QWidget* host = new QWidget;
    QVBoxLayout* box = new QVBoxLayout;
    box->setContentsMargins(0, 0, 0, 0);
    QComboBox* choice = new QComboBox;
    choice->setObjectName(QStringLiteral("landscapeview"));
    choice->setToolTip(tr("What the chart shows: the profile of the sum of squares of all constants or of one, or the sum of squares over a pair of "
                          "constants as height lines."));
    for (const Entry& e : entries)
        choice->addItem(e.text);
    connect(choice, &QComboBox::currentIndexChanged, this, [=](int row) {
        if (row >= 0 && row < entries.size())
            show(entries[row]);
    });
    QHBoxLayout* row = new QHBoxLayout;
    row->addWidget(new QLabel(tr("Show:")));
    row->addWidget(choice, 1);
    box->addLayout(row);
    box->addWidget(view, 1);
    host->setLayout(box);
    return host;
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
