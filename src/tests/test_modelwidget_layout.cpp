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
#include <QtCore/QElapsedTimer>
#include <functional>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtCore/QJsonObject>
#include <QtCore/QThread>
#include <QtGui/QPixmap>
#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QTabBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>

#include <cmath>

#include <Eigen/Dense>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/core/models/scriptmodel.h"
#include "src/global.h"

#include "src/capabilities/jobmanager.h"
#include "src/core/minimizer.h"
#include "src/ui/dialogs/resultsdialog.h"
#include "src/ui/dialogs/statisticdialog.h"
#include "src/ui/guitools/appearance.h"
#include "src/ui/mainwindow/metamodelwidget.h"
#include "src/ui/guitools/sectionstack.h"
#include "src/ui/guitools/flowlayout.h"
#include "src/core/models/titrations/nmr/nmr_any_Model.h"
#include "src/core/models/titrations/fluorescence/fl_any_Model.h"
#include "src/core/jsonhandler.h"
#include "src/core/models/meta_model.h"
#include "src/ui/dialogs/advancedsearch.h"
#include <QtWidgets/QSystemTrayIcon>
#include "src/ui/widgets/messagedock.h"
#include "src/ui/widgets/modelelement.h"
#include "src/ui/widgets/scanwidget.h"
#include "src/ui/dialogs/genericwidgetdialog.h"
#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/mainwindow/addmodelbar.h"
#include "src/ui/mainwindow/quickbardialog.h"
#include "src/ui/mainwindow/suprafitgui.h"
#include "src/ui/mainwindow/modelcatalog.h"
#include "src/ui/mainwindow/modeldataholder.h"
#include "src/ui/mainwindow/modelusage.h"
#include "src/ui/widgets/analysisparts.h"
#include "src/ui/widgets/exportsimulationwidget.h"
#include "src/ui/widgets/modelactions.h"
#include "src/ui/widgets/preparewidget.h"
#include "src/ui/guitools/morelesstoggle.h"
#include "src/ui/guitools/windowhost.h"
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

// A scripted model that lets the test report charts the way the titration models do (the chart functions of
// AbstractModel are protected).
class ChartingModel : public ScriptModel {
public:
    explicit ChartingModel(DataClass* data)
        : ScriptModel(data)
    {
    }
    void feed(const QString& chart, const QStringList& names)
    {
        Vector row(names.size());
        for (int i = 0; i < names.size(); ++i)
            row(i) = i + 1.0;
        addPoints(chart, 0.1, row, names);
        emit ChartUpdated(chart);
    }
};


// Widgets of a dialog that lie on top of each other (the label "Choose parameter to be tested:" sat on a check box of the Model
// Comparison). Only what is on screen counts: what a folded section or a scroll area clips away has an empty visible region.
static QStringList overlappingWidgets(QWidget* root)
{
    QList<QWidget*> leaves;
    for (QWidget* w : root->findChildren<QWidget*>()) {
        if (!w->isVisibleTo(root) || w->visibleRegion().isEmpty())
            continue;
        const bool leaf = qobject_cast<QLabel*>(w) || qobject_cast<QAbstractButton*>(w) || qobject_cast<QAbstractSpinBox*>(w) || qobject_cast<QComboBox*>(w);
        const QWidget* parent = w->parentWidget();
        if (leaf && !qobject_cast<const QAbstractSpinBox*>(parent) && !qobject_cast<const QComboBox*>(parent))
            leaves << w;
    }
    auto text = [](QWidget* w) {
        if (auto* l = qobject_cast<QLabel*>(w))
            return QStringLiteral("label \"%1\"").arg(l->text().left(40));
        if (auto* b = qobject_cast<QAbstractButton*>(w))
            return QStringLiteral("button \"%1\"").arg(b->text().left(40));
        return QString::fromLatin1(w->metaObject()->className());
    };
    QStringList found;
    for (int a = 0; a < leaves.size(); ++a)
        for (int b = a + 1; b < leaves.size(); ++b) {
            if (leaves[a]->isAncestorOf(leaves[b]) || leaves[b]->isAncestorOf(leaves[a]))
                continue;
            const QRect ra(leaves[a]->mapTo(root, QPoint(0, 0)), leaves[a]->size()), rb(leaves[b]->mapTo(root, QPoint(0, 0)), leaves[b]->size());
            const QRect common = ra.intersected(rb);
            if (common.width() > 2 && common.height() > 2)
                found << QStringLiteral("%1 over %2").arg(text(leaves[a]), text(leaves[b]));
        }
    return found;
}

// A fitted two-constant model on the data of the fixture, for the tests of the analysis dialogs.
static QSharedPointer<AbstractModel> twoConstantModel(DataClass* data)
{
    QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data);
    QJsonObject reactions, definition;
    reactions["value"] = QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2");
    definition["Reactions"] = reactions;
    model->DefineModel(definition);
    QJsonObject cfg = model->getOptimizerConfig();
    cfg["FitSolver"] = QStringLiteral("VarPro");
    model->setOptimizerConfig(cfg);
    model->InitialGuess();
    model->setGlobalParameter(3.0, 0);
    model->setGlobalParameter(5.4, 1);
    Minimizer minimizer(false);
    minimizer.setModel(model);
    minimizer.Minimize();
    model->Calculate();
    return model;
}

class TestModelWidgetLayout : public QObject {
    Q_OBJECT

private:
    static constexpr int N = 15;

