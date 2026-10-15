// tools_analysis.cpp - Compute tools: fit_model, fit_models, compare_models_aic,
// run_postfit_analysis, calculate_bc50.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Phase 5 of roadmap/llm_agent_analysis.md: the tools that
// CHANGE SupraFit state or start calculations. Every one is Compute-effect (fit,
// post-processing) or Read-after-compute (AIC compare, BC50) -- the agent loop's
// approval policy (ToolAutonomy) decides whether a Compute tool asks the operator.
//
// All handlers run headless against ProjectManager::instance(); fits are
// rechenintensiv and MUST run off the GUI thread, so ToolAffinity::Any is correct
// here (the agent loop is never the GUI thread) and the dispatcher marshalls only
// if someone calls from the GUI thread. Model objects are QSharedPointer and the
// fit mutates the model in place, so the tool returns the fit result as JSON.

#include "tools_analysis.h"

#include "toolregistry.h"

#include "src/core/analyse.h"
#include "src/core/analysis_manager.h"
#include "src/core/bc50.h"
#include "src/core/bc50system.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"
#include "src/core/models/scriptingengine.h"
#include "src/core/models/scriptmodel.h"
#include "src/core/projectmanager.h"

#include <algorithm>
#include <cmath>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using SupraFit::ProjectManager;

namespace {

QJsonObject objectSchema(const QJsonObject& properties, const QJsonArray& required = {})
{
    QJsonObject schema;
    schema.insert(QStringLiteral("type"), QStringLiteral("object"));
    schema.insert(QStringLiteral("properties"), properties);
    if (!required.isEmpty())
        schema.insert(QStringLiteral("required"), required);
    return schema;
}

QJsonObject stringProp(const QString& description)
{
    return QJsonObject{ { QStringLiteral("type"), QStringLiteral("string") },
        { QStringLiteral("description"), description } };
}

QJsonObject intProp(const QString& description, int minimum = 0)
{
    return QJsonObject{ { QStringLiteral("type"), QStringLiteral("integer") },
        { QStringLiteral("minimum"), minimum },
        { QStringLiteral("description"), description } };
}

QJsonObject stringArrayProp(const QString& description)
{
    return QJsonObject{ { QStringLiteral("type"), QStringLiteral("array") },
        { QStringLiteral("description"), description } };
}

QJsonObject fitToJson(const QSharedPointer<AbstractModel>& model)
{
    QJsonObject fit;
    fit.insert(QStringLiteral("model_id"), model ? static_cast<int>(model->SFModel()) : -1);
    fit.insert(QStringLiteral("name"), model ? model->Name() : QString());
    fit.insert(QStringLiteral("sse"), model ? model->SSE() : 0.0);
    fit.insert(QStringLiteral("aic"), model ? model->GetAIC() : 0.0);
    fit.insert(QStringLiteral("aicc"), model ? model->GetAICc() : 0.0);
    fit.insert(QStringLiteral("converged"), model ? model->isConverged() : false);
    fit.insert(QStringLiteral("global_parameters"), model ? model->GlobalParameterSize() : 0);
    return fit;
}

/// Extract the global parameters of a fitted model as a JSON array of
/// {name, value} -- the agent needs the actual numbers (lg beta) after a fit,
/// not only the quality metrics.
QJsonArray globalParametersToJson(const QSharedPointer<AbstractModel>& model)
{
    QJsonArray params;
    if (!model || !model->GlobalTable())
        return params;
    for (int i = 0; i < model->GlobalParameterSize(); ++i) {
        QJsonObject p;
        p.insert(QStringLiteral("index"), i);
        p.insert(QStringLiteral("name"), model->GlobalTable()->header().value(i));
        p.insert(QStringLiteral("value"), (*model->GlobalTable())[i]);
        params.append(p);
    }
    return params;
}

}  // namespace

