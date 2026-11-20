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
#include <QtTest/QSignalSpy>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

#include <charts.h>

#include <QtCore/QDir>
#include <QtWidgets/QCheckBox>
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
