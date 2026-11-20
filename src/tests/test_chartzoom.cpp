/*
 * SupraFit - offscreen test of zoom and range selection with real mouse events on the chart of the trace editor
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * Mouse events (press, move, release) go to the viewport of the chart, as a real mouse sends them: a left
 * drag zooms by the zoom strategy of the chart (horizontal, vertical, rectangular) also while the range
 * selection of the right button is horizontal; a right drag selects an x range without changing the zoom;
 * the middle button resets the zoom. Every case runs on an ascending and a descending (ppm) x axis, and
 * the axis direction of the editor (Auto, Reversed, Normal) is checked against the pixel positions of
 * known x values. Claude Generated.
 */

#include <cmath>

#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtGui/QCursor>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtTest/QSignalSpy>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

#include <charts.h>

#include <QtCore/QDir>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include "src/core/nmrhandler.h"
#include "src/ui/dialogs/nmrimport.h"
#include "src/ui/guitools/appearance.h"
#include "src/ui/widgets/traceeditorwidget.h"

namespace {
/// 1H-like spectrum from 0 to 10 ppm: a reference signal of height 100 at 0.5 ppm and small signals (height 1) at 3, 5 and 7 ppm
XYTrace spectrum(bool descending, int n = 1001)
{
    Eigen::VectorXd x(n), y(n);
    for (int i = 0; i < n; ++i) {
        const double t = descending ? 10.0 - 10.0 * i / double(n - 1) : 10.0 * i / double(n - 1);
        x(i) = t;
        y(i) = 100 * std::exp(-std::pow((t - 0.5) / 0.03, 2));
        for (double c : { 3.0, 5.0, 7.0 })
            y(i) += std::exp(-std::pow((t - c) / 0.05, 2));
    }
    return XYTrace(x, y, "spectrum");
}

QValueAxis* axis(TraceEditorWidget& editor, Qt::Orientation orientation)
{
    const QList<QAbstractAxis*> axes = editor.chart()->chart()->axes(orientation);
    return axes.isEmpty() ? nullptr : qobject_cast<QValueAxis*>(axes.first());
}

/// viewport pixel of the value (x, y)
QPoint pixel(TraceEditorWidget& editor, double x, double y)
{
    QChart* chart = editor.chart()->chart();
    const QPointF scene = chart->mapToScene(chart->mapToPosition(QPointF(x, y)));
    return editor.chart()->privateView()->mapFromScene(scene);
}

/// sends a mouse event to the viewport (as the window system does) and moves the cursor there, too
void send(TraceEditorWidget& editor, QEvent::Type type, const QPoint& at, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    QWidget* viewport = editor.chart()->privateView()->viewport();
    const QPoint global = viewport->mapToGlobal(at);
    if (qEnvironmentVariableIsSet("SUPRAFIT_CHARTZOOM_SET_CURSOR")) // the chart reads the events, not the cursor
        QCursor::setPos(global);
    QMouseEvent event(type, QPointF(at), QPointF(global), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(viewport, &event);
}

/// press at the value a, three moves, release at b
void drag(TraceEditorWidget& editor, Qt::MouseButton button, QPointF a, QPointF b)
{
    const QPoint p = pixel(editor, a.x(), a.y()), q = pixel(editor, b.x(), b.y());
    send(editor, QEvent::MouseMove, p, Qt::NoButton, Qt::NoButton);
    send(editor, QEvent::MouseButtonPress, p, button, button);
    for (int i = 1; i <= 3; ++i)
        send(editor, QEvent::MouseMove, p + (q - p) * i / 3, Qt::NoButton, button);
    send(editor, QEvent::MouseButtonRelease, q, button, Qt::NoButton);
    QCoreApplication::processEvents();
}

/// sends a wheel event (delta in eighths of a degree, 120 = one notch) to the viewport (Claude Generated, Viewer 2/3)
void wheel(TraceEditorWidget& editor, const QPoint& at, int delta, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    QWidget* viewport = editor.chart()->privateView()->viewport();
    QWheelEvent event(QPointF(at), QPointF(viewport->mapToGlobal(at)), QPoint(), QPoint(0, delta), Qt::NoButton, modifiers, Qt::NoScrollPhase, false);
    QApplication::sendEvent(viewport, &event);
}

/// value under a viewport pixel, mapped as the chart maps it
QPointF valueAt(TraceEditorWidget& editor, const QPoint& at)
{
    QChartView* view = editor.chart()->privateView();
    return view->chart()->mapToValue(view->chart()->mapFromScene(view->mapToScene(at)));
}

/// the line series of trace k (the editor adds the traces first)
QLineSeries* line(TraceEditorWidget& editor, int k)
{
    return qobject_cast<QLineSeries*>(editor.chart()->chart()->series().value(k));
}

struct Span {
    double xmin, xmax, ymin, ymax;
};

Span span(TraceEditorWidget& editor)
{
    QValueAxis* x = axis(editor, Qt::Horizontal);
    QValueAxis* y = axis(editor, Qt::Vertical);
    return { x->min(), x->max(), y->min(), y->max() };
}
} // namespace

using Direction = TraceEditorWidget::AxisDirection;
Q_DECLARE_METATYPE(TraceEditorWidget::AxisDirection)

class TestChartZoom : public QObject {
    Q_OBJECT

    /// editor with the spectrum and a peak picking step, shown at a fixed size
    void setup(TraceEditorWidget& editor, bool descending, Direction direction = Direction::Auto)
    {
        editor.setAxisDirection(direction);
        TraceSet set;
        set.add(spectrum(descending));
        editor.setTraces(set);
        editor.addOperation("PickPeaks");
        QVERIFY(editor.setOperationParameters(0, QJsonObject{ { "Threshold", 0.005 } })); // the small signals, too
        editor.resize(1100, 640);
        editor.show();
        QVERIFY(QTest::qWaitForWindowExposed(&editor));
        QCoreApplication::processEvents();
        editor.refreshMarkers();
        QVERIFY(axis(editor, Qt::Horizontal));
        QVERIFY(axis(editor, Qt::Vertical));
    }

