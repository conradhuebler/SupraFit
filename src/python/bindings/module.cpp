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
#include <pybind11/stl.h>

#include <cmath>
#include <random>
#include <string>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QThreadPool>
#include <QtCore/QVector>

#include <map>

#include "src/capabilities/datagenerator.h"
#include "src/core/analyse.h"
#include "src/core/analysis_manager.h"
#include "src/core/jsonhandler.h"
#include "src/core/minimizer.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/models.h"
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
 * \brief Serialise an Eigen matrix as a JSON array of rows (row-major list of lists).
 * Used to hand the fitted model signal and residual tables back to Python as arrays. Claude Generated.
 */
static QJsonArray matrixToJson(const Eigen::MatrixXd& m)
{
    QJsonArray rows;
    for (int r = 0; r < m.rows(); ++r) {
        QJsonArray row;
        for (int c = 0; c < m.cols(); ++c)
            row.append(m(r, c));
        rows.append(row);
    }
    return rows;
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
    int nproc,
    const std::map<int, double>& systemParams)
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
    // ITC (and other) system parameters are set on the shared DataClass so every model
    // fitModelsToData creates on it inherits them; setSystemParameterValue emits
    // SystemParameterChanged, which each model's ctor wires to UpdateParameter(). Claude Generated.
    for (const auto& kv : systemParams)
        data->setSystemParameterValue(kv.first, kv.second);

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
        if (!r.contains("model_export") || !r.value("model_export").isObject())
            continue;
        QJsonObject entry = r.value("model_export").toObject();
        // Reconstruct the fitted model from its export to attach the standardized C++ ML feature
        // vector (StatisticTool::ExtractModelMLFeatures) — rmse/sigma/dof/complexity/reduced-chi²
        // etc., richer than the raw export. This is the in-process ML deliverable (roadmap goal #5)
        // and reuses the same Json2Model path the CLI uses. Claude Generated.
        QSharedPointer<AbstractModel> model = JsonHandler::Json2Model(entry, data);
        if (model) {
            model->CalculateStatistics(true);
            model->Calculate();
            QJsonObject feats = StatisticTool::ExtractModelMLFeatures(model);
            // ExtractModelMLFeatures reads the error stats off the model, but a model rebuilt from
            // JSON has empty error accumulators (they are only filled by the full fit), so its
            // SSE-derived features come out 0/null. Overwrite those from the reliable exported
            // statistics; the structural features (counts, dof, complexity) are already correct.
            // Claude Generated.
            const double sse = entry.value("SSE").toDouble();
            const int points = feats.value("datapoints").toInt();
            const int dof = feats.value("degrees_of_freedom").toInt();
            const double sigma = entry.value("standard_error").toDouble();
            const double rmse = points > 0 ? std::sqrt(sse / points) : 0.0;
            feats["sse"] = sse;
            feats["rmse"] = rmse;
            feats["reduced_chi_squared"] = dof > 0 ? sse / dof : 0.0;
            feats["sigma"] = sigma;
            feats["normalized_rmse"] = sigma > 0.0 ? rmse / sigma : 0.0;
            feats["aic"] = entry.value("AIC").toDouble();
            feats["aicc"] = entry.value("AICc").toDouble();
            entry["ml_features"] = feats;

            // The fitted model signal (calculated curve) is not in the export JSON — it is recomputed
            // on load. CalculateVariables() (run unconditionally by Calculate()) fills ModelTable(),
            // so it is reliable after reconstruction; expose it as a row-major array for NumPy. The
            // model's ErrorTable() is NOT filled on a rebuilt model, so derive the residuals directly
            // as dependent - signal (correct regardless of the model's internal accumulators).
            if (model->ModelTable()) {
                const Eigen::MatrixXd& signal = model->ModelTable()->Table();
                entry["model_signal"] = matrixToJson(signal);
                if (signal.rows() == dependent.rows() && signal.cols() == dependent.cols())
                    entry["model_error"] = matrixToJson(dependent - signal);
            }
        }
        project[QStringLiteral("model_%1").arg(i)] = entry;
    }

    const std::string out = QJsonDocument(project).toJson(QJsonDocument::Compact).toStdString();
    delete data;
    return out;
}

/*!
 * \brief A live, in-process model handle for interactive (notebook-grade) use.
 *
 * Wraps a DataClass built from two matrices plus a model created on it, and exposes the low-level
 * verbs the roadmap calls for: set global/local parameters, run an initial guess, fit, and read back
 * scalars / parameter tables / the fitted signal as NumPy — all without a project-JSON round-trip.
 * The DataClass is owned for the model's lifetime. Claude Generated.
 */
