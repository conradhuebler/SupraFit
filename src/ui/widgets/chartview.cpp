/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global_config.h"

#include "src/core/toolset.h"

#include "src/ui/guitools/instance.h"
#include "src/ui/guitools/peakcallout.h"

#include "src/ui/dialogs/chartconfig.h"
#include "src/ui/mainwindow/modelwidget.h"

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QXYSeries>

#include <QtCore/QBuffer>
#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>

#include <QtGui/QDrag>

#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>

#include <cmath>
#include <iostream>
#include <unistd.h>

#include "chartview.h"

void ChartViewPrivate::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton || event->buttons() == Qt::MiddleButton) {
        QImage image(scene()->sceneRect().size().toSize(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        scene()->render(&painter);
        QPixmap pixmap = QPixmap::fromImage(image);
        QByteArray itemData;
        QBuffer outputBuffer(&itemData);

        outputBuffer.open(QIODevice::WriteOnly);
        pixmap.toImage().save(&outputBuffer, "PNG");

        QMimeData* mimeData = new QMimeData;
        mimeData->setData("image/png", itemData);

        QDrag* drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->setPixmap(pixmap);
        drag->setHotSpot(event->pos());

        drag->exec(Qt::CopyAction);
    } else if (event->button() == Qt::RightButton)
        event->ignore();
    else
        QChartView::mousePressEvent(event);
}

void ChartViewPrivate::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        chart()->zoomReset();
        emit UnLockZoom();
    } else {
        QChartView::mouseReleaseEvent(event);
        emit LockZoom();
    }
}

