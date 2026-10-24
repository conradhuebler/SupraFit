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
#include <QtCore/QJsonObject>
#include <QtCore/QThread>
#include <QtGui/QPixmap>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

#include <Eigen/Dense>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
#include "src/global.h"

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
};

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    TestModelWidgetLayout test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_modelwidget_layout.moc"