class LiveModel {
public:
    LiveModel(int modelId, const Eigen::MatrixXd& independent, const Eigen::MatrixXd& dependent)
    {
        ensureQCoreApplication();
        m_data = QSharedPointer<DataClass>(new DataClass());
        m_data->setIndependentTable(new DataTable(independent));
        m_data->setIndependentRawTable(new DataTable(independent));
        m_data->setDependentTable(new DataTable(dependent));
        m_data->setDependentRawTable(new DataTable(dependent));
        m_data->setType(DataClassPrivate::DataType::Table);
        m_model = CreateModel(modelId, m_data.data());
        if (!m_model)
            throw std::runtime_error("CreateModel failed for model id " + std::to_string(modelId));
    }

    void setGlobal(double value, int index) { m_model->setGlobalParameter(value, index); }
    void setLocal(double value, int series, int param) { m_model->setLocalParameter(value, param, series); }
    void initialGuess() { m_model->InitialGuess(); }
    void calculate() { m_model->CalculateStatistics(true); m_model->Calculate(); }

    // System parameters (e.g. ITC: CellVolume=1, CellConcentration=2, SyringeConcentration=3,
    // Temperature=4). CreateModel declares them with defaults; set the experiment's values here,
    // then load_system_parameters() to push them into the model's internal state before fitting.
    // NB: this calls UpdateParameter() (which reads the values into the model's m_cell_concentration
    // etc.), NOT LoadSystemParameter() — the latter reloads from stored JSON and would discard the
    // just-set values. Claude Generated.
    void setSystemParameter(int index, double value) { m_model->setSystemParameterValue(index, value); }
    void loadSystemParameters() { m_model->UpdateParameter(); }

    // Run the real Levenberg-Marquardt fit from the current parameters (call initial_guess() or set
    // parameters first). Minimize() imports the optimum back into the model; enabling statistics and
    // recalculating then populates SSE / the model tables for the getters below.
    void fit()
    {
        m_model->setFast(false);
        Minimizer minimizer(false);
        minimizer.setModel(m_model);
        minimizer.Minimize();
        m_model->CalculateStatistics(true);
        m_model->Calculate();
    }

    int modelId() const { return m_model->SFModel(); }
    std::string name() const { return m_model->Name().toStdString(); }
    double sse() const { return m_model->SSE(); }
    double aic() const { return m_model->GetAIC(); }
    double aicc() const { return m_model->GetAICc(); }
    bool converged() const { return m_model->isConverged(); }
    Eigen::MatrixXd globalParameters() const { return m_model->GlobalTable()->Table(); }
    Eigen::MatrixXd localParameters() const { return m_model->LocalTable()->Table(); }
    Eigen::MatrixXd modelSignal() const { return m_model->ModelTable()->Table(); }

    std::string exportJson() const
    {
        return QJsonDocument(m_model->ExportModel(true, true)).toJson(QJsonDocument::Compact).toStdString();
    }

private:
    QSharedPointer<DataClass> m_data;
    QSharedPointer<AbstractModel> m_model;
};

/*!
 * \brief Generate an independent data table from the CLI's equation generator.
 *
 * \param equations pipe-separated per-variable expressions, e.g. "0.001|(X-1)*1e-4"
 * \param datapoints number of rows to generate
 * \return the generated table (rows x variables) as a NumPy-convertible Eigen matrix
 * Claude Generated.
 */
static Eigen::MatrixXd generateIndependent(const std::string& equations, int datapoints)
{
    ensureQCoreApplication();
    const QString eq = QString::fromStdString(equations);
    QJsonObject cfg;
    cfg["equations"] = eq;
    cfg["datapoints"] = datapoints;
    cfg["independent"] = eq.split("|").size(); // DataGenerator needs one equation per variable
    DataGenerator gen;
    gen.setJson(cfg);
    if (!gen.Evaluate() || !gen.Table())
        throw std::runtime_error("data generator evaluation failed for equations: " + equations);
    return gen.Table()->Table();
}

/*!
 * \brief Generate a dependent data table from a model at known parameters (+ optional noise).
 *
 * Deterministic, ground-truth generation: the caller supplies the global and local parameters, so
 * the exact parameters that produced the data are known (ideal for supervised ML / fit validation).
 * Draws the random-parameter distribution in NumPy and pass it here; this only computes the model
 * signal and adds i.i.d. Gaussian noise. The series count is taken from the local-parameter rows.
 *
 * \param modelId      SupraFit model id (see enum SupraFit::Model)
 * \param independent  data points (rows) x independent variables (cols)
 * \param globalParams the global parameters (e.g. lg K values), one per global parameter
 * \param localParams  series (rows) x local-parameters-per-series (cols)
 * \param noiseStd     standard deviation of added Gaussian noise (0 = noise-free)
 * \param seed         RNG seed for the noise (reproducible)
 * Claude Generated.
 */