void ChartViewPrivate::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("image/png")) {
        if (event->source() == this) {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void ChartViewPrivate::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("image/png")) {
        if (event->source() == this) {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void ChartViewPrivate::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        chart()->zoomIn();
        break;
    case Qt::Key_Minus:
        chart()->zoomOut();
        break;
    case Qt::Key_Left:
        chart()->scroll(-10, 0);
        break;
    case Qt::Key_Right:
        chart()->scroll(10, 0);
        break;
    case Qt::Key_Up:
        chart()->scroll(0, 10);
        break;
    case Qt::Key_Down:
        chart()->scroll(0, -10);
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}

ChartView::ChartView(QtCharts::QChart* chart, bool latex_supported)
    : m_chart_private(new ChartViewPrivate(chart, this))
    , m_chart(chart)
    , has_legend(false)
    , connected(false)
    , m_x_axis(QString())
    , m_y_axis(QString())
    , m_pending(false)
    , m_lock_scaling(false)
    , m_latex_supported(latex_supported)
    , m_ymax(0)
{
    m_chart->legend()->setVisible(false);
    m_chart->legend()->setAlignment(Qt::AlignRight);
    setUi();
}

ChartView::ChartView()
    : has_legend(false)
    , connected(false)
    , m_x_axis(QString())
    , m_y_axis(QString())
    , m_pending(false)
    , m_latex_supported(false)
    , m_lock_scaling(false)
{
    m_chart = new QtCharts::QChart();
    m_chart_private = new ChartViewPrivate(m_chart, this);

    m_chart->legend()->setVisible(false);
    m_chart->legend()->setAlignment(Qt::AlignRight);
    setUi();
}

ChartView::~ChartView()
{
    //WriteSettings(m_last_config);

    qDeleteAll(m_peak_anno);
}

void ChartView::setUi()
{
    m_name = "chart";
    mCentralLayout = new QGridLayout;
    QMenu* menu = new QMenu(this);

    QAction* plotsettings = new QAction(this);
    plotsettings->setText(tr("Plot Settings"));
    connect(plotsettings, SIGNAL(triggered()), this, SLOT(PlotSettings()));
    menu->addAction(plotsettings);

    m_lock_action = new QAction(this);
    m_lock_action->setText(tr("Lock Scaling"));
    m_lock_action->setCheckable(true);
    connect(m_lock_action, &QAction::triggered, this, [this]() {
        m_lock_scaling = m_lock_action->isChecked();
    });
    menu->addAction(m_lock_action);

    QAction* scaleAction = new QAction(this);
    scaleAction->setText(tr("Rescale Axis"));
    connect(scaleAction, SIGNAL(triggered()), this, SLOT(forceformatAxis()));
    menu->addAction(scaleAction);

    QAction* exportpng = new QAction(this);
    exportpng->setText(tr("Export Diagram (PNG)"));
    connect(exportpng, SIGNAL(triggered()), this, SLOT(ExportPNG()));
    menu->addAction(exportpng);

    /*QAction *printplot = new QAction(this);
    printplot->setText(tr("Print Diagram"));
    connect(printplot, SIGNAL(triggered()), this, SLOT(PlotSettings()));
    menu->addAction(printplot);*/

    if (m_latex_supported) {
        QAction* exportlatex = new QAction(this);
        exportlatex->setText(tr("Export to Latex (tikz)"));
        connect(exportlatex, SIGNAL(triggered()), this, SLOT(ExportLatex()));
        menu->addAction(exportlatex);
    }
    /*
    QAction *exportgnuplot = new QAction(this);
    exportgnuplot->setText(tr("Export to Latex (tikz)"));
    connect(exportgnuplot, SIGNAL(triggered()), this, SLOT(ExportGnuplot()));
    menu->addAction(exportgnuplot);
  */
    m_config = new QPushButton(tr("Tools"));
    m_config->setFlat(true);
    m_config->setIcon(QIcon::fromTheme("applications-system"));
    m_config->setMaximumWidth(100);
    m_config->setStyleSheet("QPushButton {background-color: #A3C1DA; color: black;}");
    m_config->setMenu(menu);

    mCentralLayout->addWidget(m_chart_private, 0, 0, 1, 5);
    mCentralLayout->addWidget(m_config, 0, 4, Qt::AlignTop);

    mCentralHolder = new QWidget;
    mCentralHolder->setLayout(mCentralLayout);
    setWidget(mCentralHolder);

    m_chartconfigdialog = new ChartConfigDialog(this);

    connect(m_chartconfigdialog, &ChartConfigDialog::ConfigChanged, this, [this](const ChartConfig& config) {
        this->setChartConfig(config);
        this->WriteSettings(config);
        emit Instance::GlobalInstance()->ConfigurationChanged(m_name);
    });
    connect(m_chartconfigdialog, SIGNAL(ScaleAxis()), this, SLOT(forceformatAxis()));
    connect(m_chartconfigdialog, SIGNAL(ResetFontConfig()), this, SLOT(ResetFontConfig()));

    connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, this, &ChartView::ConfigurationChanged);
    ConfigurationChanged();

    connect(m_chart_private, &ChartViewPrivate::LockZoom, this, [this]() {
        this->m_lock_scaling = true;
        this->m_lock_action->setChecked(true);
    });

    connect(m_chart_private, &ChartViewPrivate::UnLockZoom, this, [this]() {
        this->m_lock_scaling = false;
        this->m_lock_action->setChecked(false);
    });

    m_x_size = (qApp->instance()->property("xSize").toInt());
    m_y_size = (qApp->instance()->property("ySize").toInt());
    m_scaling = (qApp->instance()->property("chartScaling").toInt());
    m_lineWidth = (qApp->instance()->property("chartScaling").toDouble());
    m_markerSize = (qApp->instance()->property("markerSize").toDouble());
}

QtCharts::QLineSeries* ChartView::addLinearSeries(qreal m, qreal n, qreal min, qreal max)
{
    qreal y_min = m * min + n;
    qreal y_max = m * max + n;
    QtCharts::QLineSeries* series = new QtCharts::QLineSeries(this);
    series->append(min, y_min);
    series->append(max, y_max);
    addSeries(series);
    return series;
}

void ChartView::addSeries(QPointer<QtCharts::QAbstractSeries> series, bool callout)
{
    if (!m_chart->series().contains(series) || !series) {
        QPointer<QtCharts::QXYSeries> serie = qobject_cast<QtCharts::QXYSeries*>(series);
        if (serie) {
            if (serie->points().size() > 5e3)
                serie->setUseOpenGL(true);
            if (callout) {
                qreal x = 0;
                for (const QPointF& point : serie->points())
                    x += point.x();
                x /= double(serie->points().size());
                QPointF point(x, 1.5);

                QPointer<PeakCallOut> annotation = new PeakCallOut(m_chart);
                annotation->setSeries(series);
                annotation->setText(series->name(), point);
                annotation->setAnchor(point);
                annotation->setZValue(11);
                //annotation->updateGeometry();
                annotation->show();
                connect(series, &QtCharts::QAbstractSeries::visibleChanged, series, [series, annotation]() {
                    annotation->setVisible(series->isVisible());
                });
                connect(serie, &QtCharts::QXYSeries::colorChanged, serie, [serie, annotation]() {
                    annotation->setColor(serie->color());
                });
                annotation->setColor(serie->color());
                m_peak_anno.append(annotation);
            }
        }
        m_chart->addSeries(series);
        if (!m_hasAxis) {
            m_chart->createDefaultAxes();
            m_XAxis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisX());
            m_YAxis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisY());
            m_hasAxis = true;
            ReadSettings();
        } else {
            series->attachAxis(m_XAxis);
            series->attachAxis(m_YAxis);
        }
        m_series << series;
    }
    connect(series, &QtCharts::QAbstractSeries::nameChanged, series, [this, series]() {
        if (series) {
            //qDebug() << series->name();
#warning this can be compressed due to logic gatters
            bool show = series->name().isEmpty() || series->name().isNull() || series->name().simplified() == QString(" ");
            this->m_chart->legend()->markers(series).first()->setVisible(!show);
        }
    });
    connect(series, &QtCharts::QAbstractSeries::visibleChanged, series, [this, series]() {
        if (series) {
            //qDebug() << series->name();
#warning this can be compressed due to logic gatters
            bool show = series->name().isEmpty() || series->name().isNull() || series->name().simplified() == QString(" ");
            if (series->isVisible())
                this->m_chart->legend()->markers(series).first()->setVisible(!show);
        }
    });
    //qDebug() << series->name();
    m_chart->legend()->markers(series).first()->setVisible(!(series->name().isEmpty() || series->name().isNull() || series->name().simplified() == QString(" ")));
    connect(series, SIGNAL(visibleChanged()), this, SLOT(forceformatAxis()));
    if (!connected)
        if (connect(this, SIGNAL(AxisChanged()), this, SLOT(forceformatAxis())))
            connected = true;
    forceformatAxis();
}