int registerAnalysisTools(ToolRegistry& registry, QPointer<ProjectManager> pm)
{
    int registered = 0;

    // --- add_model: create models on a project's data (Mutate) -------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("add_model");
        spec.description = QStringLiteral(
            "Create one or more models on a project's data. Model ids come from "
            "describe_model_catalog (e.g. NMR: 1, 2, 3, 4). The new models start unfitted: "
            "run fit_models afterwards. Models that need a hand-written definition "
            "(reaction equations, scripts) are refused. Returns per id the new model_index or the reason it failed.");
        spec.category = QStringLiteral("analysis");
        QJsonObject idsProp;
        idsProp.insert(QStringLiteral("type"), QStringLiteral("array"));
        idsProp.insert(QStringLiteral("items"), QJsonObject{ { QStringLiteral("type"), QStringLiteral("integer") } });
        idsProp.insert(QStringLiteral("description"), QStringLiteral("Model ids from describe_model_catalog"));
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), stringProp(QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_ids"), idsProp);
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("model_ids") });
        spec.effect = ToolEffect::Mutate;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            if (!pm)
                return ToolResult::failure(QStringLiteral("no ProjectManager available"));
            const QString projectId = args.value(QStringLiteral("project_id")).toString();
            const QJsonArray ids = args.value(QStringLiteral("model_ids")).toArray();
            if (ids.isEmpty())
                return ToolResult::failure(QStringLiteral("model_ids is empty"));
            const QSharedPointer<DataClass> data = pm->getProjectData(projectId);
            if (!data)
                return ToolResult::failure(QStringLiteral("unknown project_id '%1'").arg(projectId));

            QJsonArray added;
            int ok = 0;
            for (const QJsonValue& v : ids) {
                const int id = v.toInt(-1);
                QJsonObject entry;
                entry.insert(QStringLiteral("model_id"), id);
                QSharedPointer<AbstractModel> model = CreateModel(id, QPointer<DataClass>(data.data()));
                if (!model) {
                    entry.insert(QStringLiteral("error"),
                        QStringLiteral("unknown model id, or it does not fit this data set's number of input variables"));
                } else if (model->DemandInput()) {
                    entry.insert(QStringLiteral("error"),
                        QStringLiteral("model needs a hand-written definition and cannot be added by the agent"));
                } else if (!model->Complete()) {
                    entry.insert(QStringLiteral("error"), QStringLiteral("model could not be set up on this data"));
                } else {
                    data->addModel(model);
                    pm->addModelToProject(model, projectId);
                    entry.insert(QStringLiteral("model_index"), pm->getProjectModels(projectId).size() - 1);
                    entry.insert(QStringLiteral("name"), model->Name());
                    ++ok;
                }
                added.append(entry);
            }
            QJsonObject out;
            out.insert(QStringLiteral("project_id"), projectId);
            out.insert(QStringLiteral("models"), added);
            if (ok == 0)
                return ToolResult::failure(QStringLiteral("no model could be added: %1")
                        .arg(QString::fromUtf8(QJsonDocument(added).toJson(QJsonDocument::Compact))));
            return ToolResult::success(out, QStringLiteral("added %1 of %2 model(s)").arg(ok).arg(ids.size()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- define_script_model: let the model write its own model (Mutate) ----------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("define_script_model");
        spec.description = QStringLiteral(
            "Define a NEW model from an equation and add it to a project (unfitted; run fit_models afterwards). "
            "Equation language is ExprTk: statements end with ';', 'var k := pow(10, lgK);' declares a "
            "temporary, the LAST statement is the returned value (the modelled signal of one data point). "
            "Available: the independent columns X1..Xn (n = input columns of the project), every name in "
            "global_names (fitted once for the whole data set) and local_names (fitted separately per data series), "
            "and functions such as pow, exp, log, sqrt, abs, min, max, sin, cos. Example 1:1 NMR shift with "
            "host X1, guest X2: 'var K := pow(10, lgK); var AB := 0.5*((X1+X2+1/K) - sqrt((X1+X2+1/K)*(X1+X2+1/K) - 4*X1*X2)); "
            "((X1 - AB)*dHost + AB*dAB)/X1' with global_names 'lgK', local_names 'dHost|dAB', print_x 'X2/X1', "
            "global_guess '[2;6]'. Fit binding constants on a log10 scale. An equation that does not evaluate is refused.");
        spec.category = QStringLiteral("analysis");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), stringProp(QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("name"), stringProp(QStringLiteral("Display name of the model")));
        props.insert(QStringLiteral("equation"), stringProp(QStringLiteral("ExprTk equation, statements separated by ';' or newlines")));
        props.insert(QStringLiteral("global_names"), stringProp(QStringLiteral("Global parameter names, '|'-separated, e.g. 'lgK11|lgK12'")));
        props.insert(QStringLiteral("local_names"), stringProp(QStringLiteral("Optional per-series parameter names, '|'-separated, e.g. 'dHost|dAB'")));
        props.insert(QStringLiteral("global_guess"), stringProp(QStringLiteral("Optional start ranges, '|'-separated '[min;max]' per global parameter, e.g. '[2;6]'")));
        props.insert(QStringLiteral("local_guess"), stringProp(QStringLiteral(
            "Optional start ranges for the per-series parameters, '|'-separated '[min;max]'. Default: the range of the measured data")));
        props.insert(QStringLiteral("print_x"), stringProp(QStringLiteral("Optional x axis expression for charts, default X1, e.g. 'X2/X1'")));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("name"),
                                                 QStringLiteral("equation"), QStringLiteral("global_names") });
        spec.effect = ToolEffect::Mutate;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            if (!pm)
                return ToolResult::failure(QStringLiteral("no ProjectManager available"));
            const QString projectId = args.value(QStringLiteral("project_id")).toString();
            const QSharedPointer<DataClass> data = pm->getProjectData(projectId);
            if (!data)
                return ToolResult::failure(QStringLiteral("unknown project_id '%1'").arg(projectId));
            const QString equation = args.value(QStringLiteral("equation")).toString().trimmed();
            const QString globalNames = args.value(QStringLiteral("global_names")).toString().trimmed();
            const QString localNames = args.value(QStringLiteral("local_names")).toString().trimmed();
            if (equation.isEmpty() || globalNames.isEmpty())
                return ToolResult::failure(QStringLiteral("equation and global_names must not be empty"));
            const int nGlobal = globalNames.split(QLatin1Char('|')).size();
            const int nLocal = localNames.isEmpty() ? 0 : localNames.split(QLatin1Char('|')).size();

            auto withValue = [](QJsonObject descriptor, const QJsonValue& value) {
                descriptor.insert(QStringLiteral("value"), value);
                return descriptor;
            };
            QJsonObject lines;  // the multi-line editor format: zero-padded line number -> text
            const QStringList eqLines = equation.split(QLatin1Char('\n'));
            for (int i = 0; i < eqLines.size(); ++i)
                lines.insert(QStringLiteral("%1").arg(i, 3, 10, QLatin1Char('0')), eqLines.at(i));
            const QString printX = args.value(QStringLiteral("print_x")).toString().trimmed();
            const QString guess = args.value(QStringLiteral("global_guess")).toString().trimmed();

            // Compile the equation first: the parser's own message tells the model what to fix,
            // where a bad equation inside a ScriptModel only shows up as an all-zero fit.
            {
                QStringList variables;
                for (int k = 1; k <= data->IndependentVariableSize(); ++k)
                    variables << QStringLiteral("X%1").arg(k);
                variables << globalNames.split(QLatin1Char('|'));
                if (nLocal > 0)
                    variables << localNames.split(QLatin1Char('|'));
                std::unique_ptr<ScriptingEngine> engine = MakeScriptingEngine(ScriptBackend::ExprTk);
                if (!engine || !engine->prepare(equation, variables))
                    return ToolResult::failure(QStringLiteral("the equation does not compile: %1 (variables: %2)")
                            .arg(engine ? engine->lastError() : QStringLiteral("no engine"), variables.join(QStringLiteral(", "))));
            }

            QHash<QString, QJsonObject> def;
            auto put = [&def](const QJsonObject& descriptor) { def.insert(descriptor.value(QStringLiteral("name")).toString(), descriptor); };
            put(withValue(ModelName_Json, args.value(QStringLiteral("name")).toString()));
            put(withValue(InputSize_Json, data->IndependentVariableSize()));
            put(withValue(GlobalParameterSize_Json, nGlobal));
            put(withValue(GlobalParameterNames_Json, globalNames));
            put(withValue(LocalParameterSize_Json, nLocal));
            if (nLocal > 0)
                put(withValue(LocalParameterNames_Json, localNames));
            put(withValue(PrintX_Json, printX.isEmpty() ? QStringLiteral("X1") : printX));
            put(withValue(Engine_Json, QStringLiteral("ExprTk")));
            put(withValue(Equation_Json, lines));
            if (!guess.isEmpty())
                put(withValue(GlobalParameterGuess_Json, guess));
            if (nLocal > 0) {
                // A per-series parameter (shift, extinction coefficient, ...) lives on the scale of the
                // measured signal; starting at 0 leaves the optimiser nowhere to go. Default: the data range.
                QString localGuess = args.value(QStringLiteral("local_guess")).toString().trimmed();
                if (localGuess.isEmpty()) {
                    const DataTable* y = data->DependentModel();
                    double lo = 0, hi = 0;
                    bool first = true;
                    for (int r = 0; y && r < y->rowCount(); ++r)
                        for (int c = 0; c < y->columnCount(); ++c) {
                            const double v = y->data(r, c);
                            if (!std::isfinite(v))
                                continue;
                            lo = first ? v : std::min(lo, v);
                            hi = first ? v : std::max(hi, v);
                            first = false;
                        }
                    const QString one = QStringLiteral("[%1;%2]").arg(lo, 0, 'g', 8).arg(hi, 0, 'g', 8);
                    QStringList all;
                    for (int i = 0; i < nLocal; ++i)
                        all << one;
                    localGuess = all.join(QLatin1Char('|'));
                }
                put(withValue(LocalParameterGuess_Json, localGuess));
            }

            QSharedPointer<AbstractModel> model = CreateModel(SupraFit::ScriptModel, QPointer<DataClass>(data.data()));
            if (!model)
                return ToolResult::failure(QStringLiteral("could not create a script model on this data"));
            model->setModelDefinition(def);
            if (!model->DefineModel(QJsonObject()) || !model->Complete())
                return ToolResult::failure(QStringLiteral("the definition is incomplete or the equation does not parse"));

            // Dry run before anything is attached: the equation must evaluate to finite numbers.
            model->InitialGuess();
            model->Calculate();
            if (!std::isfinite(model->SSE()))
                return ToolResult::failure(QStringLiteral(
                    "the equation does not evaluate to finite numbers on this data (check names, X1..Xn and brackets)"));

            data->addModel(model);
            pm->addModelToProject(model, projectId);
            QJsonObject out;
            out.insert(QStringLiteral("project_id"), projectId);
            out.insert(QStringLiteral("model_index"), pm->getProjectModels(projectId).size() - 1);
            out.insert(QStringLiteral("name"), model->Name());
            out.insert(QStringLiteral("global_parameters"), nGlobal);
            out.insert(QStringLiteral("local_parameters_per_series"), nLocal);
            out.insert(QStringLiteral("sse_with_start_values"), model->SSE());
            return ToolResult::success(out, QStringLiteral("defined script model '%1'").arg(model->Name()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- fit_model: fit ONE existing model (Compute) ----------------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("fit_model");
        spec.description = QStringLiteral(
            "Fit (optimize the parameters of) one existing model in a project. "
            "This CHANGES the model's parameters and statistics -- it is a Compute "
            "tool and the approval policy applies. Identify the model by its index "
            "within the project (from list_models). Returns the fit result.");
        spec.category = QStringLiteral("analysis");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), stringProp(QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_index"), intProp(QStringLiteral("Index of the model within the project (from list_models)")));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("model_index") });
        spec.effect = ToolEffect::Compute;
        spec.affinity = ToolAffinity::Any;  // agent loop is never the GUI thread
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            if (!pm)
                return ToolResult::failure(QStringLiteral("no ProjectManager available"));
            const QString projectId = args.value(QStringLiteral("project_id")).toString();
            const int index = args.value(QStringLiteral("model_index")).toInt(-1);
            const QVector<QSharedPointer<AbstractModel>> models = pm->getProjectModels(projectId);
            if (index < 0 || index >= models.size())
                return ToolResult::failure(
                    QStringLiteral("model_index %1 out of range: project '%2' has %3 model(s)")
                        .arg(index).arg(projectId).arg(models.size()));
            const QSharedPointer<AbstractModel> model = models.at(index);
            if (!model)
                return ToolResult::failure(QStringLiteral("model %1 is null").arg(index));

            model->Calculate();  // the fit; mutates the model in place

            QJsonObject result = fitToJson(model);
            result.insert(QStringLiteral("model_index"), index);
            result.insert(QStringLiteral("parameters"), globalParametersToJson(model));
            return ToolResult::success(result,
                QStringLiteral("fitted '%1': SSE=%2, converged=%3")
                    .arg(model->Name()).arg(model->SSE(), 0, 'g', 6)
                    .arg(model->isConverged()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- fit_models: fit SEVERAL models of a project (Compute) ------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("fit_models");
        spec.description = QStringLiteral(
            "Fit several models of a project in one call (optionally all of them). "
            "This CHANGES their parameters -- Compute tool, approval applies. "
            "Pass model_indices to fit a subset; omit it to fit every model.");
        spec.category = QStringLiteral("analysis");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), stringProp(QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_indices"), stringArrayProp(
            QStringLiteral("Optional subset of model indices (from list_models) as strings; omit to fit all")));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id") });
        spec.effect = ToolEffect::Compute;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            if (!pm)
                return ToolResult::failure(QStringLiteral("no ProjectManager available"));
            const QString projectId = args.value(QStringLiteral("project_id")).toString();
            const QVector<QSharedPointer<AbstractModel>> models = pm->getProjectModels(projectId);
            if (models.isEmpty())
                return ToolResult::failure(QStringLiteral("project '%1' has no models").arg(projectId));

            // Optional subset via model_indices (strings, as the schema is JSON).
            QVector<int> subset;
            const QJsonArray indices = args.value(QStringLiteral("model_indices")).toArray();
            for (const QJsonValue& v : indices) {
                bool ok = false;
                const int idx = v.toString().toInt(&ok);
                if (!ok)
                    return ToolResult::failure(
                        QStringLiteral("model_indices entries must be integer strings, got '%1'").arg(v.toString()));
                if (idx < 0 || idx >= models.size())
                    return ToolResult::failure(
                        QStringLiteral("model_index %1 out of range (project has %2)").arg(idx).arg(models.size()));
                subset.append(idx);
            }
            if (subset.isEmpty())
                for (int i = 0; i < models.size(); ++i)
                    subset.append(i);

            QJsonArray results;
            for (const int idx : subset) {
                const QSharedPointer<AbstractModel> model = models.at(idx);
                if (!model)
                    continue;
                model->Calculate();
                QJsonObject r = fitToJson(model);
                r.insert(QStringLiteral("model_index"), idx);
                r.insert(QStringLiteral("parameters"), globalParametersToJson(model));
                results.append(r);
            }
            return ToolResult::success(
                QJsonObject{ { QStringLiteral("project_id"), projectId },
                             { QStringLiteral("fitted"), results },
                             { QStringLiteral("count"), results.size() } },
                QStringLiteral("fitted %1 model(s)").arg(results.size()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- compare_models_aic: rank models by AIC (Read) ---------------------------
    //
    // Read-effect: it only READS the models' existing fit results. A model that
    // was never fitted has SSE=0, which would win every AIC -- so the tool refuses
    // to rank a project with unfitted models and says to fit first.
    {
        ToolSpec spec;
        spec.name = QStringLiteral("compare_models_aic");
        spec.description = QStringLiteral(
            "Rank the fitted models of a project by AIC (lower is better), with "
            "evidence ratios. Pure read of existing fit results -- does not fit "
            "anything. Models must be fitted first (fit_model / fit_models).");
        spec.category = QStringLiteral("analysis");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), stringProp(QStringLiteral("Project UUID from list_projects")));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id") });
        spec.effect = ToolEffect::Read;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            if (!pm)
                return ToolResult::failure(QStringLiteral("no ProjectManager available"));
            const QString projectId = args.value(QStringLiteral("project_id")).toString();
            const QVector<QSharedPointer<AbstractModel>> models = pm->getProjectModels(projectId);
            if (models.isEmpty())
                return ToolResult::failure(QStringLiteral("project '%1' has no models").arg(projectId));

            // Guard: refuse models without a fit result (SSE<=0). Known limit:
            // a freshly created, never-calculated model may already carry a tiny
            // positive SSE, and m_converged is false for some stored fits, so
            // neither is a reliable "never fitted" signal; SSE<=0 only catches
            // degenerate/empty results.
            for (int i = 0; i < models.size(); ++i) {
                const QSharedPointer<AbstractModel>& m = models.at(i);
                if (m && m->SSE() <= 0.0)
                    return ToolResult::failure(
                        QStringLiteral("model %1 ('%2') has no fit result (SSE=0); fit it first with fit_model/fit_models")
                            .arg(i).arg(m->Name()));
            }

            QVector<QWeakPointer<AbstractModel>> weak;
            for (const auto& m : models)
                weak.append(m);
            const QJsonObject metrics = StatisticTool::CalculateAICMetrics(weak);
            QJsonObject out;
            out.insert(QStringLiteral("project_id"), projectId);
            out.insert(QStringLiteral("aic"), metrics);
            // Human/model-readable summary: best model first. CalculateAICMetrics
            // nests the payload under methods["3"]["0"] (method id 3 =
            // ModelComparison); the ranking is "ranking" there, ascending by AIC.
            QStringList ranking;
            const QJsonObject methodResult = metrics.value(QStringLiteral("methods")).toObject()
                                                 .value(QStringLiteral("3")).toObject();
            const QJsonObject payload = methodResult.value(QStringLiteral("0")).toObject();
            const QJsonArray ranked = payload.value(QStringLiteral("ranking")).toArray();
            for (const QJsonValue& v : ranked)
                ranking.append(v.toObject().value(QStringLiteral("model")).toString());
            return ToolResult::success(out,
                ranking.isEmpty() ? QStringLiteral("no ranking")
                                  : QStringLiteral("best: %1").arg(ranking.first()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- run_postfit_analysis: Monte Carlo / CV / Reduction on a fitted model (Compute) ---
    {
        // Method ids mirror SupraFit::Method (src/global.h): MonteCarlo=1,
        // WeakenedGridSearch=2, ModelComparison=3, CrossValidation=4, Reduction=5,
        // FastConfidence=6, GlobalSearch=7. The config block keys mirror the
        // *ConfigBlock templates in src/capabilities/jobmanager.h.
        ToolSpec spec;
        spec.name = QStringLiteral("run_postfit_analysis");
        spec.description = QStringLiteral(
            "Run statistical post-processing (Monte Carlo, cross-validation, "
            "reduction, ...) on a FITTED model. This STARTS a calculation -- "
            "Compute tool, approval applies. method: 1=MonteCarlo, 2=WeakenedGridSearch, "
            "3=ModelComparison, 4=CrossValidation, 5=Reduction, 6=FastConfidence, 7=GlobalSearch. "
            "max_steps caps the work; the model must be fitted first.");
        spec.category = QStringLiteral("analysis");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), stringProp(QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_index"), intProp(QStringLiteral("Index of the fitted model (from list_models)")));
        QJsonObject methodProp;
        methodProp.insert(QStringLiteral("type"), QStringLiteral("integer"));
        methodProp.insert(QStringLiteral("minimum"), 1);
        methodProp.insert(QStringLiteral("maximum"), 7);
        methodProp.insert(QStringLiteral("description"), QStringLiteral("SupraFit::Method id: 1=MonteCarlo 2=WeakenedGridSearch 3=ModelComparison 4=CrossValidation 5=Reduction 6=FastConfidence 7=GlobalSearch"));
        props.insert(QStringLiteral("method"), methodProp);
        props.insert(QStringLiteral("max_steps"), intProp(QStringLiteral("Optional cap on the number of steps (default: the method's own default)"), 1));
        props.insert(QStringLiteral("job_config"), QJsonObject{
            { QStringLiteral("type"), QStringLiteral("object") },
            { QStringLiteral("description"), QStringLiteral("Optional extra job config keys merged over the defaults (e.g. {\"VarianceSource\":2} for Monte Carlo)") } });
        spec.paramSchema = objectSchema(props,
            { QStringLiteral("project_id"), QStringLiteral("model_index"), QStringLiteral("method") });
        spec.effect = ToolEffect::Compute;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            if (!pm)
                return ToolResult::failure(QStringLiteral("no ProjectManager available"));
            const QString projectId = args.value(QStringLiteral("project_id")).toString();
            const int index = args.value(QStringLiteral("model_index")).toInt(-1);
            const int method = args.value(QStringLiteral("method")).toInt(-1);
            const QVector<QSharedPointer<AbstractModel>> models = pm->getProjectModels(projectId);
            if (index < 0 || index >= models.size())
                return ToolResult::failure(
                    QStringLiteral("model_index %1 out of range (project has %2 models)").arg(index).arg(models.size()));
            const QSharedPointer<AbstractModel> model = models.at(index);
            if (!model)
                return ToolResult::failure(QStringLiteral("model %1 is null").arg(index));
            if (model->SSE() <= 0.0)
                return ToolResult::failure(
                    QStringLiteral("model %1 has no fit result (SSE=0); fit it first").arg(index));

            // Build the job config: Method id + optional max_steps + caller extras.
            QJsonObject job;
            job.insert(QStringLiteral("Method"), method);
            if (args.contains(QStringLiteral("max_steps")))
                job.insert(QStringLiteral("MaxSteps"), args.value(QStringLiteral("max_steps")).toInt());
            const QJsonObject extra = args.value(QStringLiteral("job_config")).toObject();
            for (auto it = extra.begin(); it != extra.end(); ++it)
                job.insert(it.key(), it.value());

            AnalysisManager manager;
            const QJsonObject result = manager.executePostFitAnalysis(model, QJsonObject{ { QStringLiteral("Jobs"), QJsonObject{ { QStringLiteral("job"), job } } } });
            QJsonObject out;
            out.insert(QStringLiteral("project_id"), projectId);
            out.insert(QStringLiteral("model_index"), index);
            out.insert(QStringLiteral("method"), method);
            out.insert(QStringLiteral("result"), result);
            return ToolResult::success(out);
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- calculate_bc50: BC50 from a fitted model's parameters (Read) ------------
    //
    // Read-effect: it COMPUTES a derived quantity from existing parameters but
    // changes no state. For the fixed-stoichiometry models the BC50 is a closed
    // form in the global parameters; for reaction-driven (*_any) models the
    // stoichiometry drives the dispatch. Unsupported systems report that instead
    // of a wrong number.
    {
        ToolSpec spec;
        spec.name = QStringLiteral("calculate_bc50");
        spec.description = QStringLiteral(
            "Compute the BC50 (concentration at 50% binding) of a FITTED model from "
            "its global parameters. Pure read of the fit result -- no state change. "
            "The model must be fitted first. Returns the BC50 in the project's "
            "concentration unit, or 'unsupported' for systems without a BC50 definition.");
        spec.category = QStringLiteral("analysis");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), stringProp(QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_index"), intProp(QStringLiteral("Index of the fitted model (from list_models)")));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("model_index") });
        spec.effect = ToolEffect::Read;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            if (!pm)
                return ToolResult::failure(QStringLiteral("no ProjectManager available"));
            const QString projectId = args.value(QStringLiteral("project_id")).toString();
            const int index = args.value(QStringLiteral("model_index")).toInt(-1);
            const QVector<QSharedPointer<AbstractModel>> models = pm->getProjectModels(projectId);
            if (index < 0 || index >= models.size())
                return ToolResult::failure(
                    QStringLiteral("model_index %1 out of range (project has %2 models)").arg(index).arg(models.size()));
            const QSharedPointer<AbstractModel> model = models.at(index);
            if (!model)
                return ToolResult::failure(QStringLiteral("model %1 is null").arg(index));
            if (model->SSE() <= 0.0)
                return ToolResult::failure(
                    QStringLiteral("model %1 has no fit result (SSE=0); fit it first").arg(index));

            // 1:1 systems: BC50 = 1/beta11 exactly. Read lgBeta0 from the global
            // parameter table; the fixed 1:1 models store log10(beta11) as the
            // first global parameter.
            const BC50::ModelSystem system = model->BC50System();
            QJsonObject out;
            out.insert(QStringLiteral("model_index"), index);
            out.insert(QStringLiteral("model_id"), static_cast<int>(model->SFModel()));
            out.insert(QStringLiteral("name"), model->Name());
            if (system.isEmpty()) {
                // Fixed-stoichiometry models return an empty ModelSystem only when
                // they do not override BC50System; fall back to the closed 1:1 form
                // when the model id is a 1:1 host-guest pattern.
                out.insert(QStringLiteral("supported"), false);
                out.insert(QStringLiteral("reason"),
                    QStringLiteral("no BC50 definition for this model/system"));
                return ToolResult::success(out);
            }
            const qreal bc50 = BC50::FromSpeciation(system.stoich, system.lgBeta);
            out.insert(QStringLiteral("supported"), bc50 >= 0.0);
            if (bc50 >= 0.0)
                out.insert(QStringLiteral("bc50"), bc50);
            else
                out.insert(QStringLiteral("reason"),
                    QStringLiteral("stoichiometry has no BC50 definition (unsupported system)"));
            return ToolResult::success(out);
        };
        if (registry.add(spec))
            ++registered;
    }

    return registered;
}