    // Host/guest titration data (constant host, guest titrated in) with `series` zeroed dependent columns.
    static DataClass* makeData(int series)
    {
        Eigen::MatrixXd indep(N, 2);
        Eigen::MatrixXd dep(N, series);
        for (int i = 0; i < N; ++i) {
            indep(i, 0) = 1e-3; // host total
            indep(i, 1) = 3e-3 * i / (N - 1); // guest total
            for (int j = 0; j < series; ++j)
                dep(i, j) = 0.0;
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

    // Titration data with `inputs` independent columns (host, guest, ...) and two series.
    static DataClass* makeDataFor(int inputs)
    {
        Eigen::MatrixXd indep(N, qMax(1, inputs));
        Eigen::MatrixXd dep(N, 2);
        for (int i = 0; i < N; ++i) {
            indep(i, 0) = 1e-3;
            for (int c = 1; c < indep.cols(); ++c)
                indep(i, c) = 3e-3 * i / (N - 1) * c;
            dep(i, 0) = dep(i, 1) = 0.0;
        }
        DataClass* data = new DataClass();
        data->setIndependentTable(new DataTable(indep));
        data->setDataType(DataClassPrivate::Table);
        data->setSimulateDependent(2);
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

        // The model is built from a preset, like the creation dialog does: its definition block carries the full
        // field descriptors (name, type, once, ...) that the Model Definition page is made of. A bare
        // {"value": ...} block would leave that page empty.
        m_data = QSharedPointer<DataClass>(makeData(2));
        m_model = CreateModel(SupraFit::ScriptModel, m_data.data());
        QVERIFY(!m_model.isNull());
        const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
        int preset = -1;
        for (int i = 0; i < presets.size(); ++i)
            if (presets[i].name.startsWith(QStringLiteral("Binding 1:1 \u2014 signal")))
                preset = i;
        QVERIFY2(preset >= 0, "1:1 signal preset not found");
        QHash<QString, QJsonObject> elements;
        for (const QJsonObject& descriptor : presets[preset].block)
            elements[descriptor["name"].toString()] = descriptor;
        m_model->setModelDefinition(elements);
        QVERIFY(m_model->DefineModel(QJsonObject()));
        m_model->setGlobalParameter(4.0, 0); // lg K
        for (int series = 0; series < 2; ++series) {
            m_model->setLocalParameter(100.0 + 20 * series, 0, series);
            m_model->setLocalParameter(5000.0 - 500 * series, 1, series);
        }
        m_model->Calculate();
        // measured signal = the model plus a small deterministic wobble, so fits and Monte Carlo have a spread
        for (int i = 0; i < N; ++i)
            for (int series = 0; series < 2; ++series)
                m_data->DependentModel()->setData(m_data->DependentModel()->index(i, series),
                    m_model->ModelTable()->data(i, series) * (1.0 + 0.002 * std::sin(1.7 * i + series)));
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
    // Landscape sampling is an analysis of the Statistics page: a tab to start it (only where the model can do it), a
    // finished run in the list of results, and the result page with the intervals and the profile chart.
    void landscapeSamplingIsStartedListedAndShown()
    {
        QSettings().remove(QStringLiteral("model"));

        // a model without variable projection: the tab is there, the button is not usable
        {
            Charts charts = m_chart->addModel(m_model);
            ModelWidget widget(m_model, charts);
            widget.resize(1100, 700);
            widget.show();
            QTest::qWaitForWindowExposed(&widget);
            StatisticDialog* dialog = widget.findChild<StatisticDialog*>();
            QVERIFY(dialog);
            QVERIFY(dialog->TabTitles().contains(QStringLiteral("Landscape Sampling")));
            QVERIFY(!m_model->SupportsVarPro());
            QPushButton* sample = nullptr;
            for (QPushButton* b : dialog->findChildren<QPushButton*>())
                if (b->text() == QStringLiteral("Sample"))
                    sample = b;
            QVERIFY(sample);
            QVERIFY2(!sample->isEnabled(), "a model without variable projection must not offer to sample");
        }

        // a fitted reaction model: sample, list, show
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, m_data.data());
        QJsonObject reactions, definition;
        reactions["value"] = QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2");
        definition["Reactions"] = reactions;
        model->DefineModel(definition);
        QJsonObject cfg = model->getOptimizerConfig();
        cfg["FitSolver"] = QStringLiteral("VarPro");
        model->setOptimizerConfig(cfg);
        model->InitialGuess();
        model->setGlobalParameter(3.0, 0);
        model->setGlobalParameter(5.4, 1);
        Minimizer minimizer(false);
        minimizer.setModel(model);
        minimizer.Minimize();
        model->Calculate();
        QVERIFY(model->SupportsVarPro());
        QCOMPARE(model->GlobalParameterSize(), 2);

        Charts charts = m_chart->addModel(model);
        ModelWidget widget(model, charts);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        StatisticDialog* dialog = widget.findChild<StatisticDialog*>();
        QVERIFY(dialog);
        QPushButton* sample = nullptr;
        for (QPushButton* b : dialog->findChildren<QPushButton*>())
            if (b->text() == QStringLiteral("Sample"))
                sample = b;
        QVERIFY(sample);
        QVERIFY(sample->isEnabled());
        {   // the settings page as the user sees it: a few fields, the rest behind "..more..", Langevin's only for Langevin
            QSettings().remove(QStringLiteral("ui/more/landscape_more"));
            QMetaObject::invokeMethod(&widget, "TogglePlot"); // the Statistics page (the Results button) ...
            QCoreApplication::processEvents();
            dialog->setCurrentTab(dialog->TabTitles().indexOf(QStringLiteral("Landscape Sampling"))); // ... at the analysis to start
            QCoreApplication::processEvents();
            QCheckBox* surfaces = nullptr;
            QToolButton* more = nullptr;
            QCheckBox* correction = nullptr;
            for (QCheckBox* c : dialog->findChildren<QCheckBox*>()) {
                if (c->text() == QStringLiteral("2D surfaces of the pairs"))
                    surfaces = c;
                if (c->text() == QStringLiteral("Metropolis correction"))
                    correction = c;
            }
            for (QToolButton* b : dialog->findChildren<QToolButton*>())
                if (b->text() == QStringLiteral("..more.."))
                    more = b;
            QWidget* folded = dialog->findChild<QWidget*>(QStringLiteral("landscape_more"));
            QVERIFY(surfaces && more && correction && folded);
            QVERIFY2(surfaces->isChecked() && surfaces->isEnabled(), "the pair surfaces are on for two constants");
            QCOMPARE(folded->maximumHeight(), 0); // folded away
            QVERIFY2(!correction->isVisibleTo(dialog), "Langevin's settings are shown with Metropolis chosen");
            const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
            if (!renderDir.isEmpty())
                widget.grab().save(QStringLiteral("%1/statistic_landscape_tab.png").arg(renderDir));
            more->click();
            QCoreApplication::processEvents();
            QTRY_COMPARE_WITH_TIMEOUT(folded->maximumHeight(), QWIDGETSIZE_MAX, 3000); // the fold animation has finished
            QComboBox* sampler = nullptr;
            for (QComboBox* c : dialog->findChildren<QComboBox*>())
                if (c->findText(QStringLiteral("Langevin")) >= 0)
                    sampler = c;
            QVERIFY(sampler);
            sampler->setCurrentIndex(sampler->findText(QStringLiteral("Langevin")));
            QCoreApplication::processEvents();
            QVERIFY2(correction->isVisibleTo(dialog), "Langevin's settings are not shown with Langevin chosen");
            if (!renderDir.isEmpty())
                widget.grab().save(QStringLiteral("%1/statistic_landscape_tab_more.png").arg(renderDir));
            sampler->setCurrentIndex(0);
            // the task: a search asks for a wall and not for a confidence
            QComboBox* task = nullptr;
            for (QComboBox* c : dialog->findChildren<QComboBox*>())
                if (c->findText(QStringLiteral("Find other minima")) >= 0)
                    task = c;
            QVERIFY2(task, "no choice of the task");
            QCOMPARE(dialog->RunLandscape()["Mode"].toString(), QStringLiteral("Confidence"));
            task->setCurrentIndex(task->findText(QStringLiteral("Find other minima")));
            QCoreApplication::processEvents();
            const QJsonObject searching = dialog->RunLandscape();
            QCOMPARE(searching["Mode"].toString(), QStringLiteral("Minima"));
            QCOMPARE(searching["SearchWall"].toDouble(), 10.0);
            QCOMPARE(searching["Polish"].toInt(), 10);
            for (QLabel* l : dialog->findChildren<QLabel*>()) {
                if (l->text() == QStringLiteral("Wall (x SSE of the fit):"))
                    QVERIFY(l->isVisibleTo(dialog));
                if (l->text() == QStringLiteral("Confidence:") || l->text() == QStringLiteral("Wall height:"))
                    QVERIFY2(!l->isVisibleTo(dialog), qPrintable(l->text() + " is shown for a search"));
            }
            if (!renderDir.isEmpty())
                widget.grab().save(QStringLiteral("%1/statistic_landscape_tab_search.png").arg(renderDir));
            task->setCurrentIndex(0);
            QCoreApplication::processEvents();
            QCOMPARE(dialog->RunLandscape()["Mode"].toString(), QStringLiteral("Confidence"));
            more->click(); // fold again, the state is a setting
            QTRY_COMPARE(folded->maximumHeight(), 0);
            QSettings().remove(QStringLiteral("ui/more/landscape_more"));
        }
        const QJsonObject controller = dialog->RunLandscape();
        QCOMPARE(AccessCI(controller, "Method").toInt(), int(SupraFit::Method::LandscapeSampling));

        QJsonObject job = LandscapeConfigBlock;
        job["Walkers"] = 2;
        job["MaxSteps"] = 20000; // the height lines need whole 2x2 blocks of visited bins: a short walk draws none
        job["Polish"] = 2;
        job["Surface"] = true;
        JobManager jobs;
        jobs.setModel(model);
        jobs.AddSingleJob(job);
        jobs.RunJobs();
        QVERIFY2(model->LandscapeSize() >= 1, "the sampling left no result");
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
        QVERIFY2(titles.contains(QStringLiteral("Landscape Sampling")), qPrintable(titles.join(", "))); // to start
        QVERIFY2(titles.contains(QStringLiteral("Landscape Sampling #1")), qPrintable(titles.join(", "))); // finished

        ResultsDialog* results = widget.findChild<ResultsDialog*>();
        QVERIFY(results);
        QVERIFY2(results->isVisible(), "the result is not shown on the page");
        bool text = false;
        for (QLabel* l : results->findChildren<QLabel*>())
            if (l->isVisible() && l->text().contains(QStringLiteral("Landscape sampling")) && l->text().contains(QStringLiteral("<table>")))
                text = true;
        QVERIFY2(text, "the intervals are not written on the result page");

        // one chart, one choice above it: all profiles, one per constant, one surface per pair
        QWidget* chart = nullptr;
        for (QWidget* w : results->findChildren<QWidget*>())
            if (w->isVisible() && w->objectName() == QStringLiteral("landscapechart"))
                chart = w;
        QVERIFY2(chart, "no chart on the result page");
        int chartCount = 0;
        for (QWidget* w : results->findChildren<QWidget*>())
            if (w->objectName() == QStringLiteral("landscapechart") || w->objectName() == QStringLiteral("landscapesurface"))
                ++chartCount;
        QCOMPARE(chartCount, 1); // not two plots over each other
        QComboBox* choice = results->findChild<QComboBox*>(QStringLiteral("landscapeview"));
        QVERIFY2(choice, "no choice above the chart of a two-constant result");
        QStringList entries;
        for (int i = 0; i < choice->count(); ++i)
            entries << choice->itemText(i);
        QCOMPARE(choice->count(), 4); // Profiles, Profile: A, Profile: B, Surface: A x B
        QVERIFY2(entries[0] == QStringLiteral("Profiles") && entries[1].startsWith(QStringLiteral("Profile: ")) && entries.last().startsWith(QStringLiteral("Surface: ")),
            qPrintable(entries.join(" | ")));
        auto listRows = [chart]() {
            int rows = 0;
            for (QListWidget* l : chart->findChildren<QListWidget*>())
                rows = qMax(rows, l->count());
            return rows;
        };
        QCOMPARE(listRows(), 3); // the two profiles and the limit of the confidence region
        choice->setCurrentIndex(1); // one profile: the curve and the limit
        QCoreApplication::processEvents();
        QCOMPARE(listRows(), 2);
        choice->setCurrentIndex(3); // the surface: height lines and the fit
        QCoreApplication::processEvents();
        QVERIFY2(listRows() >= 2, qPrintable(QString("%1 entries for the surface").arg(listRows()))); // height lines and the fit
        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        if (!renderDir.isEmpty())
            widget.grab().save(QStringLiteral("%1/modelwidget_landscape_surface.png").arg(renderDir));
        choice->setCurrentIndex(0);
        QCoreApplication::processEvents();
        if (!renderDir.isEmpty())
            widget.grab().save(QStringLiteral("%1/modelwidget_landscape_page.png").arg(renderDir));

        model->RemoveStatistic(SupraFit::Method::LandscapeSampling, 0);
        QCOMPARE(model->LandscapeSize(), 0);
    }

    // A result with three constants offers every pair: the choice lists Profiles, a profile per constant and a surface per pair,
    // one chart changes with it, and the height lines of a correlated ellipse are drawn closed. The result is synthetic (an
    // exact quadratic form), so what is drawn can be compared with what is known.
    void landscapeResultOffersEveryPair()
    {
        QSettings().remove(QStringLiteral("model"));
        const QStringList names = { QStringLiteral("lg K1"), QStringLiteral("lg K2"), QStringLiteral("lg K3") };
        const double centre[3] = { 3.0, 4.5, 5.5 };
        QJsonArray variables, surfaces;
        for (int i = 0; i < 3; ++i) {
            QJsonObject v;
            v["index"] = i;
            v["name"] = names[i];
            v["value"] = centre[i];
            v["box_lower"] = centre[i] - 1.5;
            v["box_upper"] = centre[i] + 1.5;
            QJsonArray profile;
            for (int k = 0; k < 60; ++k) {
                const double x = centre[i] - 1.5 + (k + 0.5) * 3.0 / 60;
                profile << QJsonArray({ x, 1.0 + 4.0 * (x - centre[i]) * (x - centre[i]) });
            }
            v["profile"] = profile;
            variables << v;
        }
        const int pairs[3][2] = { { 0, 1 }, { 0, 2 }, { 1, 2 } };
        for (const auto& pair : pairs) {
            QJsonArray points;
            for (int a = 0; a < 60; ++a)
                for (int b = 0; b < 60; ++b) {
                    const double x = centre[pair[0]] - 1.5 + (a + 0.5) * 3.0 / 60, y = centre[pair[1]] - 1.5 + (b + 0.5) * 3.0 / 60;
                    const double dx = x - centre[pair[0]], dy = y - centre[pair[1]];
                    const double sse = 1.0 + 6.0 * dx * dx + 6.0 * dy * dy - 5.0 * dx * dy; // a tilted ellipse
                    if (sse <= 2.6) // what the walkers could reach below the wall
                        points << QJsonArray({ x, y, sse });
                }
            QJsonObject s;
            s["x"] = pair[0];
            s["y"] = pair[1];
            s["bins"] = 60;
            s["points"] = points;
            surfaces << s;
        }
        QJsonObject result;
        result["variables"] = variables;
        result["surfaces"] = surfaces;
        result["sse_min"] = 1.0;
        result["level"] = 1.3;
        result["wall"] = 2.6;
        result["contour_max"] = 2.6;
        result["confidence"] = 95.0;
        QJsonObject settings;
        settings["bins"] = 60;
        result["settings"] = settings;
        QJsonObject controller;
        controller["Method"] = int(SupraFit::Method::LandscapeSampling);
        controller["timestamp"] = 1234567.0;
        controller["title"] = QStringLiteral("synthetic");
        QJsonObject statistic;
        statistic["controller"] = controller;
        statistic["0"] = result;
        m_model->UpdateStatistic(statistic);

        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();
        ResultsDialog* results = widget.findChild<ResultsDialog*>();
        QVERIFY(results);
        QMetaObject::invokeMethod(&widget, "TogglePlot"); // the Results button: the page of the newest result
        QCoreApplication::processEvents();
        QVERIFY(results->ShowEntry(SupraFit::Method::LandscapeSampling, 0));
        QCoreApplication::processEvents();
        QVERIFY2(results->isVisible(), "the result is not shown on the page");

        QComboBox* choice = results->findChild<QComboBox*>(QStringLiteral("landscapeview"));
        QVERIFY2(choice, "no choice above the chart");
        QStringList entries;
        for (int i = 0; i < choice->count(); ++i)
            entries << choice->itemText(i);
        QCOMPARE(entries.size(), 7); // Profiles, three profiles, three surfaces
        QCOMPARE(entries[0], QStringLiteral("Profiles"));
        QCOMPARE(entries[4], QStringLiteral("Surface: lg K1 \u00d7 lg K2"));
        QCOMPARE(entries[5], QStringLiteral("Surface: lg K1 \u00d7 lg K3"));
        QCOMPARE(entries[6], QStringLiteral("Surface: lg K2 \u00d7 lg K3"));

        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        auto rows = [&widget]() {
            int count = 0;
            for (QWidget* w : widget.findChildren<QWidget*>(QStringLiteral("landscapechart")))
                for (QListWidget* l : w->findChildren<QListWidget*>())
                    count = qMax(count, l->count());
            return count;
        };
        QCOMPARE(rows(), 4); // three profiles and the limit
        for (int s = 4; s < 7; ++s) {
            choice->setCurrentIndex(s);
            QCoreApplication::processEvents();
            // the level lines are the limit (1.3) and ten steps up to 2.6: all of them exist in this ellipse, plus the fit
            QVERIFY2(rows() >= 8, qPrintable(QString("surface %1: %2 entries").arg(s).arg(rows())));
            if (!renderDir.isEmpty() && s == 4)
                widget.grab().save(QStringLiteral("%1/modelwidget_landscape_pair.png").arg(renderDir));
        }
        m_model->RemoveStatistic(SupraFit::Method::LandscapeSampling, 0);
    }

    // The result of a minima search lists its minima for the project: the choice shows them, "Load into model" and "Add as model" hand
    // the chosen minimum's model over (the signals the Monte Carlo and global-search results use), the task of the settings page
    // sets the mode of the job.
    void landscapeMinimaCanBeLoaded()
    {
        QSettings().remove(QStringLiteral("model"));
        QJsonObject variable;
        variable["index"] = 0;
        variable["name"] = QStringLiteral("lgK");
        variable["value"] = 4.0;
        variable["box_lower"] = -2.0;
        variable["box_upper"] = 10.0;
        QJsonArray profile;
        for (int k = 0; k < 60; ++k) {
            const double x = -2.0 + (k + 0.5) * 12.0 / 60;
            profile << QJsonArray({ x, 1.0 + 0.2 * (x - 4.0) * (x - 4.0) });
        }
        variable["profile"] = profile;

        const QJsonObject exported = m_model->ExportModel(false, false);
        QVERIFY(!exported.isEmpty());
        QJsonArray minima;
        const double sse[3] = { 0.9, 1.0, 3.5 };
        const double at[3] = { 5.5, 4.0, 1.0 };
        for (int k = 0; k < 3; ++k) {
            QJsonObject m;
            m["sse"] = sse[k];
            m["relative_sse"] = sse[k];
            m["within_level"] = k < 2;
            m["is_fit"] = k == 1;
            m["parameters"] = QJsonArray({ at[k] });
            m["model"] = exported;
            minima << m;
        }
        QJsonObject result;
        result["mode"] = QStringLiteral("Minima");
        result["variables"] = QJsonArray({ variable });
        result["minima"] = minima;
        result["sse_min"] = 1.0;
        result["level"] = 1.3;
        result["wall"] = 10.0;
        result["contour_max"] = 10.0;
        result["contour_log"] = true;
        result["confidence"] = 95.0;
        QJsonObject controller;
        controller["Method"] = int(SupraFit::Method::LandscapeSampling);
        controller["timestamp"] = 7654321.0;
        controller["title"] = QStringLiteral("synthetic minima");
        QJsonObject statistic;
        statistic["controller"] = controller;
        statistic["0"] = result;
        m_model->UpdateStatistic(statistic);

        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        ResultsDialog* results = widget.findChild<ResultsDialog*>();
        QVERIFY(results);
        QSignalSpy loaded(results, &ResultsDialog::LoadModel), added(results, &ResultsDialog::AddModel);
        QMetaObject::invokeMethod(&widget, "TogglePlot");
        QCoreApplication::processEvents();
        QVERIFY(results->ShowEntry(SupraFit::Method::LandscapeSampling, 0));
        QCoreApplication::processEvents();

        QComboBox* which = results->findChild<QComboBox*>(QStringLiteral("landscapeminima"));
        QVERIFY2(which, "no list of the minima on the result page");
        QCOMPARE(which->count(), 3);
        QVERIFY2(which->itemText(0).contains(QStringLiteral("deeper than the fit")), qPrintable(which->itemText(0)));
        QVERIFY2(which->itemText(1).contains(QStringLiteral("contains the fit")), qPrintable(which->itemText(1)));
        QPushButton* load = results->findChild<QPushButton*>(QStringLiteral("landscapeminima_load"));
        QPushButton* add = results->findChild<QPushButton*>(QStringLiteral("landscapeminima_add"));
        QVERIFY(load && add);
        // one constant: no choice of what to show, only the minima row
        QVERIFY(!results->findChild<QComboBox*>(QStringLiteral("landscapeview")));
        which->setCurrentIndex(2);
        load->click();
        add->click();
        QCOMPARE(loaded.count(), 1);
        QCOMPARE(added.count(), 1);
        QVERIFY2(!loaded.first().first().toJsonObject().isEmpty(), "Load handed over an empty model");
        QCOMPARE(loaded.first().first().toJsonObject(), exported);
        QCOMPARE(added.first().first().toJsonObject(), exported);

        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        if (!renderDir.isEmpty())
            widget.grab().save(QStringLiteral("%1/modelwidget_landscape_minima.png").arg(renderDir));
        m_model->RemoveStatistic(SupraFit::Method::LandscapeSampling, 0);
    }

    // A landscape job through the real pane: the progress dialog shows what happens (the stage text, a bar that counts and ends at its
    // maximum), the log gets one line for the whole analysis and none for the refits, and the stage texts are not messages.
    void landscapeJobShowsItsStagesAndStaysQuiet()
    {
        QSettings().remove(QStringLiteral("model"));
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, m_data.data());
        QJsonObject reactions, definition;
        reactions["value"] = QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2");
        definition["Reactions"] = reactions;
        model->DefineModel(definition);
        QJsonObject cfg = model->getOptimizerConfig();
        cfg["FitSolver"] = QStringLiteral("VarPro");
        model->setOptimizerConfig(cfg);
        model->InitialGuess();
        model->setGlobalParameter(3.0, 0);
        model->setGlobalParameter(5.4, 1);
        Minimizer minimizer(false);
        minimizer.setModel(model);
        minimizer.Minimize();
        model->Calculate();

        Charts charts = m_chart->addModel(model);
        ModelWidget widget(model, charts);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();
        StatisticDialog* dialog = widget.findChild<StatisticDialog*>();
        QVERIFY(dialog);
        QProgressBar* bar = dialog->findChild<QProgressBar*>();
        QVERIFY(bar);
        QMetaObject::invokeMethod(&widget, "TogglePlot"); // the user is on the Statistics page when the job runs
        QCoreApplication::processEvents();
        dialog->setCurrentTab(dialog->TabTitles().indexOf(QStringLiteral("Landscape Sampling")));
        QCoreApplication::processEvents();

        QSignalSpy logged(model->Info(), &DataClassPrivateObject::Message), warned(model->Info(), &DataClassPrivateObject::Warning);
        QStringList stagesSeen, labelsSeen;
        bool barWithinMaximum = true;
        int samplingMaximum = 0, samplingEnd = -1;
        QMetaObject::Connection stage = QObject::connect(widget.Jobs(), &JobManager::Stage, [&](const QString& text, bool restart) {
            // the dialog's own slot has run first (connected earlier): the label shows the text, a new stage starts the bar again
            QLabel* shown = nullptr;
            for (QLabel* l : dialog->findChildren<QLabel*>())
                if (l->text() == text)
                    shown = l;
            labelsSeen << (shown ? QStringLiteral("shown") : QStringLiteral("NOT shown: ") + text);
            if (restart) {
                stagesSeen << text;
                if (bar->value() != 0)
                    barWithinMaximum = false;
            }
        });
        QMetaObject::Connection counted = QObject::connect(widget.Jobs(), &JobManager::incremented, [&](int) {
            if (bar->value() > bar->maximum())
                barWithinMaximum = false;
            if (!stagesSeen.isEmpty() && stagesSeen.last().startsWith(QStringLiteral("Sampling"))) {
                samplingMaximum = bar->maximum();
                samplingEnd = bar->value();
                const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
                static bool rendered = false;
                if (!renderDir.isEmpty() && !rendered && bar->value() * 2 >= bar->maximum()) {
                    rendered = true; // what the user sees half way through the sampling
                    widget.grab().save(QStringLiteral("%1/modelwidget_landscape_progress.png").arg(renderDir));
                }
            }
        });

        QJsonObject job = LandscapeConfigBlock;
        job["Walkers"] = 2;
        job["MaxSteps"] = 2000;
        job["Polish"] = 3;
        job["Surface"] = true;
        widget.Jobs()->AddSingleJob(job);
        widget.Jobs()->RunJobs();
        QObject::disconnect(stage);
        QObject::disconnect(counted);

        qInfo().noquote() << "stages:" << stagesSeen.join(" | ");
        QVERIFY2(stagesSeen.size() >= 4, qPrintable(stagesSeen.join(" | ")));
        QVERIFY(stagesSeen[1].startsWith(QStringLiteral("Sampling")));
        QVERIFY2(!labelsSeen.join(",").contains(QStringLiteral("NOT shown")), qPrintable(labelsSeen.join(", ")));
        QVERIFY2(barWithinMaximum, "the bar went past its maximum or did not start again with a stage");
        QCOMPARE(samplingMaximum, 2 * ((2000 + 499) / 500));
        QCOMPARE(samplingEnd, samplingMaximum); // the sampling stage ends full
        // the log: one line for the whole analysis (the job's own summary), none of the "Optimisation took ..." lines of the refits
        QCOMPARE(logged.count(), 1);
        QVERIFY2(logged.first().first().toString().contains(QStringLiteral("took")), qPrintable(logged.first().first().toString()));
        QVERIFY2(!logged.first().first().toString().startsWith(QStringLiteral("Optimisation took")), "a refit logged its time");
        QCOMPARE(warned.count(), 0);
        QVERIFY(model->LandscapeSize() >= 1);
        model->RemoveStatistic(SupraFit::Method::LandscapeSampling, 0);
    }

    // The tabs of the analyses: nothing lies on anything else (the Model Comparison had a label over a check box), and the lists a
    // job carries are the ones the dialog always made: one flag per fitted parameter, a local parameter once for every series, the
    // box scaling for every parameter of every series.
    void analysisTabsDoNotOverlapAndMakeTheOldLists()
    {
        QSettings().remove(QStringLiteral("model"));
        {   // the check itself: two labels on top of each other are found, two side by side are not
            QWidget probe;
            QLabel *one = new QLabel(QStringLiteral("one"), &probe), *two = new QLabel(QStringLiteral("two"), &probe), *three = new QLabel(QStringLiteral("three"), &probe);
            one->setGeometry(10, 10, 100, 30);
            two->setGeometry(50, 15, 100, 30);
            three->setGeometry(10, 100, 100, 30);
            probe.resize(300, 200);
            probe.show();
            QVERIFY(QTest::qWaitForWindowExposed(&probe));
            const QStringList found = overlappingWidgets(&probe);
            QCOMPARE(found.size(), 1);
        }
        QSharedPointer<AbstractModel> model = twoConstantModel(m_data.data());
        Charts charts = m_chart->addModel(model);
        ModelWidget widget(model, charts);
        widget.resize(1100, 800);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        StatisticDialog* dialog = widget.findChild<StatisticDialog*>();
        QVERIFY(dialog);
        QMetaObject::invokeMethod(&widget, "TogglePlot");
        QCoreApplication::processEvents();
        const QStringList tabs = dialog->TabTitles();
        QVERIFY(tabs.size() >= 5);
        for (int i = 0; i < tabs.size(); ++i) {
            dialog->setCurrentTab(i);
            QCoreApplication::processEvents();
            const QStringList overlaps = overlappingWidgets(dialog);
            QVERIFY2(overlaps.isEmpty(), qPrintable(tabs[i] + ": " + overlaps.join("; ")));
            // the tabs that test parameters show one check box for each of them: 2 constants and 3 local parameters of this model
            if (tabs[i] == QStringLiteral("Weakened Grid Search") || tabs[i] == QStringLiteral("Model Comparison")) {
                int visibleBoxes = 0;
                for (QCheckBox* c : dialog->findChildren<QCheckBox*>())
                    if (c->isVisibleTo(dialog) && !c->visibleRegion().isEmpty() && c->text().isEmpty())
                        ++visibleBoxes; // the parameter boxes carry their name in a label beside them
                QCOMPARE(visibleBoxes, model->GlobalParameterSize() + model->LocalParameterSize());
            }
            const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
            if (!renderDir.isEmpty())
                widget.grab().save(QStringLiteral("%1/analysis_tab_%2.png").arg(renderDir).arg(i));
        }
        {   // the dialog of the project: no model, so no parameter to pick - a choice of the scope instead
            StatisticDialog project;
            project.resize(600, 700);
            project.show();
            QVERIFY(QTest::qWaitForWindowExposed(&project));
            const QStringList projectTabs = project.TabTitles();
            for (int i = 0; i < projectTabs.size(); ++i) {
                project.setCurrentTab(i);
                QCoreApplication::processEvents();
                const QStringList overlaps = overlappingWidgets(&project);
                QVERIFY2(overlaps.isEmpty(), qPrintable(QStringLiteral("project, ") + projectTabs[i] + ": " + overlaps.join("; ")));
                const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
                if (!renderDir.isEmpty())
                    project.grab().save(QStringLiteral("%1/analysis_project_%2.png").arg(renderDir).arg(i));
            }
        }

        const int series = model->SeriesCount(), locals = model->LocalParameterSize(), globals = model->GlobalParameterSize();
        QCOMPARE(globals, 2);
        const QJsonObject wgs = dialog->RunGridSearch();
        QCOMPARE(wgs["GlobalParameterList"].toString(), QStringLiteral("1 1")); // the globals ticked
        QStringList zeros;
        for (int k = 0; k < locals * series; ++k)
            zeros << QStringLiteral("0");
        QCOMPARE(wgs["LocalParameterList"].toString(), zeros.join(" ")); // the locals not, once per series
        QVERIFY(!wgs.contains("GlobalParameterScalingList")); // the grid search has no box
        QVERIFY(!wgs.contains("AllParameters"));
        QVERIFY(std::abs(wgs["MaxParameter"].toDouble() - model->ErrorfTestThreshold(0.95)) < 1e-12 * model->ErrorfTestThreshold(0.95));

        const QJsonObject moco = dialog->RunModelComparison();
        QCOMPARE(moco["GlobalParameterList"].toString(), QStringLiteral("1 1"));
        QCOMPARE(moco["GlobalParameterScalingList"].toString().split(' ').size(), globals);
        QCOMPARE(moco["LocalParameterScalingList"].toString().split(' ').size(), locals * series);
        QCOMPARE(moco["GlobalParameterScalingList"].toString(), QStringLiteral("1.5 1.5"));
        QVERIFY(std::abs(moco["MaxParameter"].toDouble() - model->ErrorfTestThreshold(0.95)) < 1e-12 * model->ErrorfTestThreshold(0.95));
        QCOMPARE(moco["Method"].toInt(), int(SupraFit::Method::ModelComparison));
        QCOMPARE(moco["MaxSteps"].toInt(), 20000);
    }

    // WP-S2: the noise of the Monte Carlo simulation is one choice (VarianceSource 1 user, 2 SEy, 3 sigma of the fit, 4 bootstrap);
    // the keys of the cross validation jobs stay as they were. Claude Generated.
    void monteCarloNoiseIsOneChoice()
    {
        QSharedPointer<AbstractModel> model = twoConstantModel(m_data.data());
        Charts charts = m_chart->addModel(model);
        ModelWidget widget(model, charts);
        widget.resize(1100, 800);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        StatisticDialog* dialog = widget.findChild<StatisticDialog*>();
        QVERIFY(dialog);
        QComboBox* noise = nullptr;
        for (QComboBox* c : dialog->findChildren<QComboBox*>())
            if (c->findData(4) >= 0 && c->findData(3) >= 0)
                noise = c;
        QVERIFY2(noise, "no noise choice in the Monte Carlo tab");
        const QList<QPair<int, int>> expected = { { 2, 2 }, { 3, 3 }, { 1, 1 }, { 4, 4 } };
        for (const QPair<int, int>& e : expected) {
            noise->setCurrentIndex(noise->findData(e.first));
            QCOMPARE(dialog->RunMonteCarlo()["VarianceSource"].toInt(), e.second);
        }
        const QJsonObject cv = dialog->RunCrossValidation();
        QCOMPARE(cv["Method"].toInt(), int(SupraFit::Method::CrossValidation));
        QVERIFY(cv.contains("CXO"));
        QVERIFY(dialog->RunReductionAnalyse().contains("Method"));
    }

    // The fluorescence presets (fl_any with the reactions of their stoichiometry) open in the pane like any model: no overlap, the options of
    // the corrections are offered, and the system parameters of the inner filter are there. Claude Generated.
    void fluorescencePresetOpensInThePane()
    {
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::fl_ItoI_ItoII, m_data.data());
        QVERIFY(model);
        model->InitialGuess();
        model->Calculate();
        QVERIFY(model->getAllOptions().contains(fl_any_Model::InnerFilter));
        QVERIFY(model->getAllOptions().contains(fl_any_Model::Quenching));
        QVERIFY(model->getSystemParameterList().contains(fl_any_Model::PathLength));
        QVERIFY(model->getSystemParameterList().contains(fl_any_Model::EpsilonExcitation));
        QCOMPARE(model->GlobalParameterSize(), 3); // lg beta(AB), lg beta(AB2), lg K_SV
        Charts charts = m_chart->addModel(model);
        ModelWidget widget(model, charts);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();
        const QStringList overlaps = overlappingWidgets(&widget);
        QVERIFY2(overlaps.isEmpty(), qPrintable(overlaps.join("; ")));
    }

