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

#include "src/core/toolset.h"

#include "src/ui/guitools/instance.h"

#include "src/ui/dialogs/chartconfig.h"
#include "src/ui/mainwindow/modelwidget.h"

#include <QtCharts/QChart>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QXYSeries>

#include <QtCore/QBuffer>
#include <QtCore/QDebug>
#include <QtCore/QMimeData>
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
    if (event->button() == Qt::RightButton)
        chart()->zoomReset();
    else
        QChartView::mouseReleaseEvent(event);
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

void ChartView::setUi()
{
    QGridLayout* layout = new QGridLayout;
    QMenu* menu = new QMenu;

    QAction* plotsettings = new QAction(this);
    plotsettings->setText(tr("Plot Settings"));
    connect(plotsettings, SIGNAL(triggered()), this, SLOT(PlotSettings()));
    menu->addAction(plotsettings);

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

    layout->addWidget(m_chart_private, 0, 0, 1, 5);
    layout->addWidget(m_config, 0, 4, Qt::AlignTop);
    setLayout(layout);

    connect(&m_chartconfigdialog, SIGNAL(ConfigChanged(ChartConfig)), this, SLOT(setChartConfig(ChartConfig)));
    connect(&m_chartconfigdialog, SIGNAL(ScaleAxis()), this, SLOT(forceformatAxis()));
    connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, this, &ChartView::ConfigurationChanged);
    ConfigurationChanged();
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

void ChartView::addSeries(QPointer<QtCharts::QAbstractSeries> series)
{
    if (!m_chart->series().contains(series) || !series) {
        QPointer<QtCharts::QXYSeries> serie = qobject_cast<QtCharts::QXYSeries*>(series);
        if (serie) {
            if (serie->points().size() > 5e3)
                serie->setUseOpenGL(true);
        }
        m_chart->addSeries(series);
        m_chart->createDefaultAxes();
    }
    connect(series, &QtCharts::QAbstractSeries::nameChanged, [this, series]() {
        if (series) {
            this->m_chart->legend()->markers(series).first()->setVisible(!series->name().isEmpty());
        }
    });
    m_chart->legend()->markers(series).first()->setVisible(true);
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

void ChartView::forceformatAxis()
{
    if (m_lock_scaling)
        return;
    m_pending = true;
    qreal x_min = 1000;
    qreal x_max = 0;
    qreal y_max = 0;
    qreal y_min = 1000;
    for (QtCharts::QAbstractSeries* series : m_chart->series()) {
        QPointer<QtCharts::QXYSeries> serie = qobject_cast<QtCharts::QXYSeries*>(series);
        if (!serie)
            continue;
        if (!serie->isVisible())
            continue;

        QVector<QPointF> points = serie->pointsVector();
        for (int i = 0; i < points.size(); ++i) {
            y_min = qMin(y_min, points[i].y());
            y_max = qMax(y_max, points[i].y());

            x_min = qMin(x_min, points[i].x());
            x_max = qMax(x_max, points[i].x());
        }
    }
    int x_mean = (x_max + x_min) / 2;
    int y_mean = (y_max + y_min) / 2;

    x_max = ToolSet::ceil(x_max - x_mean) + x_mean;
    y_max = ToolSet::ceil(y_max - y_mean) + y_mean;

    if (x_min)
        x_min = ToolSet::floor(x_min - x_mean) + x_mean;
    if (y_min)
        y_min = ToolSet::floor(y_min - y_mean) + y_mean;

    int x_ticks = ToolSet::scale(x_max - x_min) / int(ToolSet::scale(x_max - x_min) / 5) + 1;
    int y_ticks = 6; //scale(y_max-y_min)/int(scale(y_max-y_min)/ 5) + 1;

    QPointer<QtCharts::QValueAxis> y_axis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisY());
    if (!y_axis) {
        m_chart->createDefaultAxes();
        y_axis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisY());
    }
    y_axis->setMax(y_max);
    y_axis->setMin(y_min);
    y_axis->setTickCount(y_ticks);
    y_axis->setTitleText(m_y_axis);

    QPointer<QtCharts::QValueAxis> x_axis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisX());
    if (!x_axis) {
        m_chart->createDefaultAxes();
        x_axis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisY());
    }
    x_axis->setMax(x_max);
    x_axis->setMin(x_min);
    x_axis->setTickCount(x_ticks);

    x_axis->setTitleText(m_x_axis);
    m_pending = false;
    m_ymax = y_max;
    m_ymin = y_min;
    m_xmin = x_min;
    m_xmax = x_max;

    if (connected)
        m_chartconfigdialog.setConfig(getChartConfig());
}

