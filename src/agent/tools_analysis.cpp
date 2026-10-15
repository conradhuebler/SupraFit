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
#include "src/core/projectmanager.h"

#include <QJsonArray>
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