    // A meta model in a multi-project file is rebuilt and shown: a window of its own with the models it compares (as copies, read only) and the data series
    // of the projects they belong to. Claude Generated.
    void metaModelOfAFileIsShown()
    {
        struct ReinstallInstance {
            ~ReinstallInstance() { Instance::setInstance(new Instance); }
        } reinstallInstance;
        {
            QSettings settings;
            settings.beginGroup(QStringLiteral("main"));
            settings.setValue(QStringLiteral("advanced_ui"), false);
        }
        // a file with two projects and a meta model of one model of each, made through the manager
        const QString dirPath = QDir::tempPath() + QStringLiteral("/suprafit_meta_test_%1").arg(QCoreApplication::applicationPid());
        QDir().mkpath(dirPath);
        const QString path = dirPath + QStringLiteral("/with_meta.suprafit");
        {
            SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();
            pm.clearAllProjects();
            QVERIFY(pm.loadProject(QStringLiteral(SUPRAFIT_SOURCE_DIR "/data/samples/reference/simulated_1_1.json")));
            QVERIFY(pm.loadProject(QStringLiteral(SUPRAFIT_SOURCE_DIR "/data/samples/reference/simulated_2_1_1_1.json")));
            QSharedPointer<DataClass> base = QSharedPointer<DataClass>::create();
            base->setProjectTitle(QStringLiteral("Meta for the window"));
            base->NewUUID();
            QSharedPointer<AbstractModel> created = CreateModel(SupraFit::MetaModel, base);
            ::MetaModel* meta = qobject_cast<::MetaModel*>(created.data());
            QVERIFY(meta);
            for (const QString& id : pm.getLoadedProjectIds())
                meta->addModel(qobject_cast<AbstractModel*>(pm.getProjectData(id)->Children(0).data()));
            QVERIFY(pm.registerExistingProject(created.staticCast<DataClass>()));
            QJsonObject file;
            const QStringList all = pm.getLoadedProjectIds();
            for (int i = 0; i < all.size(); ++i)
                file[QStringLiteral("project_%1").arg(i)] = pm.getProjectFileJson(all[i]);
            QVERIFY(JsonHandler::WriteJsonFile(file, path));
            pm.clearAllProjects();
        }

        QStringList dialogs;
        QTimer closer;
        closer.setInterval(100);
        QObject::connect(&closer, &QTimer::timeout, [&dialogs]() {
            if (QWidget* w = QApplication::activeModalWidget()) {
                dialogs << w->windowTitle();
                w->close();
            }
        });
        closer.start();
        SupraFitGui gui;
        gui.resize(1500, 700);
        gui.show();
        QTest::qWaitForWindowExposed(&gui);
        gui.LoadFile(path);
        QTRY_COMPARE_WITH_TIMEOUT(gui.ProjectWindows().size(), 3, 20000);
        MainWindow* metaWindow = nullptr;
        for (MainWindow* w : gui.ProjectWindows())
            if (w->isMetaModel())
                metaWindow = w;
        QVERIFY2(metaWindow, "no window of the meta model");
        QCOMPARE(metaWindow->ModelCount(), 2);
        QVERIFY(metaWindow->getChartWrapper().toStrongRef()->SeriesSize() >= 2); // the data series of both projects
        // the meta model itself has its overview (parameters, minimise, statistics), not only its models
        MetaModelWidget* overview = metaWindow->findChild<MetaModelWidget*>();
        QVERIFY2(overview, "the meta model has no overview widget");
        // its buttons work: calculate and minimise leave a finite fit
        for (const QString& label : { QStringLiteral("Calculate"), QStringLiteral("Minimize") }) {
            QPushButton* button = nullptr;
            for (QPushButton* b : overview->findChildren<QPushButton*>())
                if (b->text() == label)
                    button = b;
            QVERIFY2(button, qPrintable(label + QStringLiteral(" button missing")));
            button->click();
            QVERIFY2(std::isfinite(overview->Model()->SSE()), qPrintable(label + QStringLiteral(": SSE is not finite")));
        }
        // the Statistics page is the one of the model pane: a list with the analyses to start
        SectionStack* pages = overview->Pages();
        QVERIFY2(pages, "the meta overview has no page list");
        pages->setCurrentTitle(QStringLiteral("Statistics"), true);
        QCOMPARE(pages->currentTitle(), QStringLiteral("Statistics"));
        if (!qEnvironmentVariable("SUPRAFIT_SHOT_DIR").isEmpty()) {
            metaWindow->show();
            metaWindow->setCurrentTab(0);
            QTest::qWait(300);
            metaWindow->grab().save(qEnvironmentVariable("SUPRAFIT_SHOT_DIR") + QStringLiteral("/meta_window.png"));
        }
        QVERIFY2(dialogs.isEmpty(), qPrintable(QStringLiteral("a modal dialog came up: %1").arg(dialogs.join(", "))));
        QDir(dirPath).removeRecursively();
    }

    // A real file with meta models (SUPRAFIT_META_TEST_FILE, skipped if unset): every meta model of it gets a window with models. Claude Generated.
    void metaModelsOfARealFile()
    {
        const QString path = qEnvironmentVariable("SUPRAFIT_META_TEST_FILE");
        if (path.isEmpty())
            QSKIP("SUPRAFIT_META_TEST_FILE is not set");
        struct ReinstallInstance {
            ~ReinstallInstance() { Instance::setInstance(new Instance); }
        } reinstallInstance;
        {
            QSettings settings;
            settings.beginGroup(QStringLiteral("main"));
            settings.setValue(QStringLiteral("advanced_ui"), false);
        }
        QStringList dialogs;
        QTimer closer;
        closer.setInterval(100);
        QObject::connect(&closer, &QTimer::timeout, [&dialogs]() {
            if (QWidget* w = QApplication::activeModalWidget()) {
                dialogs << w->windowTitle();
                w->close();
            }
        });
        closer.start();
        SupraFit::ProjectManager::instance().clearAllProjects();
        SupraFitGui gui;
        gui.resize(1500, 700);
        gui.show();
        QTest::qWaitForWindowExposed(&gui);
        gui.LoadFile(path);
        QTest::qWait(3000);
        int metas = 0, models = 0;
        for (MainWindow* w : gui.ProjectWindows())
            if (w->isMetaModel()) {
                ++metas;
                models += w->ModelCount();
                qInfo().noquote() << QString("meta window '%1': %2 models").arg(w->Name()).arg(w->ModelCount());
            }
        qInfo() << "windows" << gui.ProjectWindows().size() << "meta windows" << metas << "models in them" << models << "dialogs" << dialogs;
        QVERIFY(metas >= 1);
        QVERIFY(models >= metas);
    }

    // Opening a file shows the wait cursor until the windows of the file are built (they are built by queued signals, after LoadFile has
    // returned), and gives it back afterwards, also when the file is missing or no project file at all; the report of a failure comes
    // without the wait cursor. Claude Generated.
    void cursorIsRestoredAfterALoad()
    {
        struct ReinstallInstance {
            ~ReinstallInstance() { Instance::setInstance(new Instance); }
        } reinstallInstance;
        {
            QSettings settings;
            settings.beginGroup(QStringLiteral("main"));
            settings.setValue(QStringLiteral("advanced_ui"), false);
        }
        QStringList dialogs;
        bool cursorOverDialog = false;
        QTimer closer;
        closer.setInterval(50);
        QObject::connect(&closer, &QTimer::timeout, [&dialogs, &cursorOverDialog]() {
            if (QWidget* w = QApplication::activeModalWidget()) {
                dialogs << w->windowTitle();
                cursorOverDialog = cursorOverDialog || QApplication::overrideCursor();
                w->close();
            }
        });
        closer.start();
        SupraFit::ProjectManager::instance().clearAllProjects();
        QCOMPARE(QApplication::overrideCursor(), nullptr);
        SupraFitGui gui;
        gui.resize(1200, 700);
        gui.show();
        QTest::qWaitForWindowExposed(&gui);

        gui.LoadFile(QStringLiteral(SUPRAFIT_SOURCE_DIR "/data/samples/reference/simulated_2_1_1_1.json"));
        // LoadFile has returned, the window of the project is not built yet: the cursor stays
        QVERIFY(gui.isLoading());
        QVERIFY(QApplication::overrideCursor());
        QCOMPARE(QApplication::overrideCursor()->shape(), Qt::WaitCursor);
        QTRY_VERIFY_WITH_TIMEOUT(!gui.isLoading(), 20000);
        QCOMPARE(QApplication::overrideCursor(), nullptr);
        QCOMPARE(gui.ProjectWindows().size(), 1);
        QVERIFY(gui.ProjectWindows().first()->ModelCount() >= 1);
        QVERIFY(dialogs.isEmpty());

        QTemporaryDir dir;
        const QString corrupt = dir.path() + QStringLiteral("/corrupt.suprafit");
        {
            QFile file(corrupt);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("{ this is not a project");
        }
        for (const QString& path : { dir.path() + QStringLiteral("/missing.suprafit"), corrupt }) {
            dialogs.clear();
            gui.LoadFile(path);
            QTRY_VERIFY_WITH_TIMEOUT(!gui.isLoading(), 20000);
            QCOMPARE(QApplication::overrideCursor(), nullptr);
            QTRY_VERIFY_WITH_TIMEOUT(!dialogs.isEmpty(), 5000); // the load report
            QVERIFY2(!cursorOverDialog, qPrintable(path));
            QCOMPARE(gui.ProjectWindows().size(), 1);
        }
        SupraFit::ProjectManager::instance().clearAllProjects();
    }

    // The progress of a load (status bar, "project n of m", models) comes only for a load that takes longer than its delay: a file of two
    // projects with the delay at 0 shows it, with a delay of a minute it does not; it is hidden again when the load is done. Claude Generated.
    void loadProgressOnlyForSlowLoads()
    {
        struct ReinstallInstance {
            ~ReinstallInstance() { Instance::setInstance(new Instance); }
        } reinstallInstance;
        {
            QSettings settings;
            settings.beginGroup(QStringLiteral("main"));
            settings.setValue(QStringLiteral("advanced_ui"), false);
        }
        QTemporaryDir dir;
        const QString path = dir.path() + QStringLiteral("/two_projects.suprafit");
        {
            SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();
            pm.clearAllProjects();
            QVERIFY(pm.loadProject(QStringLiteral(SUPRAFIT_SOURCE_DIR "/data/samples/reference/simulated_1_1.json")));
            QVERIFY(pm.loadProject(QStringLiteral(SUPRAFIT_SOURCE_DIR "/data/samples/reference/simulated_2_1_1_1.json")));
            QJsonObject file;
            const QStringList all = pm.getLoadedProjectIds();
            for (int i = 0; i < all.size(); ++i)
                file[QStringLiteral("project_%1").arg(i)] = pm.getProjectFileJson(all[i]);
            QVERIFY(JsonHandler::WriteJsonFile(file, path));
            pm.clearAllProjects();
        }
        QTimer closer;
        closer.setInterval(50);
        QObject::connect(&closer, &QTimer::timeout, []() {
            if (QWidget* w = QApplication::activeModalWidget())
                w->close();
        });
        closer.start();
        for (const int delay : { 0, 60000 }) {
            SupraFit::ProjectManager::instance().clearAllProjects();
            SupraFitGui gui;
            gui.resize(1200, 700);
            gui.show();
            QTest::qWaitForWindowExposed(&gui);
            gui.setLoadProgressDelay(delay);
            gui.LoadFile(path);
            QTRY_VERIFY_WITH_TIMEOUT(!gui.isLoading(), 20000);
            QCOMPARE(gui.ProjectWindows().size(), 2);
            QCOMPARE(gui.LoadProgressWasShown(), delay == 0);
            for (QProgressBar* bar : gui.findChildren<QProgressBar*>())
                if (bar->parentWidget() && qobject_cast<QStatusBar*>(bar->parentWidget()->parentWidget()))
                    QVERIFY2(!bar->isVisible(), "the progress of the load stays after it");
            QCOMPARE(QApplication::overrideCursor(), nullptr);
        }
        SupraFit::ProjectManager::instance().clearAllProjects();
    }

    // The report of a project file that could not be read completely: a dialog whose text has the room it needs (no fixed size, no scrolling for a
    // short report, resizable), and the same in the log, one line per issue. Claude Generated.
    void loadReportDialogFitsItsTextAndGoesToTheLog()
    {
        struct ReinstallInstance {
            ~ReinstallInstance() { Instance::setInstance(new Instance); }
        } reinstallInstance;
        {
            QSettings settings; // fresh settings start the first-start dialog (modal)
            settings.beginGroup(QStringLiteral("main"));
            settings.setValue(QStringLiteral("advanced_ui"), false);
        }
        SupraFitGui gui;
        gui.resize(1200, 700);
        gui.show();
        QTest::qWaitForWindowExposed(&gui);

        SupraFit::LoadReport report;
        report.file = QStringLiteral("/tmp/project.suprafit");
        report.loaded = true;
        report.modelsInFile = 4;
        report.modelsLoaded = 2;
        report.issues << SupraFit::LoadIssue{ SupraFit::LoadIssue::Warning, QStringLiteral("model_2"), QStringLiteral("Left out: model number 99999 is not known to this SupraFit.") }
                      << SupraFit::LoadIssue{ SupraFit::LoadIssue::Warning, QStringLiteral("model_3"), QStringLiteral("Left out: the entry has no \"model\" field.") }
                      << SupraFit::LoadIssue{ SupraFit::LoadIssue::Note, QStringLiteral("data"), QStringLiteral("The fits of the file leave out the first 2 data points.") };

        QSize shortSize, longSize;
        bool scrollbarOnShort = true, resizable = false, textShown = false;
        QTimer poll;
        poll.setInterval(50);
        QSize* sizeOut = nullptr;
        bool* scrollOut = nullptr;
        QObject::connect(&poll, &QTimer::timeout, [&]() {
            QDialog* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog || !sizeOut)
                return;
            QPlainTextEdit* text = dialog->findChild<QPlainTextEdit*>();
            if (!text || !dialog->isVisible())
                return;
            QCoreApplication::processEvents();
            *sizeOut = dialog->size();
            textShown = text->toPlainText().contains(QStringLiteral("99999")) && text->toPlainText().contains(QStringLiteral("first 2 data points"));
            *scrollOut = text->verticalScrollBar()->isVisible();
            resizable = dialog->minimumSize() != dialog->maximumSize() && dialog->maximumSize().height() > 5000;
            sizeOut = nullptr;
            dialog->close();
        });
        poll.start();
        auto inspect = [&](QSize* size, bool* scroll) {
            sizeOut = size;
            scrollOut = scroll;
        };
        inspect(&shortSize, &scrollbarOnShort);
        gui.ShowLoadReport(report.file, report);
        QVERIFY2(textShown, "the dialog does not show the issues");
        QVERIFY2(resizable, "the dialog has a fixed size");
        QVERIFY2(!scrollbarOnShort, "a short report needs no scrolling");

        // a long report: the dialog is not taller than the screen asks, the text scrolls
        for (int i = 0; i < 60; ++i)
            report.issues << SupraFit::LoadIssue{ SupraFit::LoadIssue::Warning, QStringLiteral("model_%1").arg(10 + i), QStringLiteral("Left out: a model that cannot be read.") };
        bool scrollbarOnLong = false;
        inspect(&longSize, &scrollbarOnLong);
        gui.ShowLoadReport(report.file, report);
        QVERIFY2(scrollbarOnLong, "a long report does not scroll");
        QVERIFY2(longSize.height() < 900, qPrintable(QStringLiteral("the dialog is %1 px high").arg(longSize.height())));
        QVERIFY(longSize.height() > shortSize.height());