void ChartView::PlotSettings()
{
    if (!connected)
        return;
    m_chartconfigdialog.setConfig(getChartConfig());
    m_chartconfigdialog.show();
}

void ChartView::setChartConfig(const ChartConfig& chartconfig)
{
    m_lock_scaling = chartconfig.m_lock_scaling;

    QPointer<QtCharts::QValueAxis> x_axis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisX());
    if (x_axis) {
        x_axis->setTitleText(chartconfig.x_axis);
        x_axis->setTickCount(chartconfig.x_step);
        x_axis->setMin(chartconfig.x_min);
        x_axis->setMax(chartconfig.x_max);
        x_axis->setTitleFont(chartconfig.m_label);
        x_axis->setLabelsFont(chartconfig.m_ticks);
    }
    QPointer<QtCharts::QValueAxis> y_axis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisY());
    if (y_axis) {
        y_axis->setTitleText(chartconfig.y_axis);
        y_axis->setTickCount(chartconfig.y_step);
        y_axis->setMin(chartconfig.y_min);
        y_axis->setMax(chartconfig.y_max);
        y_axis->setTitleFont(chartconfig.m_label);
        y_axis->setLabelsFont(chartconfig.m_ticks);
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
    } else {
        m_chart->legend()->setVisible(false);
    }
    setTitle(chartconfig.title);
    m_chart->setTitleFont(chartconfig.m_title);
}

void ChartView::setTitle(const QString& str)
{
    m_chart->setTitle(str);
}

ChartConfig ChartView::getChartConfig() const
{
    QPointer<QtCharts::QValueAxis> x_axis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisX());
    QPointer<QtCharts::QValueAxis> y_axis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisY());

    ChartConfig chartconfig;
    if (x_axis) {
        chartconfig.x_axis = x_axis->titleText();
        chartconfig.x_min = x_axis->min();
        chartconfig.x_max = x_axis->max();
        chartconfig.x_step = x_axis->tickCount();
        chartconfig.m_label = x_axis->titleFont();
        chartconfig.m_ticks = x_axis->labelsFont();
    }

    chartconfig.y_axis = y_axis->titleText();
    chartconfig.y_min = y_axis->min();
    chartconfig.y_max = y_axis->max();
    chartconfig.y_step = y_axis->tickCount();

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
    Waiter wait;
    int w = m_chart->rect().size().width();
    int h = m_chart->rect().size().height();
    int scale = 4;
    QImage image(QSize(scale * w, scale * h), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    m_chart->scene()->render(&painter, QRectF(0, 0, scale * w, scale * h), m_chart->rect());
    QPixmap pixmap = QPixmap::fromImage(image);
    QByteArray itemData;

    QFile file(str);
    file.open(QIODevice::WriteOnly);
    pixmap.save(&file, "PNG");
}

void ChartView::ConfigurationChanged()
{
    bool animation = qApp->instance()->property("chartanimation").toBool();
    if (animation)
        m_chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    else
        m_chart->setAnimationOptions(QtCharts::QChart::NoAnimation);

    m_chart->setTheme(QtCharts::QChart::ChartTheme(qApp->instance()->property("charttheme").toInt()));
}

#include "chartview.moc"