void ChartView::ClearChart()
{
    m_chart->removeAllSeries();
    emit ChartCleared();
}

void ChartView::formatAxis()
{
    if (m_pending || m_chart->series().isEmpty())
        return;
    forceformatAxis();
}

void ChartView::ScaleAxis(QPointer<QtCharts::QValueAxis> axis, qreal& min, qreal& max)
{
    int mean = (max + min) / 2;

    if (1 < mean && mean < 10) {
        max = std::ceil(max);
        min = std::floor(min);
    } else {
        max = ToolSet::ceil(max - mean) + mean;
        if (min && !(0 < min && min < 1))
            min = ToolSet::floor(min - mean) + mean;
        else
            min = 0;
    }

    int ticks = ToolSet::scale(max - min) / int(ToolSet::scale(max - min) / 5) + 1;
    //ticks = 2*(max-min)-1;
    if (ticks < 10) {
        axis->setTickCount(ticks);
        axis->setRange(min, max);
    } else
        axis->applyNiceNumbers();
}

void ChartView::forceformatAxis()
{
    if (m_lock_scaling || m_chart->series().size() == 0)
        return;
    m_pending = true;

    qreal x_min = 0;
    qreal x_max = 0;
    qreal y_max = 0;
    qreal y_min = 0;
    int start = 0;
    for (QtCharts::QAbstractSeries* series : m_chart->series()) {
        QPointer<QtCharts::QXYSeries> serie = qobject_cast<QtCharts::QXYSeries*>(series);
        if (!serie)
            continue;
        if (!serie->isVisible())
            continue;

        QVector<QPointF> points = serie->pointsVector();
        if (start == 0 && points.size()) {
            y_min = points.first().y();
            y_max = points.first().y();

            x_min = points.first().x();
            x_max = points.first().x();
            start = 1;
        }
        for (int i = 0; i < points.size(); ++i) {
            y_min = qMin(y_min, points[i].y());
            y_max = qMax(y_max, points[i].y());

            x_min = qMin(x_min, points[i].x());
            x_max = qMax(x_max, points[i].x());
        }
    }

    ScaleAxis(m_XAxis, x_min, x_max);
    ScaleAxis(m_YAxis, y_min, y_max);

    m_XAxis->setTitleText(m_x_axis);
    m_YAxis->setTitleText(m_y_axis);

    m_pending = false;
    m_ymax = y_max;
    m_ymin = y_min;
    m_xmin = x_min;
    m_xmax = x_max;

    if (connected)
        m_chartconfigdialog->setConfig(getChartConfig());
}