    /// three copies of the spectrum (fixed ids), peak picking, shown at a fixed size (Claude Generated, Viewer 2)
    void setupStack(TraceEditorWidget& editor, bool descending, Direction direction)
    {
        editor.setAxisDirection(direction);
        TraceSet set;
        for (int k = 0; k < 3; ++k)
            set.add(spectrum(descending), QString("trace%1").arg(k));
        editor.setTraces(set);
        editor.addOperation("PickPeaks");
        QVERIFY(editor.setOperationParameters(0, QJsonObject{ { "Threshold", 0.005 } }));
        editor.resize(1100, 640);
        editor.show();
        QVERIFY(QTest::qWaitForWindowExposed(&editor));
        QCoreApplication::processEvents();
        editor.refreshMarkers();
    }

    /// tolerance of a value given by a drag: two pixels of the axis
    static double pixelTolerance(double from, double to, double width) { return 2.5 * std::abs(to - from) / width; }

private slots:
    void initTestCase()
    {
        qApp->setProperty("StoreRawData", true);
    }

    void zoomByStrategy_data()
    {
        QTest::addColumn<bool>("descending");
        QTest::addColumn<Direction>("direction");
        QTest::addColumn<int>("strategy");
        const QList<QPair<bool, Direction>> cases = { { false, Direction::Auto }, { true, Direction::Auto }, { false, Direction::Reversed }, { true, Direction::Normal } };
        const QStringList names = { "ascending auto", "descending auto", "ascending reversed", "descending normal" };
        for (int c = 0; c < cases.size(); ++c) {
            const QString order = names[c];
            QTest::newRow(qPrintable(QString("vertical %1").arg(order))) << cases[c].first << cases[c].second << int(ZoomStrategy::Vertical);
            QTest::newRow(qPrintable(QString("horizontal %1").arg(order))) << cases[c].first << cases[c].second << int(ZoomStrategy::Horizontal);
            QTest::newRow(qPrintable(QString("rectangular %1").arg(order))) << cases[c].first << cases[c].second << int(ZoomStrategy::Rectangular);
        }
    }

    /*! \brief (a)-(c): a left drag zooms as the zoom strategy says, although the selection strategy of the
     * editor is horizontal; the series (peak markers, range limits) stay. */
    void zoomByStrategy()
    {
        QFETCH(bool, descending);
        QFETCH(Direction, direction);
        QFETCH(int, strategy);
        TraceEditorWidget editor;
        setup(editor, descending, direction);
        const bool reversed = direction == Direction::Auto ? descending : direction == Direction::Reversed;
        QCOMPARE(editor.chart()->currentSelectStrategy(), SelectStrategy::Horizontal);
        QCOMPARE(axis(editor, Qt::Horizontal)->isReverse(), reversed);
        editor.chart()->setZoomStrategy(ZoomStrategy(strategy));
        const Span before = span(editor);
        QVERIFY2(before.ymax >= 100, qPrintable(QString("y max %1").arg(before.ymax)));
        const int series = editor.chart()->chart()->series().size();
        QSignalSpy range(&editor, &TraceEditorWidget::rangeChanged);
        QSignalSpy zoomed(editor.chart(), &ChartView::zoomChanged);

        drag(editor, Qt::LeftButton, QPointF(2, 2), QPointF(6, 0.2));
        const Span after = span(editor);
        const QString text = QString("x %1..%2 -> %3..%4, y %5..%6 -> %7..%8").arg(before.xmin).arg(before.xmax).arg(after.xmin).arg(after.xmax).arg(before.ymin).arg(before.ymax).arg(after.ymin).arg(after.ymax);
        const double width = editor.chart()->chart()->plotArea().width(), height = editor.chart()->chart()->plotArea().height();
        const double tx = pixelTolerance(before.xmin, before.xmax, width), ty = pixelTolerance(before.ymin, before.ymax, height);
        const bool zoomX = strategy != int(ZoomStrategy::Vertical);
        const bool zoomY = strategy != int(ZoomStrategy::Horizontal);
        if (zoomX) {
            QVERIFY2(std::abs(after.xmin - 2) < tx && std::abs(after.xmax - 6) < tx, qPrintable(text));
        } else {
            QVERIFY2(std::abs(after.xmin - before.xmin) < tx && std::abs(after.xmax - before.xmax) < tx, qPrintable(text));
        }
        if (zoomY) {
            QVERIFY2(std::abs(after.ymin - 0.2) < ty && std::abs(after.ymax - 2) < ty, qPrintable(text));
        } else {
            QVERIFY2(std::abs(after.ymin - before.ymin) < ty && std::abs(after.ymax - before.ymax) < ty, qPrintable(text));
        }
        QCOMPARE(axis(editor, Qt::Horizontal)->isReverse(), reversed);
        QCOMPARE(range.count(), 0);
        QVERIFY(zoomed.count() >= 1);
        QCOMPARE(editor.chart()->chart()->series().size(), series);
        QVERIFY(editor.errorText().isEmpty());

        // (e) the middle button resets the zoom
        const QPoint centre = pixel(editor, 4, 1);
        send(editor, QEvent::MouseButtonPress, centre, Qt::MiddleButton, Qt::MiddleButton);
        send(editor, QEvent::MouseButtonRelease, centre, Qt::MiddleButton, Qt::NoButton);
        const Span reset = span(editor);
        QVERIFY2(std::abs(reset.xmin - before.xmin) < 1e-9 && std::abs(reset.xmax - before.xmax) < 1e-9, qPrintable(QString("x after reset %1..%2").arg(reset.xmin).arg(reset.xmax)));
        QVERIFY2(std::abs(reset.ymin - before.ymin) < 1e-9 && std::abs(reset.ymax - before.ymax) < 1e-9, qPrintable(QString("y after reset %1..%2").arg(reset.ymin).arg(reset.ymax)));
    }