static Eigen::MatrixXd generateDependent(int modelId,
    const Eigen::MatrixXd& independent,
    const Eigen::VectorXd& globalParams,
    const Eigen::MatrixXd& localParams,
    double noiseStd,
    unsigned long seed)
{
    ensureQCoreApplication();
    const int series = static_cast<int>(localParams.rows());
    if (series <= 0)
        throw std::runtime_error("generate_dependent needs local_params with one row per series");

    QSharedPointer<DataClass> data(new DataClass());
    data->setIndependentTable(new DataTable(independent));
    data->setIndependentRawTable(new DataTable(independent));
    // A zero dependent table of the right width tells the model how many series to build.
    const Eigen::MatrixXd placeholder = Eigen::MatrixXd::Zero(independent.rows(), series);
    data->setDependentTable(new DataTable(placeholder));
    data->setDependentRawTable(new DataTable(placeholder));
    data->setType(DataClassPrivate::DataType::Table);

    QSharedPointer<AbstractModel> model = CreateModel(modelId, data.data());
    if (!model)
        throw std::runtime_error("CreateModel failed for model id " + std::to_string(modelId));

    for (int i = 0; i < globalParams.size(); ++i)
        model->setGlobalParameter(globalParams(i), i);
    for (int s = 0; s < localParams.rows(); ++s)
        for (int p = 0; p < localParams.cols(); ++p)
            model->setLocalParameter(localParams(s, p), p, s);
    model->Calculate();

    if (!model->ModelTable())
        throw std::runtime_error("generate_dependent: model produced no signal table");
    Eigen::MatrixXd signal = model->ModelTable()->Table();

    if (noiseStd > 0.0) {
        std::mt19937_64 rng(seed);
        std::normal_distribution<double> noise(0.0, noiseStd);
        for (int r = 0; r < signal.rows(); ++r)
            for (int c = 0; c < signal.cols(); ++c)
                signal(r, c) += noise(rng);
    }
    return signal;
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
        py::arg("nproc") = 0, py::arg("system_parameters") = std::map<int, double>(),
        "Fit models to independent/dependent tables in-process (optionally with post-fit analysis "
        "and ITC system parameters {index: value}); returns the project JSON (same shape as CLI).");

    m.def("generate_independent", &generateIndependent,
        py::arg("equations"), py::arg("datapoints"),
        "Generate an independent data table (rows x variables) from pipe-separated equations, "
        "e.g. generate_independent(\"0.001|(X-1)*1e-4\", 20).");

    m.def("generate_dependent", &generateDependent,
        py::arg("model_id"), py::arg("independent"), py::arg("global_params"),
        py::arg("local_params"), py::arg("noise_std") = 0.0, py::arg("seed") = 0,
        "Generate a dependent table from a model at known parameters (+ optional Gaussian noise); "
        "series count is the number of local_params rows. Deterministic ground-truth generation.");

    // Low-level interactive model handle (notebook-grade): build a model on two tables, set/guess
    // parameters, fit, and read scalars / parameter tables / the fitted signal as NumPy.
    py::class_<LiveModel>(m, "Model",
        "A live in-process model: set_global/set_local, initial_guess, fit, then read sse()/"
        "global_parameters()/model_signal()/export_json(). Claude Generated.")
        .def(py::init<int, const Eigen::MatrixXd&, const Eigen::MatrixXd&>(),
            py::arg("model_id"), py::arg("independent"), py::arg("dependent"))
        .def("set_global", &LiveModel::setGlobal, py::arg("value"), py::arg("index"))
        .def("set_local", &LiveModel::setLocal, py::arg("value"), py::arg("series"), py::arg("param"))
        .def("set_system_parameter", &LiveModel::setSystemParameter, py::arg("index"), py::arg("value"))
        .def("load_system_parameters", &LiveModel::loadSystemParameters)
        .def("initial_guess", &LiveModel::initialGuess)
        .def("calculate", &LiveModel::calculate)
        .def("fit", &LiveModel::fit)
        .def("model_id", &LiveModel::modelId)
        .def("name", &LiveModel::name)
        .def("sse", &LiveModel::sse)
        .def("aic", &LiveModel::aic)
        .def("aicc", &LiveModel::aicc)
        .def("converged", &LiveModel::converged)
        .def("global_parameters", &LiveModel::globalParameters)
        .def("local_parameters", &LiveModel::localParameters)
        .def("model_signal", &LiveModel::modelSignal)
        .def("export_json", &LiveModel::exportJson);
}