void ChartView::PlotSettings()
{
    if (!connected)
        return;
    m_chartconfigdialog->setConfig(getChartConfig());
    m_chartconfigdialog->show();
    m_chartconfigdialog->raise();
    m_chartconfigdialog->activateWindow();
}

void ChartView::setChartConfig(const ChartConfig& chartconfig)
{
    m_last_config = chartconfig;

    m_lock_scaling = chartconfig.m_lock_scaling;
    m_lock_action->setChecked(m_lock_scaling);

    m_x_size = chartconfig.x_size;
    m_y_size = chartconfig.y_size;
    m_scaling = chartconfig.scaling;

    m_markerSize = chartconfig.markerSize;
    m_lineWidth = chartconfig.lineWidth;

    QPointer<QtCharts::QValueAxis> m_XAxis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisX());
    if (m_XAxis) {
        m_XAxis->setTitleText(chartconfig.x_axis);
        m_XAxis->setTickCount(chartconfig.x_step);
        m_XAxis->setMin(chartconfig.x_min);
        m_XAxis->setMax(chartconfig.x_max);
        m_XAxis->setTitleFont(chartconfig.m_label);
        m_XAxis->setLabelsFont(chartconfig.m_ticks);
        m_XAxis->setVisible(chartconfig.showAxis);
    }
    QPointer<QtCharts::QValueAxis> m_YAxis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisY());
    if (m_YAxis) {
        m_YAxis->setTitleText(chartconfig.y_axis);
        m_YAxis->setTickCount(chartconfig.y_step);
        m_YAxis->setMin(chartconfig.y_min);
        m_YAxis->setMax(chartconfig.y_max);
        m_YAxis->setTitleFont(chartconfig.m_label);
        m_YAxis->setLabelsFont(chartconfig.m_ticks);
        m_YAxis->setVisible(chartconfig.showAxis);
    }

    if (chartconfig.m_legend) {
        m_chart->legend()->setVisible(true);
        if (chartconfig.align == Qt::AlignTop
            || chartconfig.align == Qt::AlignBottom
            || chartconfig.align == Qt::AlignLeft
            || chartconfig.align == Qt::AlignRight)
            m_chart->legend()->setAlignment(chartconfig.align);
        else
            m_chart->legend()->setAlignment(Qt::AlignRight);
        m_chart->legend()->setFont(chartconfig.m_keys);
        for (PeakCallOut* call : m_peak_anno)
            call->setFont(chartconfig.m_keys);
    } else {
        m_chart->legend()->setVisible(false);
    }
    setTitle(chartconfig.title);
    m_chart->setTitleFont(chartconfig.m_title);

    int Theme = chartconfig.Theme;
    if (Theme < 8)
        m_chart->setTheme(static_cast<QtCharts::QChart::ChartTheme>(Theme));
    else {
        for (int i = 0; i < m_series.size(); ++i) {
            if (!m_series[i])
                continue;

            if (qobject_cast<QtCharts::QXYSeries*>(m_series[i])) {
                QtCharts::QXYSeries* series = qobject_cast<QtCharts::QXYSeries*>(m_series[i]);
                series->setColor(QColor("black"));
            } else if (qobject_cast<QtCharts::QAreaSeries*>(m_series[i])) {
                QtCharts::QAreaSeries* series = qobject_cast<QtCharts::QAreaSeries*>(m_series[i]);
                QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
                gradient.setColorAt(0.0, QColor("darkGray"));
                gradient.setColorAt(1.0, QColor("lightGray"));
                gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
                series->setBrush(gradient);
                series->setOpacity(0.4);
                QPen pen(QColor("darkGray"));
                pen.setWidth(3);
                series->setPen(pen);
            }
            QBrush brush;
            brush.setColor(Qt::transparent);
            m_chart->setBackgroundBrush(brush);
            //m_chart->setStyleSheet("background-color: transparent;");
        }
        /* QFont font = chartconfig.m_title;
        font.setPointSize(12);
        m_chart->setTitleFont(font);
        m_YAxis->setLabelsFont(font);
        m_XAxis->setLabelsFont(font);
        m_XAxis->setTitleFont(font);
        m_YAxis->setTitleFont(font);

        m_chart->legend()->setFont(font);
        for (PeakCallOut* call : m_peak_anno)
            call->setFont(font);
        */
    }
    for (QPointer<PeakCallOut> call : m_peak_anno) {
        if (!call)
            continue;
        call->setVisible(chartconfig.m_annotation);
        call->setFont(chartconfig.m_keys);
    }
}