    void rightDragSelectsRange_data()
    {
        QTest::addColumn<bool>("descending");
        QTest::addColumn<Direction>("direction");
        QTest::addColumn<int>("zoom");
        const QList<QPair<bool, Direction>> cases = { { false, Direction::Auto }, { true, Direction::Auto }, { false, Direction::Reversed }, { true, Direction::Normal } };
        const QStringList names = { "ascending auto", "descending auto", "ascending reversed", "descending normal" };
        for (int c = 0; c < cases.size(); ++c) {
            QTest::newRow(qPrintable(QString("zoom rectangular %1").arg(names[c]))) << cases[c].first << cases[c].second << int(ZoomStrategy::Rectangular);
            QTest::newRow(qPrintable(QString("zoom vertical %1").arg(names[c]))) << cases[c].first << cases[c].second << int(ZoomStrategy::Vertical);
        }
    }

    /*! \brief (d): a right drag gives the x range sorted, in either drag direction, and leaves the zoom. */
    void rightDragSelectsRange()
    {
        QFETCH(bool, descending);
        QFETCH(Direction, direction);
        QFETCH(int, zoom);
        TraceEditorWidget editor;
        setup(editor, descending, direction);
        editor.chart()->setZoomStrategy(ZoomStrategy(zoom));
        const Span before = span(editor);
        const double tx = pixelTolerance(before.xmin, before.xmax, editor.chart()->chart()->plotArea().width());
        QSignalSpy range(&editor, &TraceEditorWidget::rangeChanged);

        drag(editor, Qt::RightButton, QPointF(6.5, 50), QPointF(2.5, 1));
        QCOMPARE(range.count(), 1);
        const double a = range.at(0).at(0).toDouble(), b = range.at(0).at(1).toDouble();
        QVERIFY2(std::abs(a - 2.5) < tx && std::abs(b - 6.5) < tx, qPrintable(QString("range %1..%2").arg(a).arg(b)));
        QCOMPARE(editor.rangeStart(), a);
        QCOMPARE(editor.rangeEnd(), b);

        drag(editor, Qt::RightButton, QPointF(1, 1), QPointF(4, 80));
        QCOMPARE(range.count(), 2);
        QVERIFY(std::abs(range.at(1).at(0).toDouble() - 1) < tx && std::abs(range.at(1).at(1).toDouble() - 4) < tx);

        const Span after = span(editor);
        QCOMPARE(after.xmin, before.xmin);
        QCOMPARE(after.xmax, before.xmax);
        QCOMPARE(after.ymin, before.ymin);
        QCOMPARE(after.ymax, before.ymax);
    }

    /*! \brief ESC cancels a zoom drag; the next horizontal zoom spans the whole plot height again. */
    void escapeCancelsDrag()
    {
        TraceEditorWidget editor;
        setup(editor, true, Direction::Reversed);
        editor.chart()->setZoomStrategy(ZoomStrategy::Horizontal);
        const Span before = span(editor);
        const QPoint p = pixel(editor, 2, 50), q = pixel(editor, 6, 10);
        send(editor, QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton);
        send(editor, QEvent::MouseMove, q, Qt::NoButton, Qt::LeftButton);
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(editor.chart()->privateView(), &escape);
        send(editor, QEvent::MouseButtonRelease, q, Qt::LeftButton, Qt::NoButton);
        QCOMPARE(span(editor).xmin, before.xmin);
        QCOMPARE(span(editor).xmax, before.xmax);
        drag(editor, Qt::LeftButton, QPointF(3, 50), QPointF(7, 10));
        const Span after = span(editor);
        const double tx = pixelTolerance(before.xmin, before.xmax, editor.chart()->chart()->plotArea().width());
        QVERIFY2(std::abs(after.xmin - 3) < tx && std::abs(after.xmax - 7) < tx, qPrintable(QString("x %1..%2").arg(after.xmin).arg(after.xmax)));
        QCOMPARE(after.ymin, before.ymin);
        QCOMPARE(after.ymax, before.ymax);
    }

    void axisDirection_data()
    {
        QTest::addColumn<bool>("descending");
        QTest::addColumn<Direction>("direction");
        QTest::addColumn<bool>("reversed");
        QTest::newRow("ascending auto") << false << Direction::Auto << false;
        QTest::newRow("descending auto") << true << Direction::Auto << true;
        QTest::newRow("ascending reversed") << false << Direction::Reversed << true;
        QTest::newRow("descending reversed") << true << Direction::Reversed << true;
        QTest::newRow("ascending normal") << false << Direction::Normal << false;
        QTest::newRow("descending normal") << true << Direction::Normal << false;
    }

