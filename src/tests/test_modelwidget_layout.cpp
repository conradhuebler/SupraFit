/*
 * SupraFit - offscreen layout test for the ModelWidget of a scripted model
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * Safety net for roadmap/scriptmodel_ui_layout.md (WP0): builds the ModelWidget of a ScriptModel
 * without a display and checks what must stay true while the pane is rearranged:
 *  (1) building and showing the widget leaves no other visible top-level window (the flicker
 *      regression: setHidden(false) on a parentless widget shows it as a window);
 *  (2) the widget renders (grab() is not empty);
 *  (3) the pane keeps a usable size hint at the narrow widths of small screens.
 * Set SUPRAFIT_LAYOUT_RENDER_DIR to write PNG renders of the widths tried. Claude Generated.
 */

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtCore/QJsonObject>
#include <QtCore/QThread>
#include <QtGui/QPixmap>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolButton>

#include <Eigen/Dense>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/global.h"

#include "src/capabilities/jobmanager.h"
#include "src/ui/dialogs/resultsdialog.h"
#include "src/ui/dialogs/statisticdialog.h"
#include "src/ui/guitools/flowlayout.h"
#include "src/ui/guitools/morelesstoggle.h"
#include "src/ui/instance.h"
#include "src/ui/mainwindow/chartwidget.h"
#include "src/ui/mainwindow/modelwidget.h"

// Counts widgets that were shown as their own window while the filter is installed. A parentless
// widget that gets setHidden(false) before its layout adopts it is shown, then hidden again on
// reparenting - invisible in the final state, so the end state alone cannot catch the flicker.
class WindowShowCounter : public QObject {
public:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::Show) {
            QWidget* w = qobject_cast<QWidget*>(obj);
            if (w && w->isWindow() && w != m_allowed)
                m_names << QString::fromLatin1(w->metaObject()->className());
        }
        return false;
    }
    void allow(QWidget* w) { m_allowed = w; }
    QStringList shown() const { return m_names; }

private:
    QWidget* m_allowed = nullptr;
    QStringList m_names;
};

class TestModelWidgetLayout : public QObject {
    Q_OBJECT

private:
    static constexpr int N = 15;

    static QJsonObject descr(const QJsonValue& value)
    {
        QJsonObject o;
        o["value"] = value;
        return o;
    }

    // Michaelis-Menten with one local parameter: global and local parameter pages both exist.
    static QJsonObject definition()
    {
        QJsonObject lines;
        lines["0"] = QStringLiteral("(vmax*X1)/(Km+X1)+offset");
        QJsonObject def;
        def["Name"] = descr(QStringLiteral("Layout ScriptModel"));
        def["InputSize"] = descr(1);
        def["InputNames"] = descr(QStringLiteral("X1"));
        def["DepModelNames"] = descr(QStringLiteral("Y"));
        def["GlobalParameterSize"] = descr(2);
        def["GlobalParameterNames"] = descr(QStringLiteral("vmax|Km"));
        def["LocalParameterSize"] = descr(1);
        def["LocalParameterNames"] = descr(QStringLiteral("offset"));
        def["Equation"] = descr(lines);
        return def;
    }

    static DataClass* makeData(int series)
    {
        Eigen::MatrixXd indep(N, 1);
        Eigen::MatrixXd dep(N, series);
        for (int i = 0; i < N; ++i) {
            const double x = 0.1 + 0.5 * i;
            indep(i, 0) = x;
            for (int j = 0; j < series; ++j)
                dep(i, j) = 3.5 * x / (0.8 + x) + 0.1 * j;
        }
        DataClass* data = new DataClass();
        data->setIndependentTable(new DataTable(indep));
        data->setDataType(DataClassPrivate::Table);
        data->setSimulateDependent(series);
        data->setDependentTable(new DataTable(dep));
        data->setDataBegin(0);
        data->setDataEnd(N);
        return data;
    }

    static QList<QWidget*> visibleTopLevels()
    {
        QList<QWidget*> list;
        for (QWidget* w : QApplication::topLevelWidgets())
            if (w->isVisible())
                list << w;
        return list;
    }