void ChartView::WriteSettings(const ChartConfig& chartconfig)
{
    QSettings _settings;
    _settings.beginGroup(m_name);
    _settings.setValue("labels", chartconfig.m_label);
    _settings.setValue("ticks", chartconfig.m_ticks);
    _settings.setValue("title", chartconfig.m_title);
    _settings.setValue("legend", chartconfig.m_keys);
    _settings.endGroup();
}

ChartConfig ChartView::ReadSettings()
{
    ChartConfig chartconfig = m_last_config;
    QSettings _settings;
    if (!_settings.contains(m_name)) {
#ifdef noto_font
        QFont font("Noto Sans SemBd", 11);
        chartconfig.m_label = font;
        chartconfig.m_ticks = font;
        chartconfig.m_keys = font;

        font.setPixelSize(font.pixelSize() + 1);
        chartconfig.m_title = font;
#endif
    } else {
        _settings.beginGroup(m_name);
        chartconfig.m_label = _settings.value("labels").value<QFont>();
        chartconfig.m_ticks = _settings.value("ticks").value<QFont>();
        chartconfig.m_title = _settings.value("title").value<QFont>();
        chartconfig.m_keys = _settings.value("legend").value<QFont>();
        _settings.endGroup();
    }
    m_last_config = chartconfig;
    setChartConfig(chartconfig);
    m_chartconfigdialog->setConfig(chartconfig);

    return chartconfig;
}

void ChartView::setTitle(const QString& str)
{
    m_chart->setTitle(str);
}

ChartConfig ChartView::getChartConfig() const
{
    ChartConfig chartconfig;
    if (m_hasAxis) {

        chartconfig.x_axis = m_XAxis->titleText();
        chartconfig.x_min = m_XAxis->min();
        chartconfig.x_max = m_XAxis->max();
        chartconfig.x_step = m_XAxis->tickCount();
        chartconfig.m_label = m_XAxis->titleFont();
        chartconfig.m_ticks = m_XAxis->labelsFont();
        chartconfig.y_axis = m_YAxis->titleText();
        chartconfig.y_min = m_YAxis->min();
        chartconfig.y_max = m_YAxis->max();
        chartconfig.y_step = m_YAxis->tickCount();
    }
    chartconfig.m_legend = m_chart->legend()->isVisible();
    chartconfig.m_lock_scaling = m_lock_scaling;

    chartconfig.m_keys = m_chart->legend()->font();
    chartconfig.align = m_chart->legend()->alignment();
    chartconfig.title = m_chart->title();
    chartconfig.m_title = m_chart->titleFont();

    return chartconfig;
}

void ChartView::PrintPlot()
{
}

QString ChartView::Color2RGB(const QColor& color) const
{
    QString result;
    result = QString::number(color.toRgb().red()) + "," + QString::number(color.toRgb().green()) + "," + QString::number(color.toRgb().blue());
    return result;
}