    /*! \brief The screen direction follows the setting, not the data order; range, zoom to range, range limits,
     * peak markers and the double-click position follow the axis. */
    void axisDirection()
    {
        QFETCH(bool, descending);
        QFETCH(Direction, direction);
        QFETCH(bool, reversed);
        TraceEditorWidget editor;
        setup(editor, descending, direction);
        QCOMPARE(editor.isAxisReversed(), reversed);
        QCOMPARE(axis(editor, Qt::Horizontal)->isReverse(), reversed);
        QCOMPARE(pixel(editor, 8, 0).x() < pixel(editor, 2, 0).x(), reversed);
        QCOMPARE(editor.rangeStart(), 0.0);
        QCOMPARE(editor.rangeEnd(), 10.0);

        // right drag: sorted range, either way of dragging
        const double tx = pixelTolerance(0, 10, editor.chart()->chart()->plotArea().width());
        QSignalSpy range(&editor, &TraceEditorWidget::rangeChanged);
        drag(editor, Qt::RightButton, QPointF(7.5, 50), QPointF(2.25, 1));
        QCOMPARE(range.count(), 1);
        QVERIFY2(std::abs(editor.rangeStart() - 2.25) < tx && std::abs(editor.rangeEnd() - 7.5) < tx, qPrintable(QString("%1..%2").arg(editor.rangeStart()).arg(editor.rangeEnd())));

        // range limits: two vertical dashed series at start and end, on the side the axis says
        editor.setRange(3, 6);
        int limits = 0;
        for (QAbstractSeries* series : editor.chart()->chart()->series()) {
            auto* line = qobject_cast<QLineSeries*>(series);
            if (!line || line->count() != 2 || line->at(0).x() != line->at(1).x())
                continue;
            const double x = line->at(0).x();
            QVERIFY(x == 3 || x == 6);
            ++limits;
        }
        QCOMPARE(limits, 2);
        QCOMPARE(pixel(editor, 6, 0).x() < pixel(editor, 3, 0).x(), reversed);

        // peak markers of the PickPeaks step at the peaks (0.5, 3, 5, 7 ppm)
        QCOMPARE(editor.peaks().size(), 1);
        QVERIFY(editor.peaks().at(0).size() >= 4);
        int markers = 0;
        for (QAbstractSeries* series : editor.chart()->chart()->series())
            if (auto* scatter = qobject_cast<QScatterSeries*>(series))
                markers += scatter->count();
        QCOMPARE(markers, editor.peaks().at(0).size());

        // double click: value under the pointer
        QList<double> clicked;
        connect(editor.chart(), &ChartView::pointDoubleClicked, &editor, [&clicked](const QPointF& p) { clicked << p.x(); });
        const QPoint at = pixel(editor, 7, 1);
        send(editor, QEvent::MouseButtonDblClick, at, Qt::LeftButton, Qt::LeftButton);
        QCOMPARE(clicked.size(), 1);
        QVERIFY2(std::abs(clicked.first() - 7) < tx, qPrintable(QString::number(clicked.first())));

        // zoom to range keeps the direction
        QPushButton* zoom = nullptr;
        for (QPushButton* button : editor.findChildren<QPushButton*>())
            if (button->text() == QStringLiteral("Zoom to range"))
                zoom = button;
        QVERIFY(zoom);
        zoom->click();
        QCOMPARE(span(editor).xmin, 3.0);
        QCOMPARE(span(editor).xmax, 6.0);
        QCOMPARE(axis(editor, Qt::Horizontal)->isReverse(), reversed);
        QCOMPARE(pixel(editor, 5.5, 0).x() < pixel(editor, 3.5, 0).x(), reversed);
    }

    /*! \brief The direction changes at run time and stays over a redraw of the traces. */
    void axisDirectionAtRunTime()
    {
        TraceEditorWidget editor;
        setup(editor, false);
        QVERIFY(!axis(editor, Qt::Horizontal)->isReverse());
        editor.setAxisDirection(Direction::Reversed);
        QVERIFY(axis(editor, Qt::Horizontal)->isReverse());
        QVERIFY(pixel(editor, 8, 0).x() < pixel(editor, 2, 0).x());
        editor.setShowProcessed(false); // redraw
        QVERIFY(axis(editor, Qt::Horizontal)->isReverse());
        editor.setAxisDirection(Direction::Normal);
        QVERIFY(!axis(editor, Qt::Horizontal)->isReverse());
        TraceSet set;
        set.add(spectrum(true));
        editor.setTraces(set);
        QVERIFY(!axis(editor, Qt::Horizontal)->isReverse());
        editor.setAxisDirection(Direction::Auto);
        QVERIFY(axis(editor, Qt::Horizontal)->isReverse());
        QSignalSpy range(&editor, &TraceEditorWidget::rangeChanged);
        drag(editor, Qt::RightButton, QPointF(4, 1), QPointF(1, 1));
        QCOMPARE(range.count(), 1);
        QVERIFY(range.at(0).at(0).toDouble() < range.at(0).at(1).toDouble());
    }

    /*! \brief NMR dialog: the ppm axis is reversed by default also for ascending data; the check box sets
     * Normal and is remembered in the settings. */
    void nmrDialogAxis()
    {
        QSettings().remove("nmrimport/reverseAxis");
        {
            NmrImport dialog;
            for (int k = 0; k < 3; ++k)
                dialog.handler()->addSpectrum(spectrum(false));
            dialog.refresh();
            auto* box = dialog.findChild<QCheckBox*>("nmrReverseAxis");
            auto* editor = dialog.findChild<TraceEditorWidget*>();
            QVERIFY(box && editor);
            QVERIFY(box->isChecked());
            QCOMPARE(editor->axisDirection(), Direction::Reversed);
            QVERIFY(axis(*editor, Qt::Horizontal)->isReverse());
            box->setChecked(false);
            QCOMPARE(editor->axisDirection(), Direction::Normal);
            QVERIFY(!axis(*editor, Qt::Horizontal)->isReverse());
            QCOMPARE(QSettings().value("nmrimport/reverseAxis").toBool(), false);
        }
        {
            NmrImport dialog;
            auto* box = dialog.findChild<QCheckBox*>("nmrReverseAxis");
            QVERIFY(!box->isChecked());
            QCOMPARE(dialog.findChild<TraceEditorWidget*>()->axisDirection(), Direction::Normal);
            box->setChecked(true);
        }
        QCOMPARE(QSettings().value("nmrimport/reverseAxis").toBool(), true);
    }

