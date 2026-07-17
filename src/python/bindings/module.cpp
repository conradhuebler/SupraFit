/*
 * <SupraFit in-process Python bindings — the native (pybind11) backend, Phase 2.>
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
 * AI acknowledgement: the pybind11 module scaffolding is Claude Generated.
 */

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>

#include <string>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QThreadPool>
#include <QtCore/QVector>

#include "src/core/analysis_manager.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/projectmanager.h"
#include "src/version.h"

namespace py = pybind11;

/*!
 * \brief Ensure a process-wide QCoreApplication exists.
 *
 * The fitting and statistics code relies on Qt object machinery and a QThreadPool, both of which
 * need a running QCoreApplication instance. Python has no Qt event loop of its own, so we create a
 * single instance lazily on module import and keep it alive for the interpreter's lifetime (it is
 * intentionally never destroyed — teardown of Qt's global state at exit is the OS's job here).
 * The basic fit path is synchronous, so no exec() / event loop is required.
 * Claude Generated.
 */
static void ensureQCoreApplication()
{
    if (QCoreApplication::instance() != nullptr)
        return;
    static int argc = 1;
    static char arg0[] = "suprafit";
    static char* argv[] = { arg0, nullptr };
    // Leaked on purpose: lives for the whole process, torn down by the OS at exit.
    new QCoreApplication(argc, argv);
}

/*!
 * \brief Fit models to independent/dependent tables in-process and return the project JSON.
 *
 * This is the native counterpart of the CLI fit path: it builds a DataClass straight from the two
 * matrices (no temp-file round-trip), runs SupraFit::AnalysisManager::fitModelsToData() — the same
 * routine the CLI's ML pipeline uses — and assembles the exact project shape the CLI writes to
 * `<base>-project-0.suprafit` (a `data` block plus `model_0..N`, each the model's
 * ExportModel(true, true)). The result is returned as a compact JSON string so the Python side can
 * json.loads() it and feed it to the same _results.parse_project() the CLI backend uses — the two
 * backends are therefore interchangeable.
 *
 * \param independent  data points (rows) x independent variables (cols)
 * \param dependent    data points (rows) x series (cols)
 * \param modelsJson   the CLI `AddModels` object, e.g. {"nmr_1_1": {"ID": 1, "Options": {...}}}
 * \param analysisJson the post-fit config, e.g. {"methods": [{"Method": 1, ...}]} (may be empty)
 * \param nproc        worker threads for post-fit statistics; <=0 uses the pool's max thread count
 * Claude Generated.
 */
static std::string fitFromTables(const Eigen::MatrixXd& independent,
    const Eigen::MatrixXd& dependent,
    const std::string& modelsJson,
    const std::string& analysisJson,
    int nproc)
{
    ensureQCoreApplication();

    // The statistics engine (JobManager / MonteCarloStatistics / ResampleAnalyse) reads the worker
    // count from the app-wide "threads" property and DIVIDES by it (blocksize = MaxSteps/threads/20).
    // The CLI/GUI set this; a bare module does not, so an unset property reads as 0 and post-fit
    // analysis crashes with SIGFPE. Set it here exactly as suprafit_cli does. Claude Generated.
    const int threads = nproc > 0 ? nproc : QThreadPool::globalInstance()->maxThreadCount();
    QCoreApplication::instance()->setProperty("threads", threads);

    // Each table is duplicated into its raw slot; the DataClass owns all four and frees them in its
    // destructor (see the DataClassPrivate table-ownership fixes).
    DataClass* data = new DataClass();
    data->setIndependentTable(new DataTable(independent));
    data->setIndependentRawTable(new DataTable(independent));
    data->setDependentTable(new DataTable(dependent));
    data->setDependentRawTable(new DataTable(dependent));
    data->setType(DataClassPrivate::DataType::Table);

    const QJsonObject modelsConfig
        = QJsonDocument::fromJson(QByteArray::fromStdString(modelsJson)).object();
    const QJsonObject analysisConfig
        = QJsonDocument::fromJson(QByteArray::fromStdString(analysisJson)).object();

    AnalysisManager manager;
    const QVector<QJsonObject> fitted = manager.fitModelsToData(data, modelsConfig, analysisConfig);

    QJsonObject project;
    project["data"] = data->ExportData();
    for (int i = 0; i < fitted.size(); ++i) {
        const QJsonObject& r = fitted.at(i);
        if (r.contains("model_export") && r.value("model_export").isObject())
            project[QStringLiteral("model_%1").arg(i)] = r.value("model_export").toObject();
    }

    const std::string out = QJsonDocument(project).toJson(QJsonDocument::Compact).toStdString();
    delete data;
    return out;
}

PYBIND11_MODULE(_core, m)
{
    m.doc() = "SupraFit in-process core (pybind11). Phase 2 native backend — Claude Generated.";

    ensureQCoreApplication();

    m.def("qt_ready", []() { return QCoreApplication::instance() != nullptr; },
        "True once the process-wide QCoreApplication exists (created on import).");

    // Touch a real libcore symbol so the SHARED core library is actually linked and loaded via the
    // module's RPATH — this is what validates the Phase-2 build/link wiring.
    m.def("project_manager_ready", []() {
        return &SupraFit::ProjectManager::instance() != nullptr;
    }, "Instantiate the core ProjectManager singleton; returns True on success.");

    m.def("git_commit", []() { return git_commit_hash.toStdString(); },
        "The git commit hash this module was built from.");

    m.def("fit_from_tables", &fitFromTables,
        py::arg("independent"), py::arg("dependent"),
        py::arg("models_json"), py::arg("analysis_json") = std::string("{}"),
        py::arg("nproc") = 0,
        "Fit models to independent/dependent tables in-process (optionally with post-fit analysis); "
        "returns the project JSON string (same shape as the CLI backend).");
}