        // the log has the issues, one line each (and the notes as messages)
        MessageDock* dock = gui.findChild<MessageDock*>();
        QVERIFY(dock);
        QTextEdit* log = dock->findChild<QTextEdit*>();
        QVERIFY(log);
        QVERIFY2(log->toPlainText().contains(QStringLiteral("model_2")) && log->toPlainText().contains(QStringLiteral("model_3")), qPrintable(log->toPlainText().left(400)));
        QVERIFY(log->toPlainText().contains(QStringLiteral("first 2 data points")));
    }

    // WP-S4: Monte Carlo, Grid Search, Model Comparison and Scan say what they do (a stage text, shown in the dialog), the bar does not
    // pass its maximum, and the log gets one line for the whole analysis. Claude Generated.
    void analysesSayWhatTheyDo()
    {
        QSharedPointer<AbstractModel> model = twoConstantModel(m_data.data());
        Charts charts = m_chart->addModel(model);
        ModelWidget widget(model, charts);
        widget.resize(1100, 800);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        StatisticDialog* dialog = widget.findChild<StatisticDialog*>();
        QVERIFY(dialog);
        QProgressBar* bar = dialog->findChild<QProgressBar*>();
        QVERIFY(bar);
        ScanWidget* scan = dialog->findChild<ScanWidget*>();
        QVERIFY(scan);

        QJsonObject monte = dialog->RunMonteCarlo();
        monte["MaxSteps"] = 40;
        QJsonObject grid = dialog->RunGridSearch();
        QJsonObject moco = dialog->RunModelComparison();
        moco["MaxSteps"] = 400;
        QJsonObject scanJob = scan->Job();
        // two points of the first constant only
        scanJob["0"] = QStringLiteral("%1 %2 1 1").arg(model->GlobalParameter(0) - 0.5).arg(model->GlobalParameter(0) + 0.5);
        const QList<QPair<QString, QJsonObject>> jobs = { { "Monte Carlo", monte }, { "Grid Search", grid }, { "Model Comparison", moco }, { "Scan", scanJob } };
        for (const QPair<QString, QJsonObject>& j : jobs) {
            QSignalSpy logged(model->Info(), &DataClassPrivateObject::Message);
            QStringList stages;
            bool within = true;
            QMetaObject::Connection stage = QObject::connect(widget.Jobs(), &JobManager::Stage, [&](const QString& text, bool) { stages << text; });
            QMetaObject::Connection counted = QObject::connect(widget.Jobs(), &JobManager::incremented, [&](int) {
                if (bar->value() > bar->maximum())
                    within = false;
            });
            qInfo("analysis: %s", qPrintable(j.first));
            widget.Jobs()->AddSingleJob(j.second);
            widget.Jobs()->RunJobs();
            QObject::disconnect(stage);
            QObject::disconnect(counted);
            QVERIFY2(!stages.isEmpty(), qPrintable(j.first + ": no stage text"));
            QVERIFY2(within, qPrintable(j.first + ": the bar went past its maximum"));
            QVERIFY2(logged.size() <= 1, qPrintable(j.first + QStringLiteral(": %1 lines in the log").arg(logged.size())));
        }
    }

    // The dialog of the project has no model: its jobs carry no parameter lists, they are made for each model when the job runs on it
    // (all parameters, global and local; or the global ones), together with the limit of the sum of squares of that model (the
    // dialog used to send an uninitialised one).
    void projectDialogJobsAreMadeForEachModel()
    {
        QSettings().remove(QStringLiteral("model"));
        StatisticDialog project;
        const QJsonObject job = project.RunGridSearch();
        QVERIFY(job["AllParameters"].toBool());
        QVERIFY(job["AllLocalParameters"].toBool());
        QVERIFY(job["GlobalParameterList"].toString().isEmpty()); // only the default of the block, nothing chosen
        QVERIFY(project.RunModelComparison()["BoxScaling"].toBool());

        QSharedPointer<AbstractModel> model = twoConstantModel(m_data.data());
        const int series = model->SeriesCount(), locals = model->LocalParameterSize();
        QJsonObject resolved = job;
        ParameterSelector::ResolveFor(resolved, model);
        QCOMPARE(resolved["GlobalParameterList"].toString(), QStringLiteral("1 1"));
        QStringList ones;
        for (int k = 0; k < locals * series; ++k)
            ones << QStringLiteral("1");
        QCOMPARE(resolved["LocalParameterList"].toString(), ones.join(" ")); // all parameters
        const double limit = model->ErrorfTestThreshold(job["confidence"].toDouble() / 100.0);
        QVERIFY(std::abs(resolved["MaxParameter"].toDouble() - limit) < 1e-12 * limit);
        QVERIFY(std::abs(resolved["f-value"].toDouble() - model->finv(job["confidence"].toDouble() / 100.0)) < 1e-12);
        QVERIFY(resolved["MaxParameter"].toDouble() > model->SSE());

        // the global ones only
        QJsonObject globalOnly = job;
        globalOnly["AllLocalParameters"] = false;
        ParameterSelector::ResolveFor(globalOnly, model);
        QCOMPARE(globalOnly["GlobalParameterList"].toString(), QStringLiteral("1 1"));
        QVERIFY(globalOnly["LocalParameterList"].toString().isEmpty());

        // the box scaling of the Model Comparison for every parameter of every series
        QJsonObject moco = project.RunModelComparison();
        ParameterSelector::ResolveFor(moco, model);
        QCOMPARE(moco["GlobalParameterScalingList"].toString(), QStringLiteral("1.5 1.5"));
        QCOMPARE(moco["LocalParameterScalingList"].toString().split(' ').size(), locals * series);

        // a job that already has its lists is left alone
        QJsonObject own;
        own["GlobalParameterList"] = QStringLiteral("1 0");
        own["MaxParameter"] = 42.0;
        QJsonObject copy = own;
        ParameterSelector::ResolveFor(copy, model);
        QCOMPARE(copy, own);
    }

    // The project job reaches the model: a grid search of the project dialog runs on the model through the pane and finds the limits of
    // both constants.
    void projectJobRunsOnTheModel()
    {
        QSettings().remove(QStringLiteral("model"));
        QSharedPointer<AbstractModel> model = twoConstantModel(m_data.data());
        Charts charts = m_chart->addModel(model);
        ModelWidget widget(model, charts);
        widget.resize(1100, 800);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);

        StatisticDialog project;
        QJsonObject job = project.RunGridSearch();
        job["MaxSteps"] = 400;
        const int before = model->getWGStatisticResult();
        widget.setJob(job);
        QCOMPARE(model->getWGStatisticResult(), before + 1);
        const QJsonObject result = model->getStatistic(SupraFit::Method::WeakenedGridSearch, before);
        const QJsonObject controller = result["controller"].toObject();
        QCOMPARE(controller["GlobalParameterList"].toString(), QStringLiteral("1 1"));
        QVERIFY(controller["MaxParameter"].toDouble() > model->SSE());
        QVERIFY(result.contains("0") && result.contains("1")); // the limits of both constants
        model->RemoveStatistic(SupraFit::Method::WeakenedGridSearch, before);
    }

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
    // WP2 measurement: the Model Definition page at several widths - how many scroll areas are nested and
    // which scroll bars show. PNGs for the docs with SUPRAFIT_LAYOUT_RENDER_DIR. Claude Generated.
    void modelDefinitionPageScrolling()
    {
        QSettings().remove(QStringLiteral("model"));
        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        const QList<QPair<int, int>> sizes = { { 1100, 700 }, { 760, 540 }, { 520, 540 } };
        for (const QPair<int, int>& size : sizes) {
            Charts charts = m_chart->addModel(m_model);
            ModelWidget widget(m_model, charts);
            widget.resize(size.first, size.second);
            widget.show();
            QTest::qWaitForWindowExposed(&widget);
            QCoreApplication::processEvents();

            QListWidget* list = nullptr;
            for (QListWidget* l : widget.findChildren<QListWidget*>())
                if (l->isVisible() && l->count() > 0) {
                    list = l;
                    break;
                }
            QVERIFY(list);
            for (int i = 0; i < list->count(); ++i)
                if (list->item(i)->text() == QStringLiteral("Model Definition")) {
                    list->setCurrentRow(i);
                    emit list->itemClicked(list->item(i));
                }
            QCoreApplication::processEvents();

            int areas = 0, vbars = 0, hbars = 0;
            for (QScrollArea* area : widget.findChildren<QScrollArea*>()) {
                if (!area->isVisible())
                    continue;
                ++areas;
                if (area->verticalScrollBar()->isVisible())
                    ++vbars;
                if (area->horizontalScrollBar()->isVisible())
                    ++hbars;
            }
            qInfo("%dx%d: visible scroll areas=%d vertical bars=%d horizontal bars=%d", size.first, size.second, areas, vbars, hbars);
            if (!renderDir.isEmpty()) {
                QDir().mkpath(renderDir);
                widget.grab().save(QStringLiteral("%1/modelwidget_definition_%2x%3.png").arg(renderDir).arg(size.first).arg(size.second));
            }
            QVERIFY2(hbars == 0, qPrintable(QStringLiteral("horizontal scroll bar on the definition page at %1 px").arg(size.first)));

            // the equation is what the page is about: it comes first, the form follows the width
            QGroupBox* equation = nullptr;
            QGroupBox* name = nullptr;
            QGroupBox* engine = nullptr;
            for (QGroupBox* box : widget.findChildren<QGroupBox*>()) {
                if (box->title() == QStringLiteral("Define model equation"))
                    equation = box;
                else if (box->title() == QStringLiteral("Model Name"))
                    name = box;
                else if (box->title() == QStringLiteral("Scripting engine"))
                    engine = box;
            }
            QVERIFY(equation && name && engine);
            QVERIFY2(equation->isVisible() && name->isVisible(), "equation or name not shown");
            const QWidget* page = equation->parentWidget();
            QVERIFY2(equation->mapTo(page, QPoint(0, 0)).y() < name->mapTo(page, QPoint(0, 0)).y(), "the equation is not the first field");
            QVERIFY2(equation->width() <= page->width(), "the equation is wider than the page");
            QVERIFY2(engine->height() < 5 || !engine->isVisible() || engine->parentWidget()->height() < 5, "the scripting engine is not tucked away");
        }
        QSettings().remove(QStringLiteral("model"));
    }
    // The "Define Model" dialog: clicking ANY preset in its list must fill the form so that the model built from
    // getObject() is the model the Add-model menu defines directly from that preset. Claude Generated.
    void defineModelDialogPresetBuildsTheModel()
    {
        const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
        QStringList failures;
        for (const ScriptModelPreset& preset : presets) {
            // the reference: defined straight from the preset (the menu's way)
            const int columns = qMax(2, preset.inputSize); // titration data have two columns, whatever the preset needs
            QSharedPointer<DataClass> refData(makeDataFor(columns));
            QSharedPointer<AbstractModel> reference = CreateModel(SupraFit::ScriptModel, refData.data());
            QHash<QString, QJsonObject> direct;
            for (const QJsonObject& d : ScriptModel::PresetBlockFor(preset, columns))
                direct[d["name"].toString()] = d;
            reference->setModelDefinition(direct);
            const bool referenceOk = reference->DefineModel(QJsonObject()) && reference->Complete();

            // the dialog's way
            QSharedPointer<DataClass> data(makeDataFor(columns));
            QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data.data());
            PrepareWidget* form = new PrepareWidget(model->getInputBlock(), true);
            QVector<QPair<QString, QVector<QJsonObject>>> offered; // as ModelDataHolder offers them for this data
            for (const ScriptModelPreset& p : presets)
                if (p.inputSize <= columns)
                    offered << qMakePair(p.name, ScriptModel::PresetBlockFor(p, columns));
            form->setInputColumns(columns);
            form->AddPresets(offered);
            form->AddScriptGuidance();
            form->setEquilibriumMode(false);
            form->resize(1000, 700);
            form->show();
            QTest::qWaitForWindowExposed(form);
            QCoreApplication::processEvents();

            QListWidget* list = nullptr;
            QListWidgetItem* entry = nullptr;
            for (QListWidget* l : form->findChildren<QListWidget*>())
                for (int i = 0; i < l->count(); ++i)
                    if (l->item(i)->text() == preset.name) {
                        list = l;
                        entry = l->item(i);
                    }
            if (!entry) {
                failures << preset.name + QStringLiteral(": not offered");
                delete form;
                continue;
            }
            list->setCurrentItem(entry);
            emit list->itemClicked(entry);
            QCoreApplication::processEvents();

            model->setModelDefinition(form->getObject());
            const bool defined = model->DefineModel(QJsonObject());
            const bool complete = defined && model->Complete();
            if (referenceOk && !complete)
                failures << QStringLiteral("%1: direct ok, dialog %2").arg(preset.name, defined ? QStringLiteral("incomplete") : QStringLiteral("DefineModel failed"));
            else if (referenceOk && (model->GlobalParameterSize() != reference->GlobalParameterSize()
                         || model->LocalParameterSize() != reference->LocalParameterSize()
                         || model->InputParameterSize() != reference->InputParameterSize()))
                failures << QStringLiteral("%1: parameters differ (global %2/%3, local %4/%5, input %6/%7)")
                                .arg(preset.name)
                                .arg(model->GlobalParameterSize()).arg(reference->GlobalParameterSize())
                                .arg(model->LocalParameterSize()).arg(reference->LocalParameterSize())
                                .arg(model->InputParameterSize()).arg(reference->InputParameterSize());
            delete form;
        }
        QVERIFY2(failures.isEmpty(), qPrintable(failures.join("\n")));
    }
    // The *_any titration models ask for their reaction system in the "Define Model" dialog when they are added
    // without a preset. What that dialog hands back (getObject()) must define the same model as the menu
    // does for a preset (only the Reactions element). Claude Generated.
    void anyModelDialogBuildsTheModel()
    {
        const QString reactions = QStringLiteral("A + B <=> AB");

        QSharedPointer<DataClass> refData(makeDataFor(2));
        QSharedPointer<AbstractModel> reference = CreateModel(SupraFit::nmr_any, refData.data());
        QVERIFY(reference && reference->DemandInput());
        QJsonObject direct;
        direct["name"] = QStringLiteral("Reactions");
        direct["type"] = 6;
        direct["value"] = reactions;
        reference->setModelDefinition(QHash<QString, QJsonObject>{ { QStringLiteral("Reactions"), direct } });
        QVERIFY2(reference->DefineModel(QJsonObject()), "the menu's way does not define the model");
        QVERIFY(reference->Complete());

        QSharedPointer<DataClass> data(makeDataFor(2));
        QSharedPointer<AbstractModel> model = CreateModel(SupraFit::nmr_any, data.data());
        PrepareWidget* form = new PrepareWidget(model->getInputBlock(), true);
        // hosted like ModelDataHolder::AddModel does it, at the dialog's own size
        GenericWidgetDialog dialog(QStringLiteral("Define Model"), form);
        dialog.show();
        QTest::qWaitForWindowExposed(&dialog);
        QCoreApplication::processEvents();
        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        if (!renderDir.isEmpty())
            dialog.grab().save(QStringLiteral("%1/define_model_dialog_any.png").arg(renderDir));

        PrepareBox* box = nullptr;
        QStringList names;
        for (PrepareBox* b : form->findChildren<PrepareBox*>()) {
            names << b->Name();
            if (b->Name() == QLatin1String("Reactions"))
                box = b;
        }
        QVERIFY2(box, qPrintable(QStringLiteral("no Reactions field in the dialog, fields: %1").arg(names.join(", "))));
        QVERIFY2(box->isVisible(), "the reaction editor is not shown");
        // choose "1:1" in the preset list of the reaction editor, like the user does
        QListWidgetItem* preset = nullptr;
        QListWidget* presetList = nullptr;
        for (QListWidget* l : box->findChildren<QListWidget*>())
            for (int i = 0; i < l->count(); ++i)
                if (l->item(i)->text() == QStringLiteral("1:1")) {
                    presetList = l;
                    preset = l->item(i);
                }
        QVERIFY2(preset, "the 1:1 preset is not offered in the dialog");
        presetList->setCurrentItem(preset);
        emit presetList->itemClicked(preset);
        QCoreApplication::processEvents();

        const QHash<QString, QJsonObject> elements = form->getObject();
        QVERIFY2(elements.contains(QStringLiteral("Reactions")), "the dialog returns no reaction system");
        QVERIFY2(!elements.value(QStringLiteral("Reactions"))["value"].toString().trimmed().isEmpty(), "the preset did not reach the form's value");
        qInfo("reactions from the preset: %s", qPrintable(elements.value(QStringLiteral("Reactions"))["value"].toString()));
        model->setModelDefinition(elements);
        QVERIFY2(model->DefineModel(QJsonObject()), "the dialog's definition does not define the model");
        QVERIFY2(model->Complete(), "the dialog's model is incomplete");
        QCOMPARE(model->GlobalParameterSize(), reference->GlobalParameterSize());
        QCOMPARE(model->LocalParameterSize(), reference->LocalParameterSize());
        QCOMPARE(model->InputParameterSize(), reference->InputParameterSize());
    }
    // WP6: advanced_ui is followed live (no restart): the simulation chips
    // appear and disappear when the setting changes. Claude Generated.
    void advancedUiIsFollowedLive()
    {
        const QVariant before = qApp->property("advanced_ui");
        Charts charts = m_chart->addModel(m_model);
        qApp->setProperty("advanced_ui", false);
        ModelWidget widget(m_model, charts);
        widget.resize(1100, 700);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        auto shownChips = [&widget]() {
            int shown = 0;
            for (DnDLabel* label : widget.findChildren<DnDLabel*>())
                if (!label->isHidden()) // the page may not be the shown one: only the chip's own state counts
                    ++shown;
            return shown;
        };
        const int basic = shownChips();
        QVERIFY2(basic >= 1, "no simulation chip at all without advanced_ui (the Bootstrap chip stays)");

        qApp->setProperty("advanced_ui", true);
        emit Instance::GlobalInstance()->ConfigurationChanged();
        QCoreApplication::processEvents();
        QVERIFY2(shownChips() > basic, "the advanced simulation chips did not appear");

        qApp->setProperty("advanced_ui", false);
        emit Instance::GlobalInstance()->ConfigurationChanged();
        QCoreApplication::processEvents();
        QCOMPARE(shownChips(), basic);
        qApp->setProperty("advanced_ui", before);
    }
    // WP7: on a small screen the pane must not ask for more room than there is. The page list (the statistic
    // tools alone asked for 528x443 px) and the whole pane scroll instead; nothing overlaps. Claude Generated.
    void paneScrollsOnSmallScreens()
    {
        QSettings().remove(QStringLiteral("model"));
        const QList<QPair<int, int>> sizes = { { 1000, 600 }, { 640, 460 }, { 480, 420 } };
        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        for (const QPair<int, int>& size : sizes) {
            Charts charts = m_chart->addModel(m_model);
            ModelWidget widget(m_model, charts);
            widget.resize(size.first, size.second);
            widget.show();
            QTest::qWaitForWindowExposed(&widget);
            QCoreApplication::processEvents();

            // the window really is that small (the layout no longer insists on its content size)
            QVERIFY2(widget.height() <= size.second + 2, qPrintable(QStringLiteral("pane forced to %1 px height instead of %2")
                                                                        .arg(widget.height()).arg(size.second)));
            QVERIFY2(widget.width() <= size.first + 2, qPrintable(QStringLiteral("pane forced to %1 px width instead of %2")
                                                                      .arg(widget.width()).arg(size.first)));

            // inside the scrolled content, the parts do not overlap
            QScrollArea* pane = widget.findChild<QScrollArea*>(QStringLiteral("model_pane_scroll"));
            QVERIFY(pane && pane->widget());
            QLayout* content = pane->widget()->layout();
            QVERIFY(content && content->count() >= 2);
            const QRect header = content->itemAt(0)->geometry();
            const QRect splitter = content->itemAt(1)->geometry();
            QVERIFY2(header.bottom() < splitter.top(), qPrintable(QStringLiteral("header and page area overlap at %1x%2").arg(size.first).arg(size.second)));
            if (!renderDir.isEmpty()) {
                QDir().mkpath(renderDir);
                widget.grab().save(QStringLiteral("%1/modelwidget_small_%2x%3.png").arg(renderDir).arg(size.first).arg(size.second));
            }
        }
        QSettings().remove(QStringLiteral("model"));
    }
    // WP7: the pane in the dark scheme - pages render (PNGs with SUPRAFIT_LAYOUT_RENDER_DIR for a look at the
    // contrast) and the colours come from the palette, so the window colour really is dark. Claude Generated.
    void rendersInTheDarkScheme()
    {
        Appearance::Install();
        Appearance::Apply(Appearance::Dark);
        QVERIFY(Appearance::IsDark());
        QSettings().remove(QStringLiteral("model"));
        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");

        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(900, 640);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();
        QVERIFY2(widget.palette().color(QPalette::Window).lightness() < 128, "the pane did not take the dark palette");

        auto select = [&widget](const QString& title) {
            for (QListWidget* l : widget.findChildren<QListWidget*>())
                if (l->isVisible())
                    for (int i = 0; i < l->count(); ++i)
                        if (l->item(i)->text() == title) {
                            l->setCurrentRow(i);
                            emit l->itemClicked(l->item(i));
                            QCoreApplication::processEvents();
                            return true;
                        }
            return false;
        };
        QDir().mkpath(renderDir.isEmpty() ? QStringLiteral(".") : renderDir);
        for (const QString& page : { QStringLiteral("Data series"), QStringLiteral("Model Definition"), QStringLiteral("Simulated Data") }) {
            QVERIFY2(select(page), qPrintable(page));
            QVERIFY(!widget.grab().isNull());
            if (!renderDir.isEmpty())
                widget.grab().save(QStringLiteral("%1/modelwidget_dark_%2.png").arg(renderDir, QString(page).replace(' ', '_')));
        }
        QMetaObject::invokeMethod(&widget, "ToggleStatisticDialog");
        QCoreApplication::processEvents();
        if (!renderDir.isEmpty())
            widget.grab().save(QStringLiteral("%1/modelwidget_dark_statistics.png").arg(renderDir));

        Appearance::Apply(Appearance::Light);
        QVERIFY(!Appearance::IsDark());
    }
    // WP3: a dialog that stays a window opens centred on the main window the first time and where the user left
    // it, at the size the user gave it, afterwards. Claude Generated.
    void windowHostRemembersPlaceAndSize()
    {
        QSettings().remove(QStringLiteral("windows"));
        QWidget anchor;
        anchor.resize(500, 300);
        anchor.move(100, 100);
        anchor.show();
        QTest::qWaitForWindowExposed(&anchor);

        QDialog* first = new QDialog;
        first->resize(300, 200);
        WindowHost::Show(first, QStringLiteral("test_window"), &anchor);
        QTest::qWaitForWindowExposed(first);
        const QPoint centre = first->frameGeometry().center();
        const QPoint anchorCentre = anchor.frameGeometry().center();
        QVERIFY2((centre - anchorCentre).manhattanLength() < 40,
            qPrintable(QStringLiteral("first opening is not centred on the main window: %1,%2 vs %3,%4").arg(centre.x()).arg(centre.y()).arg(anchorCentre.x()).arg(anchorCentre.y())));

        first->move(30, 40);
        first->resize(333, 222);
        QCoreApplication::processEvents();
        WindowHost::Show(first, QStringLiteral("test_window"), &anchor); // open already: brought to the front, not moved
        QCOMPARE(first->size(), QSize(333, 222));
        first->hide(); // closing keeps the place
        delete first;

        QDialog second;
        second.resize(100, 100);
        WindowHost::Show(&second, QStringLiteral("test_window"), &anchor);
        QTest::qWaitForWindowExposed(&second);
        QCOMPARE(second.size(), QSize(333, 222));
        QCOMPARE(second.pos(), QPoint(30, 40));

        // another kind of window has its own place
        QDialog other;
        other.resize(150, 120);
        WindowHost::Show(&other, QStringLiteral("other_window"), &anchor);
        QTest::qWaitForWindowExposed(&other);
        QCOMPARE(other.size(), QSize(150, 120));
        second.hide();
        other.hide();
        QSettings().remove(QStringLiteral("windows"));
    }

    // WP-S3: the Scan is an analysis of the Statistics page, not a window of its own; "Global Search..." selects it. Its job keeps the
    // format of the global search: ParameterSize and "min max step optimise" for each parameter. Claude Generated.
    void scanIsAnAnalysisOfTheStatisticsPage()
    {
        QSharedPointer<AbstractModel> model = twoConstantModel(m_data.data());
        Charts charts = m_chart->addModel(model);
        ModelWidget widget(model, charts);
        widget.resize(1100, 800);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        const int windowsBefore = visibleTopLevels().size();
        QMetaObject::invokeMethod(&widget, "OpenAdvancedSearch");
        QCoreApplication::processEvents();
        QCOMPARE(visibleTopLevels().size(), windowsBefore); // no window
        StatisticDialog* dialog = widget.findChild<StatisticDialog*>();
        QVERIFY(dialog);
        const QStringList tabs = dialog->TabTitles();
        QVERIFY(tabs.contains(QStringLiteral("Scan")));
        QVERIFY(!overlappingWidgets(dialog).size());

        ScanWidget* scan = dialog->findChild<ScanWidget*>();
        QVERIFY(scan);
        const QJsonObject job = scan->Job();
        QCOMPARE(job["Method"].toInt(), int(SupraFit::Method::GlobalSearch));
        const int size = job["ParameterSize"].toInt();
        QVERIFY(size >= model->GlobalParameterSize());
        for (int i = 0; i < size; ++i)
            QCOMPARE(job[QString::number(i)].toString().split(' ').size(), 4);
        QVERIFY(scan->Calculations() >= 1);

        // the range of a logarithmic constant is the value +- 3, the others value/5 .. 5 value
        double min, max, step;
        ScanWidget::DefaultRange(QStringLiteral("lg K11"), 2.0, min, max, step);
        QCOMPARE(min, -1.0);
        QCOMPARE(max, 5.0);
        ScanWidget::DefaultRange(QStringLiteral("K11"), 100.0, min, max, step);
        QVERIFY(min > 0 && min <= 20.0); // rounded outwards
        QVERIFY(max >= 500.0 && max < 1000.0);

        // Fit is the optimisation flag of the model
        QCheckBox* fit = nullptr;
        for (QCheckBox* c : scan->findChildren<QCheckBox*>())
            if (c->text() == QStringLiteral("Fit")) {
                fit = c;
                break;
            }
        QVERIFY(fit);
        fit->setChecked(false);
        QVERIFY(!model->GlobalTable()->isChecked(0, 0));
        QVERIFY(scan->Job()["0"].toString().endsWith(QStringLiteral(" 0")));
        fit->setChecked(true);
        QVERIFY(model->GlobalTable()->isChecked(0, 0));
    }
    // The model's charts are listed one by one (no tabs): a chart that appears is an entry of the Charts page, the
    // page is chosen with the Charts button, an entry shows its chart, and Close chart removes it. Claude Generated.
    void chartsAreListedOneByOne()
    {
        QSettings().remove(QStringLiteral("model"));
        QSharedPointer<DataClass> data(makeData(2));
        QSharedPointer<ChartingModel> charting(new ChartingModel(data.data()));
        QSharedPointer<AbstractModel> model = charting;
        QHash<QString, QJsonObject> elements;
        for (const ScriptModelPreset& preset : ScriptModel::Presets())
            if (preset.name.startsWith(QStringLiteral("Binding 1:1 \u2014 signal"))) { // the one that fits two input columns
                for (const QJsonObject& d : preset.block)
                    elements[d["name"].toString()] = d;
                break;
            }
        QVERIFY(!elements.isEmpty());
        model->setModelDefinition(elements);
        QVERIFY(model->DefineModel(QJsonObject()));

        ChartWidget chart;
        chart.setRawData(data);
        Charts charts = chart.addModel(model);
        ModelWidget widget(model, charts);
        widget.resize(900, 640);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        auto listTitles = [&widget](QListWidget** found = nullptr) {
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
        QVERIFY2(!listTitles().contains(QStringLiteral("Charts")), "Charts is listed although the model has no chart");

        // the model reports two charts (names as the titration models use them)
        charting->feed(QStringLiteral("Concentration Chart"), { QStringLiteral("A"), QStringLiteral("B") });
        charting->feed(QStringLiteral("Heat Chart I"), { QStringLiteral("q"), QStringLiteral("dq") });
        QCoreApplication::processEvents();
        QVERIFY2(listTitles().contains(QStringLiteral("Charts")), "Charts is not listed although the model has charts");

        QListWidget* list = nullptr;
        QStringList titles;
        // choose the page like the Charts button does
        for (QListWidget* l : widget.findChildren<QListWidget*>())
            if (l->isVisible())
                for (int i = 0; i < l->count(); ++i)
                    if (l->item(i)->text() == QStringLiteral("Charts")) {
                        l->setCurrentRow(i);
                        emit l->itemClicked(l->item(i));
                    }
        QCoreApplication::processEvents();
        titles = listTitles(&list);
        QVERIFY2(titles.size() >= 3 && titles.first().contains(QStringLiteral("Back")), qPrintable(titles.join(", ")));
        QVERIFY2(titles.contains(QStringLiteral("Concentration")), qPrintable(titles.join(", ")));
        QVERIFY2(titles.contains(QStringLiteral("Heat I")), qPrintable(titles.join(", ")));

        // no tab bar of its own inside the page
        for (QTabBar* bar : widget.findChildren<QTabBar*>())
            QVERIFY2(!bar->isVisible() || bar->count() == 0 || bar->parentWidget()->parentWidget() == nullptr, "a tab bar is shown on the Charts page");
        QSettings().remove(QStringLiteral("model"));
    }
    // Every entry of the page list has an icon, and the entries that open a list of their own (Statistics, Charts)
    // are marked as such, the others are not. Claude Generated.
    void pageListHasIconsAndGroupMarkers()
    {
        QSettings().remove(QStringLiteral("model"));
        QSharedPointer<ChartingModel> charting(new ChartingModel(m_data.data()));
        QSharedPointer<AbstractModel> model = charting;
        QHash<QString, QJsonObject> elements;
        for (const ScriptModelPreset& preset : ScriptModel::Presets())
            if (preset.name.startsWith(QStringLiteral("Binding 1:1 \u2014 signal"))) {
                for (const QJsonObject& d : preset.block)
                    elements[d["name"].toString()] = d;
                break;
            }
        model->setModelDefinition(elements);
        QVERIFY(model->DefineModel(QJsonObject()));
        Charts charts = m_chart->addModel(model);
        ModelWidget widget(model, charts);
        widget.resize(900, 640);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        charting->feed(QStringLiteral("Concentration Chart"), { QStringLiteral("A"), QStringLiteral("B") });
        QCoreApplication::processEvents();

        QListWidget* list = nullptr;
        for (QListWidget* l : widget.findChildren<QListWidget*>())
            if (l->isVisible() && l->count() > 0) {
                list = l;
                break;
            }
        QVERIFY(list);
        QStringList seen;
        for (int i = 0; i < list->count(); ++i) {
            const QListWidgetItem* item = list->item(i);
            if (item->isHidden())
                continue;
            seen << item->text();
            QVERIFY2(!item->icon().isNull(), qPrintable(QStringLiteral("no icon for '%1'").arg(item->text())));
            const bool group = item->text() == QStringLiteral("Statistics") || item->text() == QStringLiteral("Charts");
            QCOMPARE(item->data(Qt::UserRole + 10).toBool(), group); // GroupMarkerDelegate::GroupRole
        }
        QVERIFY2(seen.contains(QStringLiteral("Statistics")) && seen.contains(QStringLiteral("Charts")), qPrintable(seen.join(", ")));
        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        if (!renderDir.isEmpty())
            widget.grab().save(QStringLiteral("%1/modelwidget_list_icons.png").arg(renderDir));
        QSettings().remove(QStringLiteral("model"));
    }
    // The page list reaches Statistics, Results and Charts, and the global search sits in the Fit menu: the model
    // pane has no buttons for them any more. Claude Generated.
    void actionButtonsAreNotDoubledByThePageList()
    {
        QSettings().remove(QStringLiteral("model"));
        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(900, 640);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        // only the action buttons of the pane count (the dialogs of the pane have buttons of their own)
        ModelActions* actions = widget.findChild<ModelActions*>();
        QVERIFY(actions);
        QStringList shown;
        for (QPushButton* b : actions->findChildren<QPushButton*>())
            if (!b->isHidden() && b->text() != QString())
                shown << b->text();
        QPushButton* fit = nullptr;
        for (QPushButton* b : widget.findChildren<QPushButton*>())
            if (b->text() == QStringLiteral("Fit") && b->menu())
                fit = b;
        for (const QString& gone : { QStringLiteral("Statistic"), QStringLiteral("Results"), QStringLiteral("Charts"), QStringLiteral("Scan"),
                 QStringLiteral("Detailed"), QStringLiteral("Local Fits"), QStringLiteral("..more..") })
            QVERIFY2(!shown.contains(gone), qPrintable(QStringLiteral("button '%1' is still there: %2").arg(gone, shown.join(", "))));
        QVERIFY2(shown.contains(QStringLiteral("New Guess")) && shown.contains(QStringLiteral("Restore")), qPrintable(shown.join(", ")));
        // the "..more.." switch is gone: everything left sits in one row that wraps
        for (QToolButton* b : actions->findChildren<QToolButton*>())
            QVERIFY2(!b->text().contains(QStringLiteral("..")), "the more/less switch is still in the action bar");

        QVERIFY2(fit, "no Fit button with a menu");
        QStringList entries;
        for (const QAction* action : fit->menu()->actions())
            entries << action->text();
        QVERIFY2(entries.contains(QStringLiteral("Global Search...")) && entries.contains(QStringLiteral("Local Fits")), qPrintable(entries.join(", ")));
        QSettings().remove(QStringLiteral("model"));
    }
    // The detailed data (input data, model, parameters, statistics) is a page of the list that writes itself when
    // it is chosen. Claude Generated.
    void detailedIsAPage()
    {
        QSettings().remove(QStringLiteral("model"));
        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(900, 640);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        const int windowsBefore = visibleTopLevels().size();
        QListWidget* list = nullptr;
        for (QListWidget* l : widget.findChildren<QListWidget*>())
            if (l->isVisible() && l->count() > 0) {
                list = l;
                break;
            }
        QVERIFY(list);
        QListWidgetItem* entry = nullptr;
        for (int i = 0; i < list->count(); ++i)
            if (list->item(i)->text() == QStringLiteral("Detailed"))
                entry = list->item(i);
        QVERIFY2(entry, "no page \"Detailed\"");
        list->setCurrentItem(entry);
        emit list->itemClicked(entry);
        QCoreApplication::processEvents();

        QTextBrowser* text = nullptr; // TextWidget is a QTextBrowser without Q_OBJECT: found by its base class
        for (QTextBrowser* b : widget.findChildren<QTextBrowser*>())
            if (b->isVisible())
                text = b;
        QVERIFY2(text, "no visible text view on the Detailed page");
        QVERIFY2(text->isVisible(), "the detailed page is empty: its text is not shown");
        QVERIFY2(text->toPlainText().size() > 100, qPrintable(QStringLiteral("detailed text too short: %1 characters").arg(text->toPlainText().size())));
        QCOMPARE(visibleTopLevels().size(), windowsBefore); // a page, not a window
        QSettings().remove(QStringLiteral("model"));
    }
    // The statistical analyses of the stored results are the slow part of the Detailed page (BC50 integrals for every
    // result): they are an opt-in box in the page header, off by default, and switching them on adds the text.
    // Claude Generated.
    void detailedStatisticsAreOptIn()
    {
        QSettings().remove(QStringLiteral("model"));
        QSettings().remove(QStringLiteral("ui"));
        {
            QJsonObject job = MonteCarloConfigBlock;
            job["MaxSteps"] = 200;
            JobManager jobs;
            jobs.setModel(m_model);
            jobs.AddSingleJob(job);
            jobs.RunJobs();
        }
        QVERIFY(m_model->getMCStatisticResult() >= 1);

        Charts charts = m_chart->addModel(m_model);
        ModelWidget widget(m_model, charts);
        widget.resize(900, 640);
        widget.show();
        QTest::qWaitForWindowExposed(&widget);
        QCoreApplication::processEvents();

        QListWidget* list = nullptr;
        for (QListWidget* l : widget.findChildren<QListWidget*>())
            if (l->isVisible() && l->count() > 0) {
                list = l;
                break;
            }
        QVERIFY(list);
        for (int i = 0; i < list->count(); ++i)
            if (list->item(i)->text() == QStringLiteral("Detailed")) {
                list->setCurrentRow(i);
                emit list->itemClicked(list->item(i));
            }
        QCoreApplication::processEvents();

        QCheckBox* box = nullptr;
        for (QCheckBox* c : widget.findChildren<QCheckBox*>())
            if (c->text() == QStringLiteral("Include the statistical analyses"))
                box = c;
        QVERIFY2(box, "no box for the statistical analyses on the Detailed page");
        QVERIFY2(!box->isHidden(), "the box is hidden although the model has results");
        QVERIFY2(!box->isChecked(), "the statistics are on by default: the page is slow again");

        auto viewText = [&widget]() {
            QTextBrowser* view = nullptr;
            for (QTextBrowser* b : widget.findChildren<QTextBrowser*>())
                if (b->isVisible())
                    view = b;
            return view ? view->toPlainText() : QString();
        };
        const QString without = viewText();
        QVERIFY(without.size() > 100);
        box->setChecked(true);
        QCoreApplication::processEvents();
        const QString with = viewText();
        QVERIFY2(with.size() > without.size(), qPrintable(QStringLiteral("no statistics added: %1 vs %2 characters").arg(with.size()).arg(without.size())));
        box->setChecked(false);
        QCoreApplication::processEvents();
        QCOMPARE(viewText().size(), without.size());

        m_model->RemoveStatistic(SupraFit::Method::MonteCarlo, 0);
        QSettings().remove(QStringLiteral("ui"));
        QSettings().remove(QStringLiteral("model"));
    }
    // "Define Model" used twice in a row: choose one entry of the preset list (also "Equilibrium (reactions)" from
    // scratch), then another one. The form must end up as the second entry alone, whatever the first was. Claude Generated.
    void defineModelDialogSecondPresetBuildsTheModel()
    {
        const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
        QStringList entries; // the list of the dialog, by its texts
        entries << QStringLiteral("Free equation") << QStringLiteral("Equilibrium (reactions)");
        for (const ScriptModelPreset& p : presets)
            entries << p.name;
        QStringList failures;
        int combinations = 0;
        for (const QString& first : entries)
            for (const QString& second : entries) {
                if (first == second)
                    continue;
                // the second as a preset, if it is one (a start-from-scratch entry as second is checked as empty form)
                const ScriptModelPreset* wanted = nullptr;
                for (const ScriptModelPreset& p : presets)
                    if (p.name == second)
                        wanted = &p;
                const int inputs = qMax(2, wanted ? wanted->inputSize : 2); // titration data have two columns
                QSharedPointer<DataClass> data(makeDataFor(inputs));
                QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, data.data());
                PrepareWidget* form = new PrepareWidget(model->getInputBlock(), true);
                QVector<QPair<QString, QVector<QJsonObject>>> offered;
                for (const ScriptModelPreset& p : presets)
                    if (p.inputSize <= inputs)
                        offered << qMakePair(p.name, ScriptModel::PresetBlockFor(p, inputs));
                form->setInputColumns(inputs);
                form->AddPresets(offered);
                form->AddScriptGuidance();
                form->setEquilibriumMode(false);
                form->resize(1000, 700);
                form->show();
                QCoreApplication::processEvents();

                auto click = [form](const QString& text) {
                    for (QListWidget* l : form->findChildren<QListWidget*>())
                        for (int i = 0; i < l->count(); ++i)
                            if (l->item(i)->text() == text) {
                                l->setCurrentRow(i);
                                emit l->itemClicked(l->item(i));
                                QCoreApplication::processEvents();
                                return true;
                            }
                    return false;
                };
                if (!click(first) || !click(second)) {
                    failures << first + QStringLiteral(" -> ") + second + QStringLiteral(": entry not offered");
                    delete form;
                    continue;
                }
                ++combinations;
                if (wanted) {
                    QSharedPointer<DataClass> refData(makeDataFor(inputs));
                    QSharedPointer<AbstractModel> reference = CreateModel(SupraFit::ScriptModel, refData.data());
                    QHash<QString, QJsonObject> direct;
                    for (const QJsonObject& d : ScriptModel::PresetBlockFor(*wanted, inputs))
                        direct[d["name"].toString()] = d;
                    reference->setModelDefinition(direct);
                    const bool referenceOk = reference->DefineModel(QJsonObject()) && reference->Complete();

                    model->setModelDefinition(form->getObject());
                    const bool defined = model->DefineModel(QJsonObject());
                    const bool complete = defined && model->Complete();
                    if (referenceOk && !complete)
                        failures << QStringLiteral("%1 -> %2: dialog %3").arg(first, second, defined ? QStringLiteral("incomplete") : QStringLiteral("DefineModel failed"));
                    else if (referenceOk && (model->GlobalParameterSize() != reference->GlobalParameterSize()
                                 || model->LocalParameterSize() != reference->LocalParameterSize()
                                 || model->InputParameterSize() != reference->InputParameterSize()))
                        failures << QStringLiteral("%1 -> %2: parameters differ (global %3/%4, local %5/%6, input %7/%8)")
                                        .arg(first, second)
                                        .arg(model->GlobalParameterSize()).arg(reference->GlobalParameterSize())
                                        .arg(model->LocalParameterSize()).arg(reference->LocalParameterSize())
                                        .arg(model->InputParameterSize()).arg(reference->InputParameterSize());
                }
                delete form;
            }
        qInfo("%d combinations tried, %lld failures", combinations, static_cast<long long>(failures.size()));
        QVERIFY2(failures.isEmpty(), qPrintable(failures.mid(0, 12).join("\n")));
    }
    // The operator's case: a custom model with an equilibrium system is in the workspace, then a predefined model is
    // taken from the menu (New Model > Predefined models): every preset must still add its model. A message box (the
    // "preset could not be applied" warning) would block the test, so it is closed and counted as a failure.
    // Claude Generated.
    void presetFromTheMenuAfterAnEquilibriumCustomModel()
    {
        const QVector<ScriptModelPreset> presets = ScriptModel::Presets();
        QStringList failures;
        QStringList baseline; // the same presets on the same data without the custom model in the workspace
        int tried = 0;
        for (const bool withCustom : { true, false })
        for (int index = 0; index < presets.size(); ++index) {
            // titration data like an NMR experiment: two independent columns (host, guest)
            QSharedPointer<DataClass> data(makeDataFor(2));
            ChartWidget chart;
            chart.setRawData(data);
            ModelDataHolder holder;
            holder.setChartWidget(&chart);
            holder.setData(data, QSharedPointer<ChartWrapper>::create());

            // the custom model with an equilibrium system: the "native solver" preset is what the dialog's
            // "Equilibrium (reactions)" start leads to; it only gets a name of its own
            QHash<QString, QJsonObject> equilibrium;
            for (const ScriptModelPreset& p : presets)
                if (p.name.contains(QStringLiteral("native solver")))
                    for (const QJsonObject& d : p.block)
                        equilibrium[d["name"].toString()] = d;
            QVERIFY2(!equilibrium.isEmpty() && equilibrium.contains(QStringLiteral("Reactions")), "no equilibrium preset to start from");
            QJsonObject name = equilibrium.value(QStringLiteral("Name"));
            name["value"] = QStringLiteral("My equilibrium model");
            equilibrium[QStringLiteral("Name")] = name;
            if (withCustom) {
                QSharedPointer<AbstractModel> custom = CreateModel(SupraFit::ScriptModel, data.data());
                custom->setModelDefinition(equilibrium);
                const bool customOk = custom->DefineModel(QJsonObject()) && custom->Complete();
                if (!customOk) {
                    failures << presets[index].name + QStringLiteral(": the custom model could not be defined (test setup)");
                    continue;
                }
                custom->InitialGuess();
                holder.createModelWidgetFromModel(custom);
            }
            const int before = holder.ModelCount();

            QStringList boxes;
            QTimer closer;
            closer.setInterval(30);
            QObject::connect(&closer, &QTimer::timeout, [&boxes]() {
                if (QWidget* w = QApplication::activeModalWidget()) {
                    boxes << w->windowTitle();
                    w->close();
                }
            });
            closer.start();
            QMetaObject::invokeMethod(&holder, "NewModelFromPreset", Q_ARG(int, index));
            QCoreApplication::processEvents();
            closer.stop();
            ++tried;
            QStringList& into = withCustom ? failures : baseline;
            if (!boxes.isEmpty())
                into << QStringLiteral("%1: message box \"%2\"").arg(presets[index].name, boxes.join(", "));
            else if (holder.ModelCount() != before + 1)
                into << QStringLiteral("%1: no model added (%2 -> %3 models)").arg(presets[index].name).arg(before).arg(holder.ModelCount());
        }
        qInfo("%d runs, failures after the custom model: %lld, without it: %lld", tried, static_cast<long long>(failures.size()), static_cast<long long>(baseline.size()));
        qInfo("baseline failures: %s", qPrintable(baseline.join("; ")));
        QVERIFY2(failures.isEmpty(), qPrintable(failures.mid(0, 12).join("\n")));
    }
    // ---- WP-A: the model catalogue ------------------------------------------------------------------------------

    void catalogTechniquesFitTheColumns()
    {
        using ModelCatalog::Technique;
        const QList<Technique> titrations{ Technique::NMR, Technique::UVVis, Technique::Fluorescence };
        QCOMPARE(ModelCatalog::TechniquesFor(2), titrations);
        QCOMPARE(ModelCatalog::TechniquesFor(3), titrations); // three components: flexible models only
        const QList<Technique> one = ModelCatalog::TechniquesFor(1);
        QVERIFY(one.contains(Technique::Calorimetry) && one.contains(Technique::Kinetics) && !one.contains(Technique::NMR));
        QCOMPARE(ModelCatalog::TechniquesFor(0).size(), ModelCatalog::Techniques().size()); // unknown: all
        for (const ModelCatalog::TechniqueInfo& info : ModelCatalog::Techniques()) { // the settings key round-trips
            QVERIFY(!info.key.isEmpty() && !info.name.isEmpty() && !info.shortName.isEmpty());
            QCOMPARE(ModelCatalog::TechniqueFromKey(info.key, Technique::Kinetics), info.technique);
        }
        QCOMPARE(ModelCatalog::TechniqueFromKey(QStringLiteral("no such technique")), Technique::NMR); // default: NMR
    }

    void catalogStoichiometries()
    {
        const QList<ModelCatalog::Stoichiometry> list = ModelCatalog::Stoichiometries();
        QCOMPARE(list.size(), 10); // 4 established, 4 with host dimerisation, 2 with three components
        QSet<QString> keys;
        for (int i = 0; i < list.size(); ++i) {
            QVERIFY2(!list[i].key.isEmpty() && !list[i].title.isEmpty() && !list[i].reactions.isEmpty(), qPrintable(list[i].key));
            QVERIFY2(!keys.contains(list[i].key), qPrintable(QStringLiteral("key twice: %1").arg(list[i].key)));
            keys << list[i].key;
            QCOMPARE(list[i].fixedIndex, i < 4 ? i : -1);
            QCOMPARE(list[i].components, i < 8 ? 2 : 3);
        }
    }

    // What the Model menu has always offered, per technique: the four fixed models and the flexible one. The catalogue
    // must give the same, or the menu built from it (WP-E) would lose or gain entries.
    void catalogHasWhatTheModelMenuHad()
    {
        using ModelCatalog::Technique;
        QCOMPARE(ModelCatalog::FixedModels(Technique::NMR), (QList<int>{ SupraFit::nmr_ItoI, SupraFit::nmr_IItoI_ItoI, SupraFit::nmr_ItoI_ItoII, SupraFit::nmr_IItoI_ItoI_ItoII }));
        QCOMPARE(ModelCatalog::FixedModels(Technique::UVVis), (QList<int>{ SupraFit::uv_vis_ItoI, SupraFit::uv_vis_IItoI_ItoI, SupraFit::uv_vis_ItoI_ItoII, SupraFit::uv_vis_IItoI_ItoI_ItoII }));
        QCOMPARE(ModelCatalog::FixedModels(Technique::Fluorescence), (QList<int>{ SupraFit::fl_ItoI, SupraFit::fl_IItoI_ItoI, SupraFit::fl_ItoI_ItoII, SupraFit::fl_IItoI_ItoI_ItoII }));
        QCOMPARE(ModelCatalog::FixedModels(Technique::Calorimetry), (QList<int>{ SupraFit::itc_ItoI, SupraFit::itc_IItoI, SupraFit::itc_ItoII, SupraFit::itc_IItoII }));
        QCOMPARE(ModelCatalog::AnyModel(Technique::NMR), int(SupraFit::nmr_any));
        QCOMPARE(ModelCatalog::AnyModel(Technique::UVVis), int(SupraFit::uvvis_any));
        QCOMPARE(ModelCatalog::AnyModel(Technique::Fluorescence), int(SupraFit::fl_any));
        QCOMPARE(ModelCatalog::AnyModel(Technique::Calorimetry), int(SupraFit::itc_any));

        // the established stoichiometries are the fixed models, the others the flexible model with reactions
        for (const Technique t : { Technique::NMR, Technique::UVVis, Technique::Fluorescence, Technique::Calorimetry }) {
            const QList<ModelCatalog::Stoichiometry> list = ModelCatalog::Stoichiometries();
            for (int i = 0; i < list.size(); ++i) {
                const ModelCatalog::Choice choice = ModelCatalog::ForStoichiometry(t, list[i].key);
                if (i < 4) {
                    QCOMPARE(choice.kind, ModelCatalog::Choice::Kind::FixedModel);
                    QCOMPARE(choice.model, ModelCatalog::FixedModels(t).at(i));
                } else if (t == Technique::Calorimetry && list[i].components > 2) {
                    QVERIFY2(!choice.valid(), "calorimetry takes host/guest systems only");
                } else {
                    QCOMPARE(choice.kind, ModelCatalog::Choice::Kind::AnyModelWithReactions);
                    QCOMPARE(choice.model, ModelCatalog::AnyModel(t));
                    QCOMPARE(choice.reactions, list[i].reactions);
                }
            }
        }
        QVERIFY(!ModelCatalog::ForStoichiometry(Technique::NMR, QStringLiteral("no such")).valid());
    }

    void catalogEntriesByColumns()
    {
        using ModelCatalog::Technique;
        // two columns: the established and the dimerisation systems for NMR, UV/Vis and fluorescence
        for (const Technique t : { Technique::NMR, Technique::UVVis, Technique::Fluorescence })
            QCOMPARE(ModelCatalog::Entries(t, 2).size(), 8);
        // three columns: only the three-component systems, flexible models
        for (const ModelCatalog::Entry& e : ModelCatalog::Entries(Technique::NMR, 3))
            QCOMPARE(e.choice.kind, ModelCatalog::Choice::Kind::AnyModelWithReactions);
        QCOMPARE(ModelCatalog::Entries(Technique::NMR, 3).size(), 2);
        // a technique that does not fit the data offers nothing
        QVERIFY(ModelCatalog::Entries(Technique::NMR, 1).isEmpty());
        QVERIFY(ModelCatalog::Entries(Technique::Calorimetry, 2).isEmpty());
        // calorimetry (one column): the host/guest systems and the models of its own
        QCOMPARE(ModelCatalog::Entries(Technique::Calorimetry, 1).size(), 8 + 3);
        QCOMPARE(ModelCatalog::Entries(Technique::Kinetics, 1).size(), 6);
        // keys: stable and unique inside a technique
        for (const Technique t : ModelCatalog::TechniquesFor(0)) {
            QSet<QString> keys;
            for (const ModelCatalog::Entry& e : ModelCatalog::Entries(t, 0)) {
                QVERIFY2(!e.key.isEmpty() && !e.title.isEmpty(), qPrintable(e.key));
                QVERIFY2(!keys.contains(e.key), qPrintable(QStringLiteral("key twice: %1").arg(e.key)));
                keys << e.key;
            }
        }
        // script presets: as the menu, those that need more columns than the data has are left out
        const int all = ModelCatalog::ScriptPresetEntries(0).size();
        QCOMPARE(all, int(ScriptModel::Presets().size()));
        QVERIFY(ModelCatalog::ScriptPresetEntries(1).size() < all);
        QCOMPARE(ModelCatalog::ScriptPresetEntries(2).size(), all);
        const ModelCatalog::Entry file = ModelCatalog::ModelFileEntry(QStringLiteral("/some/dir/my model.json"));
        QCOMPARE(file.title, QStringLiteral("my model"));
        QCOMPARE(file.choice.file, QStringLiteral("/some/dir/my model.json"));
    }

    // Every entry of the catalogue must lead to a model that really is added, the way ModelDataHolder::AddModel
    // adds it: a fixed model is created, a flexible one is defined from its reactions. For the titration techniques
    // the model must be complete with data of the right number of columns. Claude Generated.
    void catalogChoicesMakeModels()
    {
        using ModelCatalog::Technique;
        for (const Technique t : { Technique::NMR, Technique::UVVis, Technique::Fluorescence, Technique::Calorimetry }) {
            for (const int columns : { 1, 2, 3 }) {
                for (const ModelCatalog::Entry& e : ModelCatalog::Entries(t, columns)) {
                    if (e.kind != ModelCatalog::Kind::Stoichiometry)
                        continue;
                    QSharedPointer<DataClass> data(makeDataFor(columns));
                    QSharedPointer<AbstractModel> model = CreateModel(SupraFit::Model(e.choice.model), data.data());
                    QVERIFY2(model, qPrintable(QStringLiteral("%1 / %2 columns / %3: no model").arg(ModelCatalog::Info(t).shortName).arg(columns).arg(e.key)));
                    if (e.choice.kind == ModelCatalog::Choice::Kind::AnyModelWithReactions) {
                        QJsonObject reactions;
                        reactions["name"] = QStringLiteral("Reactions");
                        reactions["type"] = 6;
                        reactions["value"] = e.choice.reactions;
                        model->setModelDefinition(QHash<QString, QJsonObject>{ { QStringLiteral("Reactions"), reactions } });
                        QVERIFY2(model->DefineModel(QJsonObject()), qPrintable(QStringLiteral("%1 / %2: definition failed").arg(ModelCatalog::Info(t).shortName, e.key)));
                    }
                    if (t != Technique::Calorimetry) // the calorimetry models need a titration protocol to be complete
                        QVERIFY2(model->Complete(), qPrintable(QStringLiteral("%1 / %2 columns / %3: incomplete").arg(ModelCatalog::Info(t).shortName).arg(columns).arg(e.key)));
                }
            }
        }
    }
    // The catalogue merges the techniques: one entry per stoichiometry that lists every technique offering it.
    void catalogAllEntriesMergeTheTechniques()
    {
        using ModelCatalog::Technique;
        const QList<Technique> titrations{ Technique::NMR, Technique::UVVis, Technique::Fluorescence };
        int stoichiometries = 0, scripts = 0;
        for (const ModelCatalog::Entry& e : ModelCatalog::AllEntries(2)) {
            if (e.kind == ModelCatalog::Kind::Stoichiometry) {
                ++stoichiometries;
                QCOMPARE(e.techniques, titrations);
            } else if (e.kind == ModelCatalog::Kind::ScriptPreset) {
                ++scripts;
                QVERIFY(e.techniques.isEmpty());
            }
        }
        QCOMPARE(stoichiometries, 8);
        QCOMPARE(scripts, int(ScriptModel::Presets().size()));
        QCOMPARE(ModelCatalog::AllEntries(3).size() - ModelCatalog::ScriptPresetEntries(3).size(), qsizetype(2)); // the two three-component systems
        for (const ModelCatalog::Entry& e : ModelCatalog::AllEntries(1)) { // one column: calorimetry and kinetics, one technique each
            if (e.kind == ModelCatalog::Kind::Stoichiometry)
                QCOMPARE(e.techniques, (QList<Technique>{ Technique::Calorimetry }));
            if (e.kind == ModelCatalog::Kind::FixedModel) // the TIAN model is in the calorimetry and in the kinetics list, as in the old menu
                QCOMPARE(e.techniques.size(), e.key == QStringLiteral("model:%1").arg(int(SupraFit::TianModel)) ? 2 : 1);
        }
        // what an entry stands for depends on the technique chosen
        ModelCatalog::Entry oneToOne;
        for (const ModelCatalog::Entry& e : ModelCatalog::AllEntries(2))
            if (e.key == QStringLiteral("stoich:1:1"))
                oneToOne = e;
        QCOMPARE(ModelCatalog::ChoiceFor(oneToOne, Technique::NMR).model, int(SupraFit::nmr_ItoI));
        QCOMPARE(ModelCatalog::ChoiceFor(oneToOne, Technique::UVVis).model, int(SupraFit::uv_vis_ItoI));
        QCOMPARE(ModelCatalog::ChoiceFor(oneToOne, Technique::Fluorescence).model, int(SupraFit::fl_ItoI));
    }

    // ---- WP-B: counters and what the quick bar shows ------------------------------------------------------------

    static QStringList keysOf(const QList<ModelCatalog::Entry>& list)
    {
        QStringList keys;
        for (const ModelCatalog::Entry& e : list)
            keys << e.key;
        return keys;
    }

    static ModelCatalog::Entry entryFor(const QString& key, int columns = 2)
    {
        for (const ModelCatalog::Entry& e : ModelUsage::Candidates(columns))
            if (e.key == key)
                return e;
        return ModelCatalog::Entry();
    }

    // A fresh installation shows the four established stoichiometries, in the order of the catalogue, NMR for each.
    void usageStartsWithTheEstablishedStoichiometries()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        using ModelCatalog::Technique;
        const ModelUsage::Preferences p = ModelUsage::Load();
        QCOMPARE(p.mode, ModelUsage::Mode::Frequent);
        QCOMPARE(p.count, 4);
        QCOMPARE(p.fixed, ModelUsage::DefaultFixed());
        QCOMPARE(keysOf(ModelUsage::Quick(2)), (QStringList{ "stoich:1:1", "stoich:2:1/1:1", "stoich:1:1/1:2", "stoich:2:1/1:1/1:2" }));
        for (const ModelCatalog::Entry& e : ModelUsage::Quick(2))
            QCOMPARE(ModelUsage::TechniqueFor(e, 2), Technique::NMR);
        QCOMPARE(ModelUsage::DefaultTechnique(2), Technique::NMR);
    }

    // An entry is ranked by all its uses, whatever the technique; equal counts keep the catalogue's order.
    void usageRanksByCounts()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        using ModelCatalog::Technique;
        for (int i = 0; i < 3; ++i)
            ModelUsage::Record(Technique::NMR, entryFor(QStringLiteral("stoich:1:1/1:2")));
        ModelUsage::Record(Technique::NMR, entryFor(QStringLiteral("stoich:2:1/1:1")));
        QCOMPARE(ModelUsage::Used(Technique::NMR, QStringLiteral("stoich:1:1/1:2")), 3);
        QCOMPARE(ModelUsage::UsedTotal(QStringLiteral("stoich:1:1/1:2")), 3);
        QCOMPARE(keysOf(ModelUsage::Quick(2)), (QStringList{ "stoich:1:1/1:2", "stoich:2:1/1:1", "stoich:1:1", "stoich:2:1/1:1/1:2" }));
        // uses with another technique count for the entry (1:1 with UV/Vis five times: it moves up)
        for (int i = 0; i < 5; ++i)
            ModelUsage::Record(Technique::UVVis, entryFor(QStringLiteral("stoich:1:1")));
        QCOMPARE(ModelUsage::UsedTotal(QStringLiteral("stoich:1:1")), 5);
        QCOMPARE(ModelUsage::Used(Technique::NMR, QStringLiteral("stoich:1:1")), 0);
        QCOMPARE(keysOf(ModelUsage::Quick(2)).first(), QStringLiteral("stoich:1:1"));
        // the number shown is a preference
        ModelUsage::Preferences p = ModelUsage::Load();
        p.count = 2;
        ModelUsage::Save(p);
        QCOMPARE(keysOf(ModelUsage::Quick(2)), (QStringList{ "stoich:1:1", "stoich:1:1/1:2" }));
        ModelUsage::Reset();
        QCOMPARE(ModelUsage::UsedTotal(QStringLiteral("stoich:1:1")), 0);
        QSettings().remove(QStringLiteral("add_model_bar"));
    }

    // A click on an entry uses the technique it was used with most; never used: the fixed technique, else the one used
    // last, else NMR. A scripted preset has no technique and leaves the one used last alone.
    void usageTechniqueOfAnEntry()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        using ModelCatalog::Technique;
        const QString one = QStringLiteral("stoich:1:1");
        const QString two = QStringLiteral("stoich:1:1/1:2");
        QCOMPARE(ModelUsage::TechniqueFor(entryFor(one), 2), Technique::NMR); // the default
        ModelUsage::Record(Technique::UVVis, entryFor(one));
        QCOMPARE(ModelUsage::TechniqueFor(entryFor(one), 2), Technique::UVVis); // used with UV/Vis
        ModelUsage::Record(Technique::NMR, entryFor(one));
        ModelUsage::Record(Technique::NMR, entryFor(one));
        QCOMPARE(ModelUsage::TechniqueFor(entryFor(one), 2), Technique::NMR); // NMR now more often
        // another entry that never was used follows the technique used last (NMR was last here)
        QCOMPARE(ModelUsage::TechniqueFor(entryFor(two), 2), Technique::NMR);
        ModelUsage::Record(Technique::Fluorescence, entryFor(QStringLiteral("stoich:2:1/1:1")));
        QCOMPARE(ModelUsage::TechniqueFor(entryFor(two), 2), Technique::Fluorescence);
        // a scripted preset is counted for all techniques and does not change the one used last
        const ModelCatalog::Entry hill = entryFor(QStringLiteral("script:Hill"));
        QVERIFY(hill.choice.valid() && hill.techniques.isEmpty());
        ModelUsage::Record(Technique::UVVis, hill);
        QCOMPARE(ModelUsage::TechniqueFor(entryFor(two), 2), Technique::Fluorescence);
        QCOMPARE(ModelUsage::Used(Technique::NMR, hill.key), 1);
        // a fixed technique beats the one used last, for entries never used with any
        ModelUsage::Preferences p = ModelUsage::Load();
        p.technique = ModelCatalog::Info(Technique::UVVis).key;
        ModelUsage::Save(p);
        QCOMPARE(ModelUsage::TechniqueFor(entryFor(two), 2), Technique::UVVis);
        QCOMPARE(ModelUsage::TechniqueFor(entryFor(one), 2), Technique::NMR); // ... not for one used with another
        // one column: the entries have calorimetry only
        QCOMPARE(ModelUsage::TechniqueFor(entryFor(one, 1), 1), Technique::Calorimetry);
        QCOMPARE(ModelUsage::DefaultTechnique(1), Technique::Calorimetry);
        QSettings().remove(QStringLiteral("add_model_bar"));
    }

    // In the fixed mode the bar shows the stored entries, in that order; what the data does not offer is left out.
    // Scripted presets and model files can be among them.
    void usageFixedListIsShownAsStored()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        using ModelCatalog::Technique;
        QTemporaryDir dir;
        QFile file(dir.filePath(QStringLiteral("my model.json")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{}");
        file.close();
        ModelUsage::Record(Technique::NMR, ModelCatalog::ModelFileEntry(file.fileName())); // a file is known by having been used

        ModelUsage::Preferences p;
        p.mode = ModelUsage::Mode::Fixed;
        p.fixed = { QStringLiteral("stoich:1:1/1:2"), QStringLiteral("script:Hill"), QStringLiteral("stoich:2:1/1:1"), QStringLiteral("file:") + file.fileName(),
            QStringLiteral("stoich:no such"), QStringLiteral("file:/does/not/exist.json") };
        ModelUsage::Save(p);
        QCOMPARE(keysOf(ModelUsage::Quick(2)), (QStringList{ "stoich:1:1/1:2", "script:Hill", "stoich:2:1/1:1", QStringLiteral("file:") + file.fileName() }));
        // one column: the host/guest systems of calorimetry stay, the single-input preset and the file too
        QVERIFY(keysOf(ModelUsage::Quick(1)).contains(QStringLiteral("script:Hill")));
        QSettings().remove(QStringLiteral("add_model_bar"));
    }

    // ---- WP-C: the Add-Model bar --------------------------------------------------------------------------------

    static QToolButton* barButton(AddModelBar& bar, const QString& entryKey)
    {
        for (QToolButton* b : bar.findChildren<QToolButton*>())
            if (b->property("entryKey").toString() == entryKey)
                return b;
        return nullptr;
    }

private: // moc takes everything after "private slots:" for slots: a struct needs its own section
    struct BarFixture {
        QSharedPointer<DataClass> data;
        ChartWidget chart;
        ModelDataHolder holder;
        AddModelBar bar;
        explicit BarFixture(int columns)
            : data(makeDataFor(columns))
        {
            chart.setRawData(data);
            holder.setChartWidget(&chart);
            holder.setData(data, QSharedPointer<ChartWrapper>::create());
            bar.setWorkspace(holder.TitleBarWidget());
        }
    };
private slots:

    // The bar shows the models added most as buttons; a click adds the model of the technique the entry was used
    // with most, the menu of the arrow offers the other techniques. Claude Generated.
    void addModelBarAddsTheModelOfTheEntrysTechnique()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        using ModelCatalog::Technique;
        BarFixture f(2);
        AddModelBar& bar = f.bar;
        QVERIFY(bar.isEnabled());

        // the four established stoichiometries, each with a menu of the three techniques; NMR is the one a click uses
        for (const QString& key : QStringList{ "stoich:1:1", "stoich:2:1/1:1", "stoich:1:1/1:2", "stoich:2:1/1:1/1:2" }) {
            QToolButton* b = barButton(bar, key);
            QVERIFY2(b, qPrintable(key));
            QVERIFY(b->menu() && b->menu()->actions().size() == 3);
            QCOMPARE(b->property("technique").toInt(), int(Technique::NMR));
        }
        QCOMPARE(barButton(bar, QStringLiteral("stoich:1:1"))->text(), QStringLiteral("1:1 \u00b7 NMR")); // the technique is always named
        QVERIFY(!barButton(bar, QStringLiteral("stoich:1:1+A2"))); // not among the most used

        // a click adds the NMR model
        barButton(bar, QStringLiteral("stoich:1:1/1:2"))->click();
        QCoreApplication::processEvents();
        QCOMPARE(f.holder.ModelCount(), 1);
        QCOMPARE(int(f.holder.Model(0)->SFModel()), int(SupraFit::nmr_ItoI_ItoII));
        QCOMPARE(ModelUsage::Used(Technique::NMR, QStringLiteral("stoich:1:1/1:2")), 1);

        // the menu adds it for another technique
        QAction* uvvis = nullptr;
        for (QAction* a : barButton(bar, QStringLiteral("stoich:1:1/1:2"))->menu()->actions())
            if (a->data().toInt() == int(Technique::UVVis))
                uvvis = a;
        QVERIFY(uvvis);
        uvvis->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(f.holder.ModelCount(), 2);
        QCOMPARE(int(f.holder.Model(1)->SFModel()), int(SupraFit::uv_vis_ItoI_ItoII));

        // the buttons do not change under the pointer; after a Refresh the click uses what was used most
        QCOMPARE(barButton(bar, QStringLiteral("stoich:1:1/1:2"))->property("technique").toInt(), int(Technique::NMR));
        uvvis->trigger(); // UV/Vis twice now, NMR once
        QCoreApplication::processEvents();
        bar.Refresh();
        QToolButton* b = barButton(bar, QStringLiteral("stoich:1:1/1:2"));
        QCOMPARE(b->property("technique").toInt(), int(Technique::UVVis));
        QCOMPARE(b->text(), QStringLiteral("1:1 / 1:2 \u00b7 UV/Vis")); // it says which technique a click uses
        QCOMPARE(ModelUsage::DefaultTechnique(2), Technique::UVVis);

        // without a project the bar shows nothing
        bar.setWorkspace(nullptr);
        QVERIFY(!bar.isEnabled());
        QVERIFY(bar.findChildren<QToolButton*>().size() <= 1); // the gear is hidden
        QSettings().remove(QStringLiteral("add_model_bar"));
    }

    // Showing and hiding: the master checkbox of a model, the "Show in Plot" of one of its series, "Include" and "Single Plot" are independent of the
    // other models and of the data series. (Before, hiding one model hid every model and the data, "Include" did not reach the fit, and "Single Plot"
    // did nothing.) Claude Generated.
    void modelsAndSeriesAreShownAndHiddenOneByOne()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        BarFixture f(2);
        for (const QString& key : QStringList{ "stoich:1:1", "stoich:2:1/1:1", "stoich:1:1/1:2" }) {
            barButton(f.bar, key)->click();
            QCoreApplication::processEvents();
        }
        QCOMPARE(f.holder.ModelCount(), 3);
        const QList<ModelWidget*> widgets = f.holder.findChildren<ModelWidget*>();
        QCOMPARE(widgets.size(), 3);
        QList<QCheckBox*> masters;
        for (ModelWidget* w : widgets) { // the master checkbox of the tab of each model
            QCheckBox* master = nullptr;
            for (QCheckBox* c : f.holder.findChildren<QCheckBox*>(QStringLiteral("model_curves_toggle")))
                if (!masters.contains(c) && !master)
                    master = c;
            QVERIFY(master);
            masters << master;
            Q_UNUSED(w);
        }
        // which master belongs to which widget: the tab order is the order of the models
        auto visible = [&](ModelWidget* w, bool error, int series) {
            Charts ch = w->Chart();
            return (error ? ch.error_wrapper : ch.signal_wrapper)->Series(series)->isVisible();
        };
        auto dataVisible = [&](int series) { return widgets[0]->Chart().data_wrapper->Series(series)->isVisible(); };
        auto allShown = [&](ModelWidget* w) { return visible(w, false, 0) && visible(w, false, 1) && visible(w, true, 0) && visible(w, true, 1); };
        auto noneShown = [&](ModelWidget* w) { return !visible(w, false, 0) && !visible(w, false, 1) && !visible(w, true, 0) && !visible(w, true, 1); };
        auto elementsOf = [&](ModelWidget* w) { return w->findChildren<ModelElement*>(); };
        auto box = [&](ModelElement* e, const QString& text) -> QCheckBox* {
            for (QCheckBox* c : e->findChildren<QCheckBox*>())
                if (c->text() == text)
                    return c;
            return nullptr;
        };
        for (ModelWidget* w : widgets)
            QVERIFY(allShown(w));
        QVERIFY(dataVisible(0) && dataVisible(1));

        // 1. the master checkbox of one model hides this model, nothing else
        for (int m = 0; m < 3; ++m) {
            QCheckBox* master = masters[m];
            // find the widget this master belongs to: hiding it must hide exactly one model
            master->setChecked(false);
            QCoreApplication::processEvents();
            int hidden = 0, hiddenIndex = -1;
            for (int k = 0; k < 3; ++k)
                if (noneShown(widgets[k])) {
                    ++hidden;
                    hiddenIndex = k;
                }
            QVERIFY2(hidden == 1, qPrintable(QString("hiding model %1 hid %2 models").arg(m).arg(hidden)));
            for (int k = 0; k < 3; ++k)
                if (k != hiddenIndex)
                    QVERIFY2(allShown(widgets[k]), "another model was hidden");
            QVERIFY2(dataVisible(0) && dataVisible(1), "hiding a model hid the data series");
            QVERIFY2(!widgets[hiddenIndex]->isChecked(), "the unchecked master box does not take the model out of the analyses");
            master->setChecked(true);
            QVERIFY(widgets[hiddenIndex]->isChecked());
            QCoreApplication::processEvents();
            for (ModelWidget* w : widgets)
                QVERIFY(allShown(w));
        }

        ModelWidget* second = widgets[1];
        const QList<ModelElement*> els = elementsOf(second);
        QCOMPARE(els.size(), 2);

        // 2. "Show in Plot" of one series of one model: only that curve (signal and error); the master checkbox becomes partly checked
        QCheckBox* show0 = box(els[0], QStringLiteral("Show in Plot"));
        QVERIFY(show0);
        show0->setChecked(false);
        QCoreApplication::processEvents();
        QVERIFY(!visible(second, false, 0) && !visible(second, true, 0));
        QVERIFY(visible(second, false, 1) && visible(second, true, 1));
        QVERIFY(allShown(widgets[0]) && allShown(widgets[2]));
        QVERIFY(dataVisible(0) && dataVisible(1));
        int partly = 0;
        for (QCheckBox* c : masters)
            partly += c->checkState() == Qt::PartiallyChecked;
        QCOMPARE(partly, 1);
        show0->setChecked(true);
        QCoreApplication::processEvents();
        QVERIFY(allShown(second));
        for (QCheckBox* c : masters)
            QCOMPARE(c->checkState(), Qt::Checked);

        // hiding the curves one by one leaves the model in use (Optimise All, Compare, Statistics): the master box is partly checked, not unchecked
        QCheckBox* show1 = box(els[1], QStringLiteral("Show in Plot"));
        show0->setChecked(false);
        show1->setChecked(false);
        QCoreApplication::processEvents();
        QVERIFY(noneShown(second));
        QVERIFY2(second->isChecked(), "hiding every curve one by one took the model out of the analyses");
        show0->setChecked(true);
        show1->setChecked(true);
        QCoreApplication::processEvents();
        QVERIFY(allShown(second));

        // 3. "Include" belongs to the fit of this model: ActiveSignals follow, the curves of this series of this model go, nothing else
        QCheckBox* include1 = box(els[1], QStringLiteral("Include"));
        QVERIFY(include1);
        QVERIFY(second->Model()->ActiveSignals(1));
        include1->click();
        QCoreApplication::processEvents();
        QVERIFY2(!second->Model()->ActiveSignals(1), "Include did not reach the fit of the model");
        QVERIFY(second->Model()->ActiveSignals(0));
        QVERIFY(!visible(second, false, 1) && !visible(second, true, 1));
        QVERIFY(visible(second, false, 0) && visible(second, true, 0));
        QVERIFY(widgets[0]->Model()->ActiveSignals(1) && widgets[2]->Model()->ActiveSignals(1));
        QVERIFY(allShown(widgets[0]) && allShown(widgets[2]));
        QVERIFY2(dataVisible(0) && dataVisible(1), "Include hid a data series");
        // the master checkbox only counts the series that take part: this model shows all of them
        for (QCheckBox* c : masters)
            QCOMPARE(c->checkState(), Qt::Checked);
        include1->click();
        QCoreApplication::processEvents();
        QVERIFY(second->Model()->ActiveSignals(1));
        QVERIFY(allShown(second));

        // 4. "Single Plot": only this series in the data chart and in every model chart; nothing that was hidden comes back (a hidden model, a series
        // that was unchecked), and when it is switched off everything is as it was
        masters[2]->setChecked(false);
        QCoreApplication::processEvents();
        ModelWidget* hiddenModel = nullptr;
        for (ModelWidget* w : widgets)
            if (noneShown(w))
                hiddenModel = w;
        QVERIFY(hiddenModel);
        ModelWidget* thirdModel = nullptr; // a visible model of which one series is unchecked
        for (ModelWidget* w : widgets)
            if (w != hiddenModel && w != second)
                thirdModel = w;
        QVERIFY(thirdModel);
        QCheckBox* thirdShow0 = box(elementsOf(thirdModel)[0], QStringLiteral("Show in Plot"));
        thirdShow0->setChecked(false); // series 0 of the third model is off
        QCoreApplication::processEvents();
        QPushButton* single = nullptr;
        for (QPushButton* b : els[0]->findChildren<QPushButton*>())
            if (b->text() == QStringLiteral("Single Plot"))
                single = b;
        QVERIFY(single);
        single->click(); // series 0 of the second model's card
        QCoreApplication::processEvents();
        QVERIFY(dataVisible(0) && !dataVisible(1));
        QVERIFY(noneShown(hiddenModel)); // the hidden model stays hidden
        QVERIFY2(!visible(thirdModel, false, 0) && !visible(thirdModel, true, 0), "an unchecked series came back with the single plot");
        QVERIFY(!visible(thirdModel, false, 1) && !visible(thirdModel, true, 1)); // series 1 is not the single one
        QVERIFY(visible(second, false, 0) && visible(second, true, 0) && !visible(second, false, 1) && !visible(second, true, 1));
        // a model switched on during the single plot shows the one series only, and stays so afterwards
        for (QCheckBox* c : masters)
            if (c->checkState() == Qt::Unchecked)
                c->setChecked(true);
        QCoreApplication::processEvents();
        QVERIFY(visible(hiddenModel, false, 0) && visible(hiddenModel, true, 0));
        QVERIFY2(!visible(hiddenModel, false, 1) && !visible(hiddenModel, true, 1), "a model switched on during the single plot shows another series");
        single->click();
        QCoreApplication::processEvents();
        QVERIFY(dataVisible(0) && dataVisible(1));
        QVERIFY(allShown(hiddenModel) && allShown(second));
        QVERIFY2(!visible(thirdModel, false, 0) && visible(thirdModel, false, 1), "the unchecked series of a model is not as it was after the single plot");
        thirdShow0->setChecked(true);
        QCoreApplication::processEvents();
        QVERIFY(allShown(thirdModel));
        QSettings().remove(QStringLiteral("add_model_bar"));
    }

    // The fixed list: a dimerisation system is the technique's flexible model with its reactions (no dialog), a
    // scripted preset is a plain button without technique menu. Claude Generated.
    void addModelBarShowsAFixedList()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        using ModelCatalog::Technique;
        BarFixture f(2);
        ModelUsage::Preferences p;
        p.mode = ModelUsage::Mode::Fixed;
        p.fixed = { QStringLiteral("script:Hill"), QStringLiteral("stoich:1:1+A2") };
        ModelUsage::Save(p);
        f.bar.Refresh();
        QVERIFY(barButton(f.bar, QStringLiteral("script:Hill")));
        QVERIFY(!barButton(f.bar, QStringLiteral("script:Hill"))->menu()); // no technique to choose
        QVERIFY(!barButton(f.bar, QStringLiteral("stoich:2:1/1:1")));

        barButton(f.bar, QStringLiteral("script:Hill"))->click();
        QCoreApplication::processEvents();
        QCOMPARE(f.holder.ModelCount(), 1);
        QCOMPARE(int(f.holder.Model(0)->SFModel()), int(SupraFit::ScriptModel));
        QCOMPARE(ModelUsage::DefaultTechnique(2), Technique::NMR); // a scripted preset does not change the technique

        barButton(f.bar, QStringLiteral("stoich:1:1+A2"))->click(); // NMR: the flexible model
        QCoreApplication::processEvents();
        QCOMPARE(f.holder.ModelCount(), 2);
        QCOMPARE(int(f.holder.Model(1)->SFModel()), int(SupraFit::nmr_any));
        QAction* fl = nullptr;
        for (QAction* a : barButton(f.bar, QStringLiteral("stoich:1:1+A2"))->menu()->actions())
            if (a->data().toInt() == int(Technique::Fluorescence))
                fl = a;
        QVERIFY(fl);
        fl->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(int(f.holder.Model(2)->SFModel()), int(SupraFit::fl_any));
        QSettings().remove(QStringLiteral("add_model_bar"));
    }

    void addModelBarRenders()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        BarFixture f(2);
        for (int i = 0; i < 3; ++i) // UV/Vis for the second one, to show a technique in the text
            ModelUsage::Record(ModelCatalog::Technique::UVVis, entryFor(QStringLiteral("stoich:2:1/1:1")));
        f.bar.Refresh();
        f.bar.resize(700, 40);
        f.bar.show();
        QTest::qWaitForWindowExposed(&f.bar);
        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        if (!renderDir.isEmpty()) {
            QDir().mkpath(renderDir);
            f.bar.grab().save(QStringLiteral("%1/add_model_bar.png").arg(renderDir));
        }
        QVERIFY(!f.bar.grab().isNull());
        QSettings().remove(QStringLiteral("add_model_bar"));
    }
    // The main toolbar in three zones: files and menus on the left, the models that are added most in the middle, the
    // docks and the program menu on the right. With a project loaded the middle shows the model buttons. Claude Generated.
    void mainToolbarHasThreeZones()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        const QString project = QStringLiteral(SUPRAFIT_SOURCE_DIR "/data/samples/reference/simulated_1_1.json");
        if (!QFileInfo::exists(project))
            QSKIP("sample project not found");

        // fresh settings start the first-start dialog (modal, it would wait for ever): the setting exists, and
        // anything else that is modal is closed and counted
        {
            QSettings settings;
            settings.beginGroup(QStringLiteral("main"));
            settings.setValue(QStringLiteral("advanced_ui"), false);
        }
        QStringList dialogs;
        QTimer closer;
        closer.setInterval(100);
        QObject::connect(&closer, &QTimer::timeout, [&dialogs]() {
            if (QWidget* w = QApplication::activeModalWidget()) {
                dialogs << w->windowTitle();
                w->close();
            }
        });
        closer.start();

        // the main window takes the global Instance with it when it goes: the next test needs a new one (declared before
        // the window, so it runs after the window's destructor)
        struct ReinstallInstance {
            ~ReinstallInstance() { Instance::setInstance(new Instance); }
        } reinstallInstance;

        SupraFitGui gui;
        gui.resize(1500, 700);
        gui.show();
        QTest::qWaitForWindowExposed(&gui);
        gui.LoadFile(project);
        AddModelBar* bar = gui.findChild<AddModelBar*>();
        QVERIFY(bar);
        QTRY_VERIFY2(!bar->findChildren<QToolButton*>().isEmpty() && bar->isEnabled(), "no model buttons after loading a project");
        QCoreApplication::processEvents();

        QToolBar* toolbar = gui.findChild<QToolBar*>(QStringLiteral("main_toolbar"));
        QVERIFY(toolbar);
        auto leftOf = [toolbar](const QWidget* w) { return w->mapTo(toolbar, QPoint(0, 0)).x(); };
        QToolButton* supra = nullptr;
        QToolButton* newTable = nullptr;
        for (QToolButton* b : toolbar->findChildren<QToolButton*>()) {
            if (b->text() == QStringLiteral("SupraFit"))
                supra = b;
            if (b->defaultAction() && b->defaultAction()->text().contains(QStringLiteral("New Table")))
                newTable = b;
        }
        QVERIFY2(dialogs.isEmpty(), qPrintable(QStringLiteral("a modal dialog came up: %1").arg(dialogs.join(", "))));
        QVERIFY2(supra && newTable, "toolbar buttons not found");
        const int barLeft = leftOf(bar), barRight = barLeft + bar->width();
        QVERIFY2(leftOf(newTable) + newTable->width() < barLeft, "the file buttons are not left of the models");
        QVERIFY2(barRight < leftOf(supra), "the program menu is not right of the models");
        QVERIFY2(qAbs((barLeft + barRight) / 2 - toolbar->width() / 2) < toolbar->width() / 4, "the model buttons are not in the middle");

        const QString renderDir = qEnvironmentVariable("SUPRAFIT_LAYOUT_RENDER_DIR");
        if (!renderDir.isEmpty()) {
            QDir().mkpath(renderDir);
            toolbar->grab().save(QStringLiteral("%1/main_toolbar.png").arg(renderDir));
        }
        QSettings().remove(QStringLiteral("add_model_bar"));
    }
    // ---- WP-D: customising the quick bar --------------------------------------------------------------------

    // The dialog hands back what the operator set: the mode, how many, the ticked entries in their order, the technique.
    void quickBarDialogReturnsWhatWasSet()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        ModelUsage::Preferences start;
        start.fixed = { QStringLiteral("stoich:1:1"), QStringLiteral("stoich:2:1/1:1") };
        QuickBarDialog dialog(start);

        // as stored: most used, 4, and the ticked entries come first, in order
        QCOMPARE(dialog.preferences().mode, ModelUsage::Mode::Frequent);
        QCOMPARE(dialog.preferences().count, 4);
        QCOMPARE(dialog.preferences().fixed, (QStringList{ "stoich:1:1", "stoich:2:1/1:1" }));
        QCOMPARE(dialog.preferences().technique, QString());
        QListWidget* list = dialog.findChild<QListWidget*>(QStringLiteral("fixed_list"));
        QVERIFY(list && list->count() > 10); // everything that can be added
        QVERIFY2(!list->isEnabled(), "the list can be edited in the frequent mode");

        // set a fixed list: tick the Hill preset, move it to the top, choose the technique and the count
        dialog.findChild<QRadioButton*>(QStringLiteral("fixed"))->setChecked(true);
        QVERIFY(list->isEnabled());
        int hill = -1;
        for (int i = 0; i < list->count(); ++i)
            if (list->item(i)->data(Qt::UserRole).toString() == QStringLiteral("script:Hill")) {
                hill = i;
                list->item(i)->setCheckState(Qt::Checked);
            }
        QVERIFY(hill >= 0);
        list->setCurrentRow(hill);
        for (int i = hill; i > 0; --i)
            dialog.findChild<QPushButton*>(QStringLiteral("up"))->click();
        QCOMPARE(list->item(0)->data(Qt::UserRole).toString(), QStringLiteral("script:Hill"));
        QComboBox* technique = dialog.findChild<QComboBox*>(QStringLiteral("technique"));
        technique->setCurrentIndex(technique->findData(ModelCatalog::Info(ModelCatalog::Technique::UVVis).key));
        dialog.findChild<QSpinBox*>(QStringLiteral("count"))->setValue(6);

        const ModelUsage::Preferences p = dialog.preferences();
        QCOMPARE(p.mode, ModelUsage::Mode::Fixed);
        QCOMPARE(p.count, 6);
        QCOMPARE(p.fixed, (QStringList{ "script:Hill", "stoich:1:1", "stoich:2:1/1:1" }));
        QCOMPARE(p.technique, ModelCatalog::Info(ModelCatalog::Technique::UVVis).key);

        // what is saved is what the bar shows
        ModelUsage::Save(p);
        QCOMPARE(keysOf(ModelUsage::Quick(2)), (QStringList{ "script:Hill", "stoich:1:1", "stoich:2:1/1:1" }));
        QCOMPARE(ModelUsage::TechniqueFor(entryFor(QStringLiteral("stoich:1:1/1:2")), 2), ModelCatalog::Technique::UVVis);
        QSettings().remove(QStringLiteral("add_model_bar"));
    }

    // The right-click menu of a button: remove it from a fixed list, or fix the bar as it is.
    void addModelBarContextMenu()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        BarFixture f(2);
        auto trigger = [&f](const QString& entryKey, const QString& text) {
            QMenu* menu = f.bar.ContextMenu(entryKey);
            QAction* found = nullptr;
            for (QAction* a : menu->actions())
                if (a->text() == text)
                    found = a;
            const bool ok = found != nullptr;
            if (ok)
                found->trigger();
            menu->deleteLater();
            return ok;
        };
        // the most used: no "Remove", but the bar can be fixed as it is (the four established stoichiometries)
        QVERIFY(!trigger(QStringLiteral("stoich:1:1"), QStringLiteral("Remove from the bar")));
        QVERIFY(trigger(QStringLiteral("stoich:1:1"), QStringLiteral("Fix the bar as it is")));
        QCOMPARE(ModelUsage::Load().mode, ModelUsage::Mode::Fixed);
        QCOMPARE(ModelUsage::Load().fixed, (QStringList{ "stoich:1:1", "stoich:2:1/1:1", "stoich:1:1/1:2", "stoich:2:1/1:1/1:2" }));
        // a fixed list: remove an entry, the button goes
        QVERIFY(barButton(f.bar, QStringLiteral("stoich:2:1/1:1")));
        QVERIFY(trigger(QStringLiteral("stoich:2:1/1:1"), QStringLiteral("Remove from the bar")));
        QVERIFY(!barButton(f.bar, QStringLiteral("stoich:2:1/1:1")));
        QCOMPARE(ModelUsage::Load().fixed, (QStringList{ "stoich:1:1", "stoich:1:1/1:2", "stoich:2:1/1:1/1:2" }));
        QVERIFY(!trigger(QStringLiteral("stoich:1:1"), QStringLiteral("Fix the bar as it is"))); // already fixed
        QSettings().remove(QStringLiteral("add_model_bar"));
    }
    // The Model menu goes through the same catalogue: what is added there is counted, and a right click puts the
    // entry on the quick bar. Claude Generated.
    void modelMenuCountsAndPins()
    {
        QSettings().remove(QStringLiteral("add_model_bar"));
        using ModelCatalog::Technique;
        // every entry is found again from the choice that adds it
        for (const ModelCatalog::Entry& entry : ModelCatalog::AllEntries(0)) {
            if (entry.kind == ModelCatalog::Kind::ScriptPreset) {
                ModelCatalog::Entry back;
                Technique t = Technique::NMR;
                QVERIFY2(ModelCatalog::Identify(entry.choice, &back, &t), qPrintable(entry.key));
                QCOMPARE(back.key, entry.key);
                continue;
            }
            for (const Technique t : entry.techniques) {
                ModelCatalog::Entry back;
                Technique found = Technique::NMR;
                QVERIFY2(ModelCatalog::Identify(ModelCatalog::ChoiceFor(entry, t), &back, &found), qPrintable(entry.key));
                QCOMPARE(back.key, entry.key);
                // the Tian model is offered by two techniques with the same choice: either one will do
                QVERIFY(entry.techniques.contains(found));
                QCOMPARE(ModelCatalog::ChoiceFor(entry, found).model, ModelCatalog::ChoiceFor(entry, t).model);
            }
        }
        ModelCatalog::Choice own; // a model with its own reactions is none of the entries
        own.kind = ModelCatalog::Choice::Kind::AnyModelWithReactions;
        own.model = SupraFit::nmr_any;
        own.reactions = QStringLiteral("A + B = AB\nAB + A = A2B");
        ModelCatalog::Entry ignored;
        Technique ignoredT = Technique::NMR;
        QVERIFY(!ModelCatalog::Identify(own, &ignored, &ignoredT));

        BarFixture f(2);
        MDHDockTitleBar* title = f.holder.TitleBarWidget();
        QAction* uvvis = nullptr;
        QAction* nmrPreset = nullptr;
        for (QMenu* menu : title->findChildren<QMenu*>())
            for (QAction* a : menu->actions()) {
                if (a->data().isValid() && a->data().toInt() == SupraFit::uv_vis_IItoI_ItoI && !a->menu())
                    uvvis = a;
                // a preset the bar does not show from the start
                if (a->property("preset_reactions").isValid() && !nmrPreset && a->data().toInt() == SupraFit::nmr_any
                    && !title->EntryKeyOf(a).isEmpty() && !ModelUsage::DefaultFixed().contains(title->EntryKeyOf(a)))
                    nmrPreset = a;
            }
        QVERIFY(uvvis);
        QCOMPARE(title->EntryKeyOf(uvvis), QStringLiteral("stoich:2:1/1:1"));
        QVERIFY(nmrPreset);
        QVERIFY(!title->EntryKeyOf(nmrPreset).isEmpty());

        // using the menu entry counts for UV/Vis, and the bar is told
        QSignalSpy changed(title, &MDHDockTitleBar::UsageChanged);
        uvvis->trigger();
        QCOMPARE(ModelUsage::Used(Technique::UVVis, QStringLiteral("stoich:2:1/1:1")), 1);
        QCOMPARE(changed.count(), 1);

        // pinning: the frequent bar becomes the fixed list, the entry is added once
        QVERIFY(ModelUsage::Pin(title->EntryKeyOf(nmrPreset), 2));
        const ModelUsage::Preferences p = ModelUsage::Load();
        QCOMPARE(p.mode, ModelUsage::Mode::Fixed);
        QCOMPARE(p.fixed.last(), title->EntryKeyOf(nmrPreset));
        QCOMPARE(p.fixed.size(), 5);
        QVERIFY(!ModelUsage::Pin(title->EntryKeyOf(nmrPreset), 2));
        QSettings().remove(QStringLiteral("add_model_bar"));
    }
    // Data (left) and Evaluation (right) sit next to the tabs and use the project's actions. Claude Generated.
    void dataAndEvaluationSitNextToTheTabs()
    {
        BarFixture f(2);
        QToolButton* data = f.holder.findChild<QToolButton*>(QStringLiteral("data_corner"));
        QToolButton* evaluation = f.holder.findChild<QToolButton*>(QStringLiteral("evaluation_corner"));
        QVERIFY(data && evaluation);
        QCOMPARE(data->defaultAction()->text(), QStringLiteral("Edit Data"));
        QVERIFY(data->menu() == f.holder.TitleBarWidget()->DataMenu());
        QVERIFY(evaluation->menu() == f.holder.TitleBarWidget()->EvaluationMenu());
        QCOMPARE(data->isEnabled(), f.holder.TitleBarWidget()->ToolsAvailable());
        f.holder.TitleBarWidget()->HideModelTools();
        QVERIFY(!data->isEnabled());
        QVERIFY(!evaluation->isEnabled());
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