    /*! \brief Viewer 2: the zoom of the user stays over a change of the steps, the axis direction, the stack offset and
     * raw/processed; the same traces with a shifted axis move range and zoom along; other traces reset it. */
    void zoomKeptOverRedraws()
    {
        TraceEditorWidget editor;
        editor.setAxisDirection(Direction::Reversed);
        TraceSet set;
        set.add(spectrum(true), "a");
        set.add(spectrum(true), "b");
        editor.setTraces(set);
        editor.resize(1100, 640);
        editor.show();
        QVERIFY(QTest::qWaitForWindowExposed(&editor));
        QCoreApplication::processEvents();
        editor.chart()->setZoomStrategy(ZoomStrategy::Rectangular);
        drag(editor, Qt::LeftButton, QPointF(6, 2), QPointF(2, 0.2));
        QVERIFY(editor.hasUserView());
        const Span zoomed = span(editor);
        auto same = [&](const QString& what) {
            const Span now = span(editor);
            QVERIFY2(now.xmin == zoomed.xmin && now.xmax == zoomed.xmax && now.ymin == zoomed.ymin && now.ymax == zoomed.ymax,
                qPrintable(QString("%1: x %2..%3 y %4..%5, zoom was x %6..%7 y %8..%9").arg(what).arg(now.xmin).arg(now.xmax).arg(now.ymin).arg(now.ymax).arg(zoomed.xmin).arg(zoomed.xmax).arg(zoomed.ymin).arg(zoomed.ymax)));
        };
        QCOMPARE(editor.addOperation("Smooth"), 0);
        same("step added");
        QVERIFY(editor.setOperationParameters(0, QJsonObject{ { "Window", 9 } }));
        same("step changed");
        QVERIFY(editor.setOperationEnabled(0, false));
        same("step switched off");
        editor.setShowProcessed(false);
        same("raw view");
        editor.setAxisDirection(Direction::Normal);
        same("axis direction");
        editor.setStackPercent(30);
        same("stack offset");
        editor.setStackPercent(0);
        // a host adding a series (rescales the axes) and calling refreshMarkers
        auto* extra = new LineSeries;
        extra->append(0, -50);
        extra->append(10, 500);
        editor.chart()->addSeries(extra);
        editor.refreshMarkers();
        same("series of the host");

        // the same traces on an axis moved by 0.25 (referencing): range and zoom move along
        editor.setRange(3, 7);
        TraceSet moved;
        for (const QString& id : { QString("a"), QString("b") }) {
            XYTrace t = spectrum(true);
            Eigen::VectorXd x = t.x().array() + 0.25;
            t.setData(x, t.y());
            moved.add(t, id);
        }
        editor.setTraces(moved);
        QVERIFY(editor.hasUserView());
        QVERIFY(std::abs(span(editor).xmin - (zoomed.xmin + 0.25)) < 1e-12 && std::abs(span(editor).xmax - (zoomed.xmax + 0.25)) < 1e-12);
        QCOMPARE(span(editor).ymin, zoomed.ymin);
        QVERIFY(std::abs(editor.rangeStart() - 3.25) < 1e-12 && std::abs(editor.rangeEnd() - 7.25) < 1e-12);

        // other traces: whole range
        TraceSet other;
        other.add(spectrum(true), "c");
        editor.setTraces(other);
        QVERIFY(!editor.hasUserView());
        QCOMPARE(span(editor).xmin, 0.0);
        QCOMPARE(span(editor).xmax, 10.0);
        QCOMPARE(editor.rangeStart(), 0.0);
        QCOMPARE(editor.rangeEnd(), 10.0);
    }

    /*! \brief Viewer 2 (Viewer 3: with Ctrl): one notch scales the y span by 0.9 (up) or 1/0.9 (down) about the value
     * under the pointer; x and the intensity factor stay; the middle button goes back to the whole range. */
    void ctrlWheelScalesY()
    {
        TraceEditorWidget editor;
        setup(editor, true, Direction::Reversed);
        const Span before = span(editor);
        const QPoint at = pixel(editor, 4, 20);
        const double anchor = valueAt(editor, at).y();
        wheel(editor, at, 120, Qt::ControlModifier);
        QCOMPARE(editor.intensityScale(), 1.0);
        Span after = span(editor);
        const double tolerance = 1e-9 * (before.ymax - before.ymin);
        QVERIFY2(std::abs(after.ymin - (anchor - 0.9 * (anchor - before.ymin))) < tolerance && std::abs(after.ymax - (anchor + 0.9 * (before.ymax - anchor))) < tolerance,
            qPrintable(QString("y %1..%2 -> %3..%4 at %5").arg(before.ymin).arg(before.ymax).arg(after.ymin).arg(after.ymax).arg(anchor)));
        QCOMPARE(after.xmin, before.xmin);
        QCOMPARE(after.xmax, before.xmax);
        QVERIFY(editor.hasUserView());
        // the value under the pointer stays under it (within a pixel)
        const double pixel_value = (before.ymax - before.ymin) / editor.chart()->chart()->plotArea().height();
        QVERIFY(std::abs(valueAt(editor, at).y() - anchor) < pixel_value);
        // two notches down at the same place: back to 1/0.9 of the start span
        wheel(editor, at, -240, Qt::ControlModifier);
        after = span(editor);
        QVERIFY(std::abs((after.ymax - after.ymin) - (before.ymax - before.ymin) / 0.9) < tolerance * 10);
        // kept over a redraw, reset by the middle button
        editor.addOperation("Smooth");
        QCOMPARE(span(editor).ymin, after.ymin);
        const QPoint centre = pixel(editor, 4, 1);
        send(editor, QEvent::MouseButtonPress, centre, Qt::MiddleButton, Qt::MiddleButton);
        send(editor, QEvent::MouseButtonRelease, centre, Qt::MiddleButton, Qt::NoButton);
        QVERIFY(!editor.hasUserView());
        QCOMPARE(span(editor).xmin, before.xmin);
        QCOMPARE(span(editor).xmax, before.xmax);
        QVERIFY(editor.hintText().contains("Ctrl+wheel: y zoom"));
    }

    void wheelScalesIntensity_data()
    {
        QTest::addColumn<bool>("descending");
        QTest::addColumn<Direction>("direction");
        QTest::addColumn<double>("stack");
        QTest::addColumn<bool>("reverse");
        QTest::newRow("ascending normal") << false << Direction::Normal << 0.0 << false;
        QTest::newRow("descending reversed") << true << Direction::Reversed << 0.0 << false;
        QTest::newRow("stacked ascending normal") << false << Direction::Normal << 60.0 << false;
        QTest::newRow("stacked descending reversed") << true << Direction::Reversed << 60.0 << false;
        QTest::newRow("stacked reverse order, ascending reversed") << false << Direction::Reversed << 60.0 << true;
        QTest::newRow("stacked reverse order, descending normal") << true << Direction::Normal << 60.0 << true;
    }