PgfPlotConfig ChartView::getScatterTable() const
{
    PgfPlotConfig config;
    QStringList table;
    int i = 0;
    for (QtCharts::QAbstractSeries* series : m_chart->series()) {
        if (!qobject_cast<QtCharts::QScatterSeries*>(series))
            continue;
        QtCharts::QScatterSeries* serie = qobject_cast<QtCharts::QScatterSeries*>(series);
        if (serie->isVisible()) {
            QVector<QPointF> points = serie->pointsVector();
            for (int i = 0; i < points.size(); ++i) {
                if (i == table.size())
                    table << QString::number(points[i].x());
                table[i] += " " + QString::number(points[i].y());
            }
            QString definecolor;
            definecolor = "\\definecolor{scatter" + QString::number(i) + "}{RGB}{" + Color2RGB(serie->color()) + "}";
            config.colordefinition += definecolor + "\n";

            QString defineplot;
            defineplot = "\\addplot+[color = scatter" + QString::number(i) + ",only marks,mark = *,mark options={scale=0.75, fill=scatter" + QString::number(i) + "}]" + QString("table[x=0, y = %1] from \\scatter;").arg(QString::number(i + 1)) + "\n";
            defineplot += "\\addlegendentry{" + serie->name() + "};";
            config.plots += defineplot + "\n";
            ++i;
        }
    }
    config.table = table;
    return config;
}

PgfPlotConfig ChartView::getLineTable() const
{
    PgfPlotConfig config;
    QStringList table;
    int i = 0;
    for (QtCharts::QAbstractSeries* series : m_chart->series()) {
        if (!qobject_cast<QtCharts::QLineSeries*>(series))
            continue;
        QtCharts::QLineSeries* serie = qobject_cast<QtCharts::QLineSeries*>(series);
        if (serie->isVisible()) {
            QVector<QPointF> points = serie->pointsVector();
            for (int i = 0; i < points.size(); ++i) {
                if (i == table.size())
                    table << QString::number(points[i].x());
                table[i] += " " + QString::number(points[i].y());
            }
            QString definecolor;
            definecolor = "\\definecolor{line" + QString::number(i) + "}{RGB}{" + Color2RGB(serie->color()) + "}";
            config.colordefinition += definecolor + "\n";

            QString defineplot;
            defineplot = "\\addplot+[color = line" + QString::number(i) + QString(",no marks, solid] table[x=0, y = %1] from \\lines;").arg(QString::number(i + 1)) + "\n";
            defineplot += "\\addlegendentry{" + serie->name() + "};";
            config.plots += defineplot + "\n";
            ++i;
        }
    }
    config.table = table;
    return config;
}

void ChartView::WriteTable(const QString& str)
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Save File"),
        getDir());
    if (dir.isEmpty() || dir.isNull())
        return;
    setLastDir(dir);

    QFile data(dir + "/" + str + "_scatter.dat");

    PgfPlotConfig scatter_table = getScatterTable();
    if (data.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&data);
        for (const QString& str : qAsConst(scatter_table.table))
            out << str << "\n";
    }

    data.setFileName(dir + "/" + str + "_line.dat");
    PgfPlotConfig line_table = getLineTable();
    if (data.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&data);
        for (const QString& str : qAsConst(line_table.table))
            out << str << "\n";
    }

    data.setFileName(dir + "/" + str + ".tex");
    if (data.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&data);
        out << "\\documentclass{standalone}"
            << "\n";
        out << "\\usepackage[latin1]{inputenc}"
            << "\n";
        out << "\\usepackage{tikz}"
            << "\n";
        out << "\\usepackage{xcolor}"
            << "\n";
        out << "\\usepackage{pgfplots} % LaTeX"
            << "\n";
        out << "\\renewcommand\\familydefault{\\sfdefault}"
            << "\n";
        out << "\\usepackage{pgfplotstable} "
            << "\n";
        out << scatter_table.colordefinition;
        out << line_table.colordefinition;
        out << "\\begin{document}"
            << "\n";
        out << "\\pagestyle{empty}"
            << "\n";
        out << "\\pgfplotstableread{" + str + "_scatter.dat}{\\scatter}"
            << "\n";
        out << "\\pgfplotstableread{" + str + "_line.dat}{\\lines}"
            << "\n";
        out << "\\begin{tikzpicture}"
            << "\n";
        out << "\\tikzstyle{every node}=[font=\\footnotesize]"
            << "\n";
        out << "\\begin{axis}[title={" + m_chart->title() + "}, legend style={at={(1.08,0.9))},anchor=north,legend cell align=left},x tick label style={at={(1,10)},anchor=north},xlabel={\\begin{footnotesize}" + m_x_axis + "\\end{footnotesize}}, xlabel near ticks, ylabel={\\begin{footnotesize}" + m_y_axis + "\\end{footnotesize}},ylabel near ticks]"
            << "\n";
        out << scatter_table.plots;
        out << line_table.plots;
        out << "\\end{axis}"
            << "\n";
        out << "\\end{tikzpicture}"
            << "\n";
        out << "\\end{document}"
            << "\n";
    }
}