    QSharedPointer<DataClass> m_data;
    QSharedPointer<AbstractModel> m_model;
    ChartWidget* m_chart = nullptr;

private slots:
    void initTestCase()
    {
        qApp->setProperty("threads", QThread::idealThreadCount());
        qApp->setProperty("lineWidth", 20);
        Instance::setInstance(new Instance);

        m_data = QSharedPointer<DataClass>(makeData(2));
        m_model = CreateModel(SupraFit::ScriptModel, m_data.data());
        QVERIFY(!m_model.isNull());
        QVERIFY(m_model->DefineModel(definition()));
        m_model->setGlobalParameter(3.5, 0);
        m_model->setGlobalParameter(0.8, 1);
        m_model->Calculate();

        m_chart = new ChartWidget;
        m_chart->setRawData(m_data); // creates the data mapper addModel() needs
    }

    void cleanupTestCase() { delete m_chart; }

    // (1) the flicker regression: while the widget is built, nothing but the widget itself may be shown
    // as a window. The filter stays on until the widget is up, so a transient window is counted.
    void noStrayTopLevelWindows()
    {
        Charts charts = m_chart->addModel(m_model);
        QVERIFY(charts.signal_wrapper && charts.error_wrapper && charts.data_wrapper);

        WindowShowCounter counter;
        qApp->installEventFilter(&counter);
        ModelWidget widget(m_model, charts);
        counter.allow(&widget);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();
        qApp->removeEventFilter(&counter);

        QVERIFY2(counter.shown().isEmpty(),
            qPrintable(QStringLiteral("shown as own window during build: %1").arg(counter.shown().join(", "))));
        for (QWidget* w : visibleTopLevels())
            QVERIFY2(w == &widget, qPrintable(QStringLiteral("stray visible window: %1").arg(w->metaObject()->className())));
    }

    // (2)+(3) renders at wide and small-screen widths; optional PNGs for the docs.
    void rendersAtSeveralWidths_data()
    {
        QTest::addColumn<int>("width");
        QTest::addColumn<int>("height");
        QTest::newRow("1366x768") << 1100 << 700;
        QTest::newRow("1024x600") << 760 << 540;
        QTest::newRow("narrow") << 520 << 540;
    }