    /*! \brief Viewer 3: the wheel (no modifier) multiplies the intensity factor by 1.25 per notch: the axes stay, the
     * baselines stay at their pixels (with and without a stack, both axis directions, reverse order), the distances of
     * the levels stay, a peak of height h rises to 1.25^n h above its baseline, the peak markers sit on the scaled
     * tops; finer wheels in proportion; limits; y labels hidden and the factor shown while it is not 1. Claude Generated */
    void wheelScalesIntensity()
    {
        QFETCH(bool, descending);
        QFETCH(Direction, direction);
        QFETCH(double, stack);
        QFETCH(bool, reverse);
        TraceEditorWidget editor;
        setupStack(editor, descending, direction);
        editor.setStackPercent(stack);
        editor.setStackReversed(reverse);
        editor.findChild<QPushButton*>("traceFitY")->click(); // a fixed axis for the pixel comparison
        const Span before = span(editor);
        QChart* chart = editor.chart()->chart();
        const XYTrace& raw = editor.traces().at(0);
        Eigen::Index top = 0;
        raw.y().maxCoeff(&top); // the 100 high reference signal; point 0 lies on the baseline (y < 1e-100)
        auto py = [&](int k, int i) { return chart->mapToPosition(line(editor, k)->at(i)).y(); };
        QVector<double> base, height;
        for (int k = 0; k < 3; ++k) {
            base << py(k, 0);
            height << py(k, 0) - py(k, int(top));
        }
        auto* label = editor.findChild<QLabel*>("traceIntensity");
        QVERIFY(label && label->isHidden());
        QVERIFY(stack != 0 || axis(editor, Qt::Vertical)->labelsVisible());
        const QPoint at = pixel(editor, 4, 0.5 * (before.ymin + before.ymax));
        auto check = [&](double factor, const char* what) {
            QVERIFY2(std::abs(editor.intensityScale() - factor) < 1e-12 * factor, qPrintable(QString("%1: factor %2, expected %3").arg(what).arg(editor.intensityScale()).arg(factor)));
            const Span now = span(editor);
            QVERIFY2(now.xmin == before.xmin && now.xmax == before.xmax && now.ymin == before.ymin && now.ymax == before.ymax, what);
            for (int k = 0; k < 3; ++k) {
                QVERIFY2(std::abs(py(k, 0) - base[k]) < 1e-6, qPrintable(QString("%1: baseline %2 at %3 px, was %4").arg(what).arg(k).arg(py(k, 0)).arg(base[k])));
                const double h = py(k, 0) - py(k, int(top));
                QVERIFY2(std::abs(h - factor * height[k]) < 1e-6 * factor * height[k], qPrintable(QString("%1: peak %2 px high, expected %3").arg(what).arg(h).arg(factor * height[k])));
            }
            // peak markers: displayPoint of the picked peak, on the top of the drawn line
            int markers = 0;
            for (QAbstractSeries* series : chart->series())
                if (auto* scatter = qobject_cast<QScatterSeries*>(series)) {
                    const int k = markers++;
                    for (int j = 0; j < scatter->count(); ++j) {
                        const TracePeak& peak = editor.peaks().at(k).at(j);
                        QCOMPARE(scatter->at(j), editor.displayPoint(k, peak.x, peak.height));
                        const double rise = scatter->at(j).y() - editor.stackOffset(k).y();
                        QVERIFY2(std::abs(scatter->at(j).y() - line(editor, k)->at(int(peak.index)).y()) < 0.01 * rise, what);
                    }
                }
            QCOMPARE(markers, 3);
            QCOMPARE(label->isHidden(), factor == 1);
            QCOMPARE(axis(editor, Qt::Vertical)->labelsVisible(), stack == 0 && factor == 1);
        };
        wheel(editor, at, 120);
        check(1.25, "one notch up");
        QVERIFY(label->text().contains("1.25") || label->text().contains("1,25"));
        // the distances of the levels in pixels are unchanged
        if (stack != 0)
            QVERIFY(std::abs((py(0, 0) - py(1, 0)) - (base[0] - base[1])) < 1e-6 && std::abs(base[0] - base[1]) > 20);
        wheel(editor, at, -120);
        check(1, "one notch down");
        wheel(editor, at, 360);
        check(std::pow(1.25, 3), "three notches");
        wheel(editor, at, 60); // a fine wheel: half a notch
        check(std::pow(1.25, 3.5), "half a notch");
        wheel(editor, at, -60);
        // limits
        wheel(editor, at, 120 * 100);
        check(TraceEditorWidget::kMaxIntensity, "upper limit");
        wheel(editor, at, -120 * 200);
        check(TraceEditorWidget::kMinIntensity, "lower limit");
        editor.setIntensityScale(4);
        check(4, "set");
        // Ctrl + wheel: the y axis changes, the factor stays
        wheel(editor, at, 120, Qt::ControlModifier);
        QCOMPARE(editor.intensityScale(), 4.0);
        QVERIFY(span(editor).ymax - span(editor).ymin < before.ymax - before.ymin);
        // "Fit y to the visible range": factor 1, y to the unscaled traces (as before scaling)
        editor.findChild<QPushButton*>("traceFitY")->click();
        check(1, "fit y");
        // middle click: factor 1 and the whole view
        wheel(editor, at, 240);
        send(editor, QEvent::MouseButtonPress, at, Qt::MiddleButton, Qt::MiddleButton);
        send(editor, QEvent::MouseButtonRelease, at, Qt::MiddleButton, Qt::NoButton);
        QCOMPARE(editor.intensityScale(), 1.0);
        QVERIFY(!editor.hasUserView());
        QVERIFY(label->isHidden());
    }