void ChartView::ExportLatex()
{
    bool ok;
    QString str = QInputDialog::getText(this, tr("Select output file"),
        tr("Please specify the base name of the files.\nA tex file, file for scatter and line table data will be created."), QLineEdit::Normal,
        qApp->instance()->property("projectname").toString(), &ok);

    if (ok)
        WriteTable(str);
}
/*
void ChartView::ExportGnuplot()
{
    WriteTable("table");
}
*/
void ChartView::ExportPNG()
{
    const QString str = QFileDialog::getSaveFileName(this, tr("Save File"),
        getDir(),
        tr("Images (*.png)"));
    if (str.isEmpty() || str.isNull())
        return;
    setLastDir(str);

    QtCharts::QChart::AnimationOptions animation = m_chart->animationOptions();

    m_chart->setAnimationOptions(QtCharts::QChart::NoAnimation);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // first cache the original size, within SupraFit
    QSize widgetSize = mCentralHolder->size();

    // and resize as set in settings
    m_chart->resize(m_x_size, m_y_size);
    mCentralHolder->resize(m_x_size, m_y_size);

    for (PeakCallOut* call : m_peak_anno) {
        call->update();
    }
    m_chart->scene()->update();
    QApplication::processEvents();

    Waiter wait;
    int w = m_chart->rect().size().width();
    int h = m_chart->rect().size().height();
    // scaling is important for good resolution
    QImage image(QSize(m_scaling * w, m_scaling * h), QImage::Format_ARGB32);

    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    bool xGrid = m_XAxis->isGridLineVisible();
    bool yGrid = m_YAxis->isGridLineVisible();

    // hide grid lines
    if (qApp->instance()->property("noGrid").toBool() == true) {
        m_XAxis->setGridLineVisible(false);
        m_YAxis->setGridLineVisible(false);
    }

    QPen xPen = m_XAxis->linePen();
    QPen yPen = m_YAxis->linePen();

    QPen pen = m_XAxis->linePen();
    pen.setColor(Qt::black);
    pen.setWidth(2);

    // make axis stronger
    if (qApp->instance()->property("empAxis").toBool() == true) {
        m_XAxis->setLinePen(pen);
        m_YAxis->setLinePen(pen);
    }
    QBrush brush_backup = m_chart->backgroundBrush();
    QBrush brush;
    brush.setColor(Qt::transparent);

    // remove background of chart
    if (qApp->instance()->property("transparentChart").toBool() == true)
        m_chart->setBackgroundBrush(brush);

    // cache all individual series size and border colors and remove border colors and resize series

    QList<QColor> colors;
    QList<int> size, width;
    QList<bool> openGl;
    for (QtCharts::QAbstractSeries* serie : m_chart->series()) {

        if (qobject_cast<QtCharts::QScatterSeries*>(serie)) {

            colors << qobject_cast<QtCharts::QScatterSeries*>(serie)->borderColor();
            qobject_cast<QtCharts::QScatterSeries*>(serie)->setBorderColor(Qt::transparent);
            size << qobject_cast<QtCharts::QScatterSeries*>(serie)->markerSize();

            if (qobject_cast<QtCharts::QScatterSeries*>(serie)->markerSize() > m_markerSize)
                qobject_cast<QtCharts::QScatterSeries*>(serie)->setMarkerSize(m_markerSize);

        } else if (qobject_cast<LineSeries*>(serie)) {

            width << qobject_cast<LineSeries*>(serie)->LineWidth();
            qobject_cast<LineSeries*>(serie)->setSize(m_lineWidth / 10.0);
        }
        openGl << serie->useOpenGL();
        serie->setUseOpenGL(false);
    }
    // do the painting!!
    m_chart->scene()->render(&painter, QRectF(0, 0, m_scaling * w, m_scaling * h), m_chart->rect());

    /*
     * copyied from here:
     *
     * https://stackoverflow.com/questions/3720947/does-qt-have-a-way-to-find-bounding-box-of-an-image
     *
     */

    auto border = [](const QImage& tmp) -> QImage {
        int l = tmp.width(), r = 0, t = tmp.height(), b = 0;
        for (int y = 0; y < tmp.height(); ++y) {
            QRgb* row = (QRgb*)tmp.scanLine(y);
            bool rowFilled = false;
            for (int x = 0; x < tmp.width(); ++x) {
                if (qAlpha(row[x])) {
                    rowFilled = true;
                    r = std::max(r, x);
                    if (l > x) {
                        l = x;
                        x = r; // shortcut to only search for new right bound from here
                    }
                }
            }
            if (rowFilled) {
                t = std::min(t, y);
                b = y;
            }
        }
        return tmp.copy(l, t, r, b);
    };

    QPixmap pixmap;

    // remove transparent border of resulting image

    if (qApp->instance()->property("cropedChart").toBool() == true) {
#warning stupid things
        /* dont unterstand that in particular, but it works somehow, and it is not to slow */

        QImage mirrored = border(border(image.mirrored(true, true)).mirrored(true, true).mirrored(true, true)).mirrored(true, true);
        pixmap = QPixmap::fromImage(border(mirrored));
    } else
        pixmap = QPixmap::fromImage(image);

    // restore background brush
    m_chart->setBackgroundBrush(brush_backup);

    // restore series colors and size
    for (QtCharts::QAbstractSeries* serie : m_chart->series()) {
        if (qobject_cast<QtCharts::QScatterSeries*>(serie)) {
            qobject_cast<QtCharts::QScatterSeries*>(serie)->setBorderColor(colors.takeFirst());
            qobject_cast<QtCharts::QScatterSeries*>(serie)->setMarkerSize(size.takeFirst());
        } else if (qobject_cast<LineSeries*>(serie)) {
            qobject_cast<LineSeries*>(serie)->setSize(width.takeFirst() / 10.0);
        }
        serie->setUseOpenGL(openGl.takeFirst());
    }

    // bring back the grids
    m_XAxis->setGridLineVisible(xGrid);
    m_YAxis->setGridLineVisible(yGrid);

    // bring back the old and weak axis
    m_XAxis->setLinePen(xPen);
    m_YAxis->setLinePen(yPen);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // restore old size
    mCentralHolder->resize(widgetSize);

    for (PeakCallOut* call : m_peak_anno)
        call->Update();

    // and nothing ever happens -> Lemon Tree
    m_chart->setAnimationOptions(animation);

    QFile file(str);
    file.open(QIODevice::WriteOnly);
    pixmap.save(&file, "PNG");
}