    void rendersAtSeveralWidths()
    {
        QFETCH(int, width);
        QFETCH(int, height);

        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(width, height);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        const QWidget* base = &widget; // ModelWidget overrides the size hints privately
        QVERIFY(base->sizeHint().isValid());
        QVERIFY2(base->minimumSizeHint().width() > 0, "minimumSizeHint collapsed");
        const QPixmap pix = widget.grab();
        QVERIFY(!pix.isNull());
        QVERIFY(pix.width() > 0 && pix.height() > 0);

        const QString dir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        if (!dir.isEmpty()) {
            QDir().mkpath(dir);
            pix.save(QStringLiteral("%1/modelwidget_%2x%3.png").arg(dir).arg(width).arg(height));
        }
    }
    // Measurement for WP-F (FlowLayout calm-down): sweeps the pane width down and up and reports, per
    // FlowLayout in the tree, rows and height at each step. A pure flow layout must never get taller
    // when it gets wider; hidden items must not take room. Prints a table (qInfo), fails on a
    // non-monotonic height.
    void flowLayoutsStayMonotonic()
    {
        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        QList<FlowLayout*> flows; // FlowLayout has no Q_OBJECT, so findChildren<FlowLayout*> is not available
        for (QLayout* layout : widget.findChildren<QLayout*>())
            if (FlowLayout* flow = dynamic_cast<FlowLayout*>(layout))
                flows << flow;
        QVERIFY2(!flows.isEmpty(), "no FlowLayout found in the pane");
        qInfo("FlowLayouts in the pane: %lld", static_cast<long long>(flows.size()));

        int hiddenTaken = 0;
        for (int f = 0; f < flows.size(); ++f) {
            const FlowLayout* flow = flows[f];
            int hidden = 0;
            for (int i = 0; i < flow->count(); ++i) {
                const QLayoutItem* item = flow->itemAt(i);
                if (item->widget() && item->widget()->isHidden())
                    ++hidden;
            }
            hiddenTaken += hidden;
            qInfo("flow %d: parent=%s items=%d hidden=%d minimumSize=%dx%d", f,
                flow->parent() ? flow->parent()->metaObject()->className() : "none", flow->count(), hidden,
                flow->minimumSize().width(), flow->minimumSize().height());
        }

        for (int f = 0; f < flows.size(); ++f) {
            const FlowLayout* flow = flows[f];
            int previous = -1;
            for (int w = 200; w <= 1400; w += 10) {
                const int h = flow->heightForWidth(w);
                QVERIFY2(previous < 0 || h <= previous,
                    qPrintable(QStringLiteral("flow %1 (%2): height %3 at width %4 is larger than %5 at width %6")
                                   .arg(f)
                                   .arg(QString::fromLatin1(flow->parent() ? flow->parent()->metaObject()->className() : "none"))
                                   .arg(h).arg(w).arg(previous).arg(w - 10)));
                previous = h;
            }
        }

        // Count layout passes the whole pane needs per resize step: the "restlessness" of the live pane.
        struct LayoutCounter : QObject {
            int requests = 0;
            bool eventFilter(QObject*, QEvent* e) override
            {
                if (e->type() == QEvent::LayoutRequest)
                    ++requests;
                return false;
            }
        } counter;
        qApp->installEventFilter(&counter);
        QStringList report;
        for (int w : { 1100, 900, 760, 640, 520, 420, 520, 640, 760, 900, 1100 }) {
            counter.requests = 0;
            widget.resize(w, 700);
            QCoreApplication::processEvents();
            QList<int> rows;
            for (const FlowLayout* flow : flows) {
                QSet<int> ys;
                for (int i = 0; i < flow->count(); ++i)
                    if (flow->itemAt(i)->widget() && !flow->itemAt(i)->widget()->isHidden())
                        ys.insert(flow->itemAt(i)->geometry().y());
                rows << ys.size();
            }
            QStringList r;
            for (int n : rows)
                r << QString::number(n);
            report << QStringLiteral("w=%1 layoutRequests=%2 rows=[%3]").arg(w).arg(counter.requests).arg(r.join(','));
        }
        qApp->removeEventFilter(&counter);
        for (const QString& line : report)
            qInfo("%s", qPrintable(line));
        qInfo("hidden items still inside flow layouts: %d", hiddenTaken);
    }
    // Hidden widgets must take neither room nor spacing in a FlowLayout. Claude Generated.
    void hiddenItemsTakeNoRoomInFlowLayout()
    {
        QWidget host;
        FlowLayout* flow = new FlowLayout(&host, 0, 12, 4);
        QList<QPushButton*> buttons;
        for (int i = 0; i < 5; ++i) {
            QPushButton* b = new QPushButton(QStringLiteral("button %1").arg(i), &host);
            flow->addWidget(b);
            buttons << b;
        }
        host.resize(2000, 100);
        host.show();
        QTest::qWaitForWindowExposed(&host);
        const int widthAll = flow->heightForWidth(2000) >= 0 ? buttons[4]->geometry().right() : 0;

        buttons[1]->hide();
        buttons[2]->hide();
        buttons[3]->hide();
        QCoreApplication::processEvents();
        // the last button now follows the first one directly after one spacing
        QCOMPARE(buttons[4]->geometry().left(), buttons[0]->geometry().right() + 1 + 12);
        QVERIFY(buttons[4]->geometry().right() < widthAll);
        // hidden items do not count for the minimum width either
        QCOMPARE(flow->minimumSize().width(), qMax(buttons[0]->minimumSizeHint().width(), buttons[4]->minimumSizeHint().width()));
    }
    // WP1: the data series are a page of the list (default) and can be pinned beside the pages; toggling
    // must keep them visible, never show a window, and the choice must stick in the settings. Claude Generated.
    void dataSeriesPinToggle()
    {
        QSettings().remove(QStringLiteral("model"));
        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        QGroupBox* series = widget.findChild<QGroupBox*>(QStringLiteral("series_box"));
        QToolButton* pin = widget.findChild<QToolButton*>(QStringLiteral("series_pin"));
        QVERIFY(series && pin);
        QVERIFY2(!pin->isChecked(), "not pinned is the default: the data series are a page of the list");
        QVERIFY2(series->isVisible(), "data series not visible as the default page");
        QVERIFY(series->isFlat());
        const QWidget* pageParent = series->parentWidget();

        WindowShowCounter counter;
        qApp->installEventFilter(&counter);
        counter.allow(&widget);
        pin->click(); // pin: the data series move beside the pages
        QCoreApplication::processEvents();
        QVERIFY(pin->isChecked());
        QVERIFY2(series->isVisible(), "data series vanished after pinning");
        QVERIFY(series->parentWidget() != pageParent);
        QVERIFY(!series->isFlat());

        pin->click(); // unpin again
        QCoreApplication::processEvents();
        qApp->removeEventFilter(&counter);
        QVERIFY(!pin->isChecked());
        QVERIFY2(series->isVisible(), "data series vanished after unpinning");
        QCOMPARE(series->parentWidget(), pageParent);
        QVERIFY2(counter.shown().isEmpty(),
            qPrintable(QStringLiteral("shown as own window while toggling: %1").arg(counter.shown().join(", "))));

        pin->click(); // leave it pinned: it must come back pinned
        QCoreApplication::processEvents();
        QVERIFY(QSettings().value(QStringLiteral("model/model %1 series pinned").arg(m_model->SFModel())).toBool());
        ModelWidget again(m_model, m_chart->addModel(m_model));
        again.resize(1100, 700);
        again.show();
        QTest::qWaitForWindowExposed(&again);
        QToolButton* pin2 = again.findChild<QToolButton*>(QStringLiteral("series_pin"));
        QVERIFY(pin2 && pin2->isChecked());
        QGroupBox* series2 = again.findChild<QGroupBox*>(QStringLiteral("series_box"));
        QVERIFY2(series2 && series2->isVisible(), "pinned data series not visible after a restart");
        QSettings().remove(QStringLiteral("model"));
    }
    // WP1b: the statistic tools are a page of the list; choosing it swaps the page list for the list of
    // analyses with a "Back" entry, and the dialog is really shown (an explicitly hidden widget stays empty
    // inside a layout). Claude Generated.
    void statisticsIsAPageOfTheList()
    {
        QSettings().remove(QStringLiteral("model"));
        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        auto visibleListTitles = [&widget](QListWidget** found = nullptr) {
            QStringList titles;
            for (QListWidget* l : widget.findChildren<QListWidget*>())
                if (l->isVisible() && l->count() > 0) {
                    if (found)
                        *found = l;
                    for (int i = 0; i < l->count(); ++i)
                        if (!l->item(i)->isHidden())
                            titles << l->item(i)->text();
                    break;
                }
            return titles;
        };
        QListWidget* list = nullptr;
        QStringList titles = visibleListTitles(&list);
        QVERIFY2(titles.contains(QStringLiteral("Data series")), qPrintable(titles.join(", ")));
        QVERIFY2(titles.contains(QStringLiteral("Statistics")), qPrintable(titles.join(", ")));
        QVERIFY2(!titles.contains(QStringLiteral("Charts")), "Charts is listed although the model has no chart");

        // the Statistic button asks for the page instead of opening a window
        WindowShowCounter counter;
        qApp->installEventFilter(&counter);
        counter.allow(&widget);
        QMetaObject::invokeMethod(&widget, "ToggleStatisticDialog");
        QCoreApplication::processEvents();
        qApp->removeEventFilter(&counter);
        QVERIFY2(counter.shown().isEmpty(),
            qPrintable(QStringLiteral("shown as own window: %1").arg(counter.shown().join(", "))));
        QVERIFY(visibleTopLevels().size() == 1);

        StatisticDialog* dialog = widget.findChild<StatisticDialog*>();
        QVERIFY(dialog);
        QVERIFY2(dialog->isVisible(), "the statistic page is empty: the dialog is hidden");
        QVERIFY(dialog->width() > 100 && dialog->height() > 100);
        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        if (!renderDir.isEmpty()) {
            QDir().mkpath(renderDir);
            widget.grab().save(QStringLiteral("%1/modelwidget_statistics_page.png").arg(renderDir));
        }

        titles = visibleListTitles(&list);
        QVERIFY2(titles.size() >= 3, qPrintable(titles.join(", ")));
        QVERIFY2(titles.first().contains(QStringLiteral("Back")), qPrintable(titles.join(", ")));
        QVERIFY2(titles.contains(QStringLiteral("Monte Carlo")), qPrintable(titles.join(", ")));

        // choosing an analysis shows it; "Back" returns to the page list
        const int mc = titles.indexOf(QStringLiteral("Monte Carlo"));
        list->setCurrentRow(mc);
        emit list->itemClicked(list->item(mc));
        QCoreApplication::processEvents();
        list->setCurrentRow(0);
        emit list->itemClicked(list->item(0));
        QCoreApplication::processEvents();
        titles = visibleListTitles();
        QVERIFY2(titles.contains(QStringLiteral("Statistics")), qPrintable(titles.join(", ")));
        QVERIFY(dialog->isVisible());
    }
    // A stored "Statistics" page must not come back as the start page: it would be shown beside the plain
    // page list instead of its own list of analyses. Claude Generated.
    void statisticsIsNeverTheStartPage()
    {
        QSettings().setValue(QStringLiteral("model/model %1 page title").arg(m_model->SFModel()), QStringLiteral("Statistics"));
        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        StatisticDialog* dialog = widget.findChild<StatisticDialog*>();
        QVERIFY(dialog);
        QVERIFY2(!dialog->isVisibleTo(&widget) || !dialog->isVisible(), "the statistic page is the start page");
        for (QListWidget* l : widget.findChildren<QListWidget*>())
            if (l->isVisible() && l->count() > 0) {
                QVERIFY2(!l->item(0)->text().contains(QStringLiteral("Back")), "sub-page list shown at start");
                QVERIFY2(l->currentItem() && l->currentItem()->text() != QStringLiteral("Statistics"), "Statistics selected at start");
                break;
            }
        QSettings().remove(QStringLiteral("model"));
    }
    // Finished analyses are listed in the Statistics list next to the analyses to start, can be looked at, and
    // a result can be taken out of the pane into a window of its own; closing that window puts it back. Claude Generated.
    void finishedResultsAreListedAndDetachable()
    {
        QSettings().remove(QStringLiteral("model"));
        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        const QString resultTitle = QStringLiteral("Monte Carlo #1"); // short name in the list
        const QString tabName = QStringLiteral("%1 # 1").arg(SupraFit::Method2Name(SupraFit::Method::MonteCarlo)); // the window's
        QJsonObject job = MonteCarloConfigBlock;
        job["MaxSteps"] = 100;
        JobManager jobs;
        jobs.setModel(m_model);
        jobs.AddSingleJob(job);
        jobs.RunJobs();
        QVERIFY2(m_model->getMCStatisticResult() >= 1, "the Monte Carlo run left no result");
        QCoreApplication::processEvents();

        QMetaObject::invokeMethod(&widget, "TogglePlot"); // the Results button
        QCoreApplication::processEvents();

        QListWidget* list = nullptr;
        for (QListWidget* l : widget.findChildren<QListWidget*>())
            if (l->isVisible() && l->count() > 0) {
                list = l;
                break;
            }
        QVERIFY(list);
        QStringList titles;
        for (int i = 0; i < list->count(); ++i)
            titles << list->item(i)->text();
        QVERIFY2(titles.contains(QStringLiteral("Monte Carlo")), qPrintable(titles.join(", "))); // to start
        QVERIFY2(titles.contains(QStringLiteral("Results")), qPrintable(titles.join(", ")));
        QVERIFY2(titles.contains(resultTitle), qPrintable(titles.join(", "))); // finished
        QCOMPARE(list->currentItem()->text(), resultTitle);

        // the heading is no choice
        const int heading = titles.indexOf(QStringLiteral("Results"));
        QVERIFY(!(list->item(heading)->flags() & Qt::ItemIsSelectable));

        ResultsDialog* results = widget.findChild<ResultsDialog*>();
        QVERIFY(results);
        QVERIFY2(results->isVisible(), "the result is not shown on the page");
        QTabWidget* resultTabs = results->findChild<QTabWidget*>();
        QVERIFY(resultTabs);
        QVERIFY2(!resultTabs->tabBar()->isVisible(), "the result tab bar repeats what the list already shows");
        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        if (!renderDir.isEmpty())
            widget.grab().save(QStringLiteral("%1/modelwidget_results_page.png").arg(renderDir));
        const int windowsBefore = visibleTopLevels().size();

        // take the result out of the pane
        QVERIFY(results->DetachEntry(SupraFit::Method::MonteCarlo, 0));
        QCoreApplication::processEvents();
        QList<QWidget*> windows = visibleTopLevels();
        QCOMPARE(windows.size(), windowsBefore + 1);
        QWidget* external = nullptr;
        for (QWidget* w : windows)
            if (w != &widget)
                external = w;
        QVERIFY(external);
        QVERIFY2(external->windowTitle().contains(tabName), qPrintable(external->windowTitle()));
        QVERIFY(results->DetachEntry(SupraFit::Method::MonteCarlo, 0)); // again: raises, no second window
        QCOMPARE(visibleTopLevels().size(), windowsBefore + 1);

        // closing it puts the result back as a tab
        external->close();
        QCoreApplication::processEvents();
        QCOMPARE(visibleTopLevels().size(), windowsBefore);
        QVERIFY(results->isVisible());
        QVERIFY2(results->ShowEntry(SupraFit::Method::MonteCarlo, 0), "the result is gone after closing its window");

        m_model->RemoveStatistic(SupraFit::Method::MonteCarlo, 0);
        QSettings().remove(QStringLiteral("model"));
    }
    // WP5: the More/Less switch takes the height from its content (a wrapped FlowLayout included), lifts the
    // height limit when open so a later wrap is never clipped, and remembers the choice. Claude Generated.
    void moreLessToggleUsesItsContentHeight()
    {
        QSettings().remove(QStringLiteral("ui"));
        auto makeTarget = []() {
            QWidget* target = new QWidget;
            FlowLayout* flow = new FlowLayout(target, 0, 6, 4);
            for (int i = 0; i < 20; ++i)
                flow->addWidget(new QPushButton(QStringLiteral("button %1").arg(i), target));
            return target;
        };

        QWidget host;
        QVBoxLayout* layout = new QVBoxLayout(&host);
        QWidget* target = makeTarget();
        MoreLessToggle* toggle = new MoreLessToggle(target, QStringLiteral("test_layout"));
        layout->addWidget(toggle);
        layout->addWidget(target);
        layout->addStretch();
        host.resize(300, 500);
        host.show();
        QTest::qWaitForWindowExposed(&host);
        QCoreApplication::processEvents();

        QVERIFY(!toggle->isExpanded());
        QCOMPARE(toggle->text(), QStringLiteral("..more.."));
        QCOMPARE(target->maximumHeight(), 0);

        toggle->click();
        QTRY_COMPARE(target->maximumHeight(), QWIDGETSIZE_MAX); // the animation ends by lifting the limit
        QCOMPARE(toggle->text(), QStringLiteral("..less.."));
        const FlowLayout* flow = dynamic_cast<FlowLayout*>(target->layout());
        QVERIFY(flow);
        const int needed = flow->heightForWidth(target->width());
        QVERIFY2(needed > 60, "the buttons are expected to wrap at this width");
        QVERIFY2(target->height() >= needed, qPrintable(QStringLiteral("clipped: %1 < %2").arg(target->height()).arg(needed)));
        QVERIFY(QSettings().value(QStringLiteral("ui/more/test_layout")).toBool());

        // narrowing the host later must not clip the now taller content
        host.resize(160, 800);
        QCoreApplication::processEvents();
        QVERIFY(target->height() >= flow->heightForWidth(target->width()));

        // a second switch with the same id starts open
        QWidget host2;
        QVBoxLayout* layout2 = new QVBoxLayout(&host2);
        QWidget* target2 = makeTarget();
        MoreLessToggle* toggle2 = new MoreLessToggle(target2, QStringLiteral("test_layout"));
        layout2->addWidget(toggle2);
        layout2->addWidget(target2);
        QVERIFY(toggle2->isExpanded());
        QCOMPARE(target2->maximumHeight(), QWIDGETSIZE_MAX);

        toggle->click(); // and closes again
        QTRY_COMPARE(target->maximumHeight(), 0);
        QVERIFY(!QSettings().value(QStringLiteral("ui/more/test_layout")).toBool());
        QSettings().remove(QStringLiteral("ui"));
    }
};

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    // never touch the real SupraFit settings: the pane stores page and pin choices there
    QTemporaryDir settingsDir;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsDir.path());
    QCoreApplication::setOrganizationName(QStringLiteral("SupraFitLayoutTest"));
    QCoreApplication::setApplicationName(QStringLiteral("test_modelwidget_layout"));
    TestModelWidgetLayout test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_modelwidget_layout.moc"