    /*! \brief Viewer 3: the intensity factor stays over steps, stack offsets and the axis direction; other traces set it
     * to 1; it is not stored. Claude Generated */
    void intensityKeptAndReset()
    {
        TraceEditorWidget editor;
        editor.setSettingsKey("intensityHost");
        setupStack(editor, true, Direction::Reversed);
        const Span before = span(editor);
        wheel(editor, pixel(editor, 4, 0.5 * (before.ymin + before.ymax)), 240);
        QCOMPARE(editor.intensityScale(), 1.5625);
        QVERIFY(editor.hasUserView());
        const Span scaled = span(editor);
        QCOMPARE(scaled.ymin, before.ymin); // the wheel does not change the axis
        QCOMPARE(scaled.ymax, before.ymax);
        QVERIFY(editor.addOperation("Smooth") >= 0);
        QCOMPARE(editor.intensityScale(), 1.5625);
        editor.setStackPercent(40);
        editor.setStackXPercent(2);
        editor.setStackReversed(true);
        editor.setAxisDirection(Direction::Normal);
        QCOMPARE(editor.intensityScale(), 1.5625);
        QCOMPARE(line(editor, 1)->at(7).y(), editor.displayPoint(1, 0, editor.traces().at(1).y()(7)).y());
        // an x range set by the host while scaled stays over "Fit y to the visible range" and a new factor
        editor.chart()->setXRange(3, 6);
        editor.fitYToVisible();
        QCOMPARE(editor.intensityScale(), 1.0);
        QCOMPARE(span(editor).xmin, 3.0);
        QCOMPARE(span(editor).xmax, 6.0);
        editor.chart()->setXRange(2, 7);
        editor.setIntensityScale(1.5625);
        QCOMPARE(span(editor).xmin, 2.0);
        QCOMPARE(span(editor).xmax, 7.0);
        // the same ids again: kept; other ids: 1
        editor.setTraces(editor.traces());
        QCOMPARE(editor.intensityScale(), 1.5625);
        TraceSet other;
        other.add(spectrum(true), "other");
        editor.setTraces(other);
        QCOMPARE(editor.intensityScale(), 1.0);
        QVERIFY(editor.findChild<QLabel*>("traceIntensity")->isHidden());
        for (const QString& key : QSettings().allKeys())
            QVERIFY2(!key.contains("ntensity", Qt::CaseInsensitive) || key.startsWith("traceeditor/intensityHost/stack"), qPrintable(key));
        QSettings().remove("traceeditor/intensityHost");
    }

    /*! \brief Viewer 2: "Fit y to the visible range" scales y to the traces between the ends of the x axis (+5 %). */
    void fitYToVisibleRange()
    {
        TraceEditorWidget editor;
        setup(editor, false, Direction::Auto);
        editor.chart()->setZoomStrategy(ZoomStrategy::Horizontal);
        drag(editor, Qt::LeftButton, QPointF(2, 50), QPointF(6, 50));
        const Span zoomed = span(editor);
        QVERIFY(zoomed.ymax >= 100); // the reference line at 0.5 ppm still sets y
        auto* button = editor.findChild<QPushButton*>("traceFitY");
        QVERIFY(button);
        button->click();
        const XYTrace& t = editor.traces().at(0);
        double lo = 1e300, hi = -1e300;
        for (Eigen::Index i = 0; i < t.size(); ++i)
            if (t.x()(i) >= zoomed.xmin && t.x()(i) <= zoomed.xmax) {
                lo = std::min(lo, t.y()(i));
                hi = std::max(hi, t.y()(i));
            }
        const Span fitted = span(editor);
        QVERIFY2(std::abs(fitted.ymin - (lo - 0.05 * (hi - lo))) < 1e-12 && std::abs(fitted.ymax - (hi + 0.05 * (hi - lo))) < 1e-12,
            qPrintable(QString("y %1..%2 for data %3..%4").arg(fitted.ymin).arg(fitted.ymax).arg(lo).arg(hi)));
        QCOMPARE(fitted.xmin, zoomed.xmin);
        QCOMPARE(fitted.xmax, zoomed.xmax);
        QVERIFY(hi < 1.5);
    }

    void stacking_data()
    {
        QTest::addColumn<bool>("descending");
        QTest::addColumn<Direction>("direction");
        QTest::newRow("ascending normal") << false << Direction::Normal;
        QTest::newRow("descending reversed") << true << Direction::Reversed;
        QTest::newRow("ascending reversed") << false << Direction::Reversed;
        QTest::newRow("descending normal") << true << Direction::Normal;
    }