void ChartView::ConfigurationChanged(const QString& str)
{
    if (str == m_name)
        ReadSettings();
    else {
        bool animation = qApp->instance()->property("chartanimation").toBool();
        if (animation)
            m_chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
        else
            m_chart->setAnimationOptions(QtCharts::QChart::NoAnimation);

        m_chart->setTheme(QtCharts::QChart::ChartTheme(qApp->instance()->property("charttheme").toInt()));
    }
}

void ChartView::resizeEvent(QResizeEvent* event)
{
    event->accept();
    mCentralHolder->resize(0.99 * size());
    /*
    if(event->size().width() > event->size().height()){
        QWidget::resize(event->size().height(),event->size().height());
    }else{
        QWidget::resize(event->size().width(),event->size().width());
    }*/
}

void ChartView::ResetFontConfig()
{
    ChartConfig chartconfig = m_last_config;
    QFont font;
#ifdef noto_font
    font = QFont("Noto Sans SemBd");
    font.setPointSize(11);
#else
    font.setPointSize(11);
#endif

    chartconfig.m_label = font;
    chartconfig.m_ticks = font;
    chartconfig.m_keys = font;
    font.setPointSize(12);

    chartconfig.m_title = font;
    m_last_config = chartconfig;
    setChartConfig(chartconfig);
    m_chartconfigdialog->setConfig(chartconfig);
}

#include "chartview.moc"
