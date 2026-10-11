/*
 * SupraFit - offscreen render-time benchmark for the Monte-Carlo results widget
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
 * Measures the cost of building the Monte-Carlo results widget (ResultsWidget ->
 * MCResultsWidget::setUi) without a visible GUI. Motivation: after an MC run on the fixed
 * 1:1/1:2 NMR model the widget build took 4286 ms against 30 ms for scripted models on
 * identical inputs (SESSION_HANDOFF_MC_PERF.md §3); the split inside setUi could not be
 * measured by feel. This tool loads a fitted project, reruns MC headless through the real
 * JobManager (so controller["raw"] is populated exactly as in the GUI), then constructs the
 * widget under the PhaseTiming marks. The widget build is repeated on the same statistic
 * blob because construction is deterministic, unlike the thread-pooled MC run.
 *
 * The measured cost is object construction, which in the GUI happens before the first
 * paint (the phase mark sits before the dialog is shown), so the offscreen platform
 * measures the real quantity. Claude Generated.
 *
 * Usage: benchmark_mcwidget [project.json] [model-index] [rebuilds]
 *        defaults: input/multiple_statistic.json (fixed ¹H 1:1/1:2, 2000 MC steps), 0, 3
 *
 * The default fixture is a 21 MB local file and deliberately NOT in the repository - pass any
 * saved project instead. Any fitted model works; the job is re-run from MonteCarloConfigBlock.
 */

#include <QtWidgets/QApplication>

#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>
#include <QtCore/QThread>

#include <cstdio>
#include <cstdlib>

#include "src/capabilities/jobmanager.h"
#include "src/core/jsonhandler.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/phasetiming.h"
#include "src/core/projectmanager.h"
#include "src/global.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/instance.h"
#include "src/ui/widgets/results/resultswidget.h"

namespace {

// Resolve the project file: explicit argument, then CWD-relative, then source-tree default.
QString resolveProject(int argc, char** argv)
{
    if (argc > 1)
        return QString::fromLocal8Bit(argv[1]);
    const QString relative = QStringLiteral("input/multiple_statistic.json");
    if (QFileInfo::exists(relative))
        return relative;
    return QStringLiteral(SUPRAFIT_SOURCE_DIR) + "/" + relative;
}

} // namespace

int main(int argc, char** argv)
{
    // Both must be set before QApplication/the first PhaseTiming::Enabled() call.
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("SUPRAFIT_PHASE_TIMING", "1");

    QApplication app(argc, argv);

    // Minimal stand-in for the GUI bootstrap: the chart stack reads these qApp properties,
    // and MCResultsWidget dereferences Instance::GlobalInstance() unconditionally.
    app.setProperty("threads", QThread::idealThreadCount());
    app.setProperty("lineWidth", 20);
    Instance::setInstance(new Instance);

    const QString file = resolveProject(argc, argv);
    const int modelIndex = argc > 2 ? std::atoi(argv[2]) : 0;
    const int rebuilds = argc > 3 ? std::atoi(argv[3]) : 3;

    const QJsonObject root = JsonHandler::LoadFile(file);
    if (root.isEmpty()) {
        std::fprintf(stderr, "could not load project '%s'\n"
                             "pass a saved SupraFit project as the first argument\n",
            qPrintable(file));
        return 1;
    }

    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();
    const QString projectId = pm.loadProjectFromJson(root, QFileInfo(file).fileName());
    if (projectId.isEmpty()) {
        std::fprintf(stderr, "loadProjectFromJson failed for '%s'\n", qPrintable(file));
        return 1;
    }
    const QVector<QSharedPointer<AbstractModel>> models = pm.getProjectModels(projectId);
    if (modelIndex < 0 || modelIndex >= models.size()) {
        std::fprintf(stderr, "model index %d out of range (project has %lld models)\n",
            modelIndex, static_cast<long long>(models.size()));
        return 1;
    }
    QSharedPointer<AbstractModel> model = models[modelIndex];
    model->Calculate();
    std::printf("project '%s', model '%s' (id %d), SSE %g\n", qPrintable(file),
        qPrintable(model->Name()), static_cast<int>(model->SFModel()), model->SSE());

    // Full pipeline once: MC through the real JobManager, then the widget build. This is
    // the run whose phase report answers the MakeHistogram/MakeScatter split.
    PhaseTiming::Begin(QStringLiteral("MC + results widget for %1").arg(model->Name()));
    JobManager* jobmanager = new JobManager;
    jobmanager->setModel(model);
    jobmanager->AddSingleJob(MonteCarloConfigBlock); // defaults: Method=1, MaxSteps=2000
    jobmanager->RunJobs();
    delete jobmanager;

    const int mcCount = model->getMCStatisticResult();
    if (mcCount < 1) {
        std::fprintf(stderr, "Monte-Carlo run produced no statistic block\n");
        return 1;
    }
    const QJsonObject statistic = model->getStatistic(SupraFit::Method::MonteCarlo, mcCount - 1);

    ChartWrapper* wrapper = new ChartWrapper;
    wrapper->setData(model);

    ResultsWidget* widget = new ResultsWidget(statistic, model, wrapper);
    PhaseTiming::End();
    delete widget;

    // Rebuilds on the identical blob: deterministic repetition of just the widget phase.
    for (int i = 1; i <= rebuilds; ++i) {
        PhaseTiming::Begin(QStringLiteral("results widget rebuild %1/%2 (same MC blob)").arg(i).arg(rebuilds));
        ResultsWidget* rebuilt = new ResultsWidget(statistic, model, wrapper);
        PhaseTiming::End();
        delete rebuilt;
    }

    delete wrapper;
    return 0;
}