    /*! \brief Viewer 2: stack offsets move the series of trace k (and its peak markers) by stackOffset(k): y by
     * level x percent of the reference height, x to the right on the screen in either axis direction; reverse
     * order, excluded traces, reference height independent of the zoom, y labels hidden while stacked. */
    void stacking()
    {
        QFETCH(bool, descending);
        QFETCH(Direction, direction);
        TraceEditorWidget editor;
        setupStack(editor, descending, direction);
        const XYTrace& raw = editor.traces().at(0);
        const double reference = raw.y().maxCoeff() - raw.y().minCoeff();
        QCOMPARE(editor.stackReferenceHeight(), reference);
        QVERIFY(axis(editor, Qt::Vertical)->labelsVisible());
        editor.setStackPercent(100);
        QVERIFY(!axis(editor, Qt::Vertical)->labelsVisible());
        editor.findChild<QPushButton*>("traceFitY")->click(); // a fixed y axis: the pixels below compare at one scale
        // screen position (plot coordinates, sub-pixel) of the first point of trace k
        auto levelPixel = [&](int k) { return editor.chart()->chart()->mapToPosition(line(editor, k)->at(0)).y(); };
        const double d100 = levelPixel(0) - levelPixel(1);
        QVERIFY2(d100 > 20, qPrintable(QString::number(d100))); // trace 1 drawn higher than trace 0
        QVERIFY(std::abs((levelPixel(1) - levelPixel(2)) - d100) < 1e-6);
        editor.setStackPercent(50);
        const double d50 = levelPixel(0) - levelPixel(1);
        QVERIFY2(std::abs(2 * d50 - d100) < 1e-6, qPrintable(QString("%1 px at 100 %, %2 px at 50 %").arg(d100).arg(d50)));
        // the points: exactly the trace moved by level x 50 % of the reference height
        for (int k = 0; k < 3; ++k) {
            QCOMPARE(editor.stackOffset(k), QPointF(0, k * 0.5 * reference));
            QCOMPARE(line(editor, k)->count(), int(raw.size()));
            for (int i = 0; i < int(raw.size()); i += 97)
                QCOMPARE(line(editor, k)->at(i).y(), raw.y()(i) + k * 0.5 * reference);
        }
        // peak markers move along
        int markers = 0;
        for (QAbstractSeries* series : editor.chart()->chart()->series())
            if (auto* scatter = qobject_cast<QScatterSeries*>(series)) {
                const int k = markers++;
                QCOMPARE(scatter->count(), int(editor.peaks().at(k).size()));
                QCOMPARE(scatter->at(0).y(), editor.peaks().at(k).at(0).height + editor.stackOffset(k).y());
            }
        QCOMPARE(markers, 3);
        // reference height: not changed by a zoom, but by the range
        editor.chart()->setZoomStrategy(ZoomStrategy::Rectangular);
        drag(editor, Qt::LeftButton, QPointF(2, 40), QPointF(6, 10));
        QCOMPARE(editor.stackReferenceHeight(), reference);
        editor.setRange(2, 8);
        QVERIFY2(editor.stackReferenceHeight() < 0.02 * reference, qPrintable(QString::number(editor.stackReferenceHeight())));
        QCOMPARE(line(editor, 1)->at(0).y(), raw.y()(0) + 0.5 * editor.stackReferenceHeight());
        editor.setRange(0, 10);
        // reverse order: the first trace on top
        editor.setStackReversed(true);
        QCOMPARE(editor.stackOffset(0).y(), 2 * 0.5 * reference);
        QCOMPARE(editor.stackOffset(2).y(), 0.0);
        editor.setStackReversed(false);
        // an excluded trace does not count and is not drawn
        editor.setExcludedTraces({ false, true, false });
        QVERIFY(!line(editor, 1)->isVisible());
        QCOMPARE(editor.stackOffset(1), QPointF(0, 0));
        QCOMPARE(editor.stackOffset(2).y(), 0.5 * reference);
        editor.setExcludedTraces({});
        // x offset: 10 % of the range = 1 ppm per level, to the right on the screen
        editor.setStackXPercent(10);
        const bool reversed = editor.isAxisReversed();
        QCOMPARE(editor.stackOffset(1).x(), reversed ? -1.0 : 1.0);
        QCOMPARE(editor.stackOffset(2).x(), reversed ? -2.0 : 2.0);
        for (int k = 1; k < 3; ++k)
            QVERIFY(pixel(editor, line(editor, k)->at(0).x(), 0).x() > pixel(editor, line(editor, k - 1)->at(0).x(), 0).x());
        // back to 0: unchanged data, labels again
        editor.setStackPercent(0);
        editor.setStackXPercent(0);
        QVERIFY(axis(editor, Qt::Vertical)->labelsVisible());
        for (int k = 0; k < 3; ++k)
            QCOMPARE(line(editor, k)->at(5), QPointF(raw.x()(5), raw.y()(5)));
    }

    /*! \brief Viewer 2: the stack settings are remembered per host key. */
    void stackSettingsPerHost()
    {
        {
            TraceEditorWidget editor;
            editor.setSettingsKey("hostA");
            QCOMPARE(editor.stackPercent(), 0.0);
            editor.setStackPercent(40);
            editor.setStackXPercent(2.5);
            editor.setStackReversed(true);
        }
        TraceEditorWidget a, b, none;
        a.setSettingsKey("hostA");
        b.setSettingsKey("hostB");
        QCOMPARE(a.stackPercent(), 40.0);
        QCOMPARE(a.stackXPercent(), 2.5);
        QVERIFY(a.stackReversed());
        QCOMPARE(b.stackPercent(), 0.0);
        QVERIFY(!b.stackReversed());
        QCOMPARE(none.stackPercent(), 0.0);
        QCOMPARE(QSettings().value("traceeditor/hostA/stackY").toDouble(), 40.0);
    }

    /*! \brief With SUPRAFIT_TRACE_RENDER_DIR: the NMR dialog with ascending data on the reversed ppm axis,
     * before and after a vertical zoom by mouse, light and dark, 1366x768. */
    void renderNmrZoom()
    {
        const QString dir = qEnvironmentVariable("SUPRAFIT_TRACE_RENDER_DIR");
        if (dir.isEmpty())
            QSKIP("SUPRAFIT_TRACE_RENDER_DIR not set");
        QDir().mkpath(dir);
        for (int mode : { int(Appearance::Light), int(Appearance::Dark) }) {
            Appearance::Apply(mode);
            const QString suffix = mode == Appearance::Light ? "" : "_dark";
            NmrImport dialog;
            for (int k = 0; k < 4; ++k)
                dialog.handler()->addSpectrum(spectrum(false, 1001));
            dialog.handler()->addSignal(5.0, 0.2, "H1");
            dialog.refresh();
            dialog.resize(1366, 768);
            dialog.show();
            QVERIFY(QTest::qWaitForWindowExposed(&dialog));
            QTest::qWait(80);
            auto* editor = dialog.findChild<TraceEditorWidget*>();
            QVERIFY(dialog.grab().save(QDir(dir).filePath("chartzoom_before" + suffix + ".png")));
            editor->chart()->setZoomStrategy(ZoomStrategy::Vertical);
            drag(*editor, Qt::LeftButton, QPointF(5, 1.6), QPointF(5, -0.1));
            QTest::qWait(80);
            QVERIFY(dialog.grab().save(QDir(dir).filePath("chartzoom_vertical" + suffix + ".png")));
            qInfo("%s: y %g..%g, x %g..%g, reversed %d", qPrintable(suffix), span(*editor).ymin, span(*editor).ymax, span(*editor).xmin, span(*editor).xmax, int(axis(*editor, Qt::Horizontal)->isReverse()));
        }
        Appearance::Apply(Appearance::Light);
    }
};

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QTemporaryDir settingsDir;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsDir.path());
    QCoreApplication::setOrganizationName(QStringLiteral("SupraFitChartZoomTest"));
    QCoreApplication::setApplicationName(QStringLiteral("test_chartzoom"));
    Appearance::Install();
    TestChartZoom test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_chartzoom.moc"
