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
#include "tools_inspect.h"

#include "src/core/analyse.h"
#include "src/capabilities/jobmanager.h"
#include "src/core/analysis_manager.h"
#include "src/core/bc50.h"
#include "src/core/bc50system.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"
#include "src/core/models/scriptingengine.h"
#include "src/core/models/scriptmodel.h"
#include "src/core/models/titrations/AbstractTitrationModel.h"
#include "src/core/reactionparser.h"
#include "src/core/minimizer.h"
#include "src/core/projectmanager.h"

#include <algorithm>
#include <cmath>

#include <QCoreApplication>
#include <QElapsedTimer>
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
    // OpenAI rejects an array schema without items, so state it.
    return QJsonObject{ { QStringLiteral("type"), QStringLiteral("array") },
        { QStringLiteral("items"), QJsonObject{ { QStringLiteral("type"), QStringLiteral("string") } } },
        { QStringLiteral("description"), description } };
}

/// Optimise @p model with the same Minimizer the GUI and the statistics use. Calculate() alone
/// only re-evaluates the model at its CURRENT parameters -- it does not fit. Claude Generated.
bool runFit(const QSharedPointer<AbstractModel>& model)
{
    // Same sequence as AnalysisManager/Minimizer::Minimize(): the optimiser works on a clone and the
    // optimum is imported back. Minimize() itself is not used because it reports through the model's
    // Info() object, which a headless model need not have. Claude Generated.
    model->setFast(false);
    NonLinearFitThread thread(false);
    thread.setModel(model);
    thread.start();
    while (thread.Running())
        QCoreApplication::processEvents();
    const bool converged = thread.Converged();
    const QJsonObject best = converged ? thread.ConvergedParameter() : thread.BestIntermediateParameter();
    model->ImportModel(best);
    model->CalculateStatistics(true);
    return converged;
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
                    model->InitialGuess();  // a start value to fit from
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
            const QString localGuessGiven = args.value(QStringLiteral("local_guess")).toString().trimmed();
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
            // Deterministic, sensible start (InitialGuess may draw random values from the ranges, which
            // leaves a multi-series scripted model in a poor basin): globals at the middle of their
            // range, per-series parameters spread from the series' first to its last measured value
            // (free host ... saturated complex for a titration).
            {
                const QStringList ranges = guess.split(QLatin1Char('|'));
                for (int g = 0; g < nGlobal && g < ranges.size(); ++g) {
                    QString r = ranges.at(g);
                    r.remove(QLatin1Char('[')).remove(QLatin1Char(']'));
                    const QStringList mm = r.split(QLatin1Char(';'));
                    if (mm.size() == 2)
                        (*model->GlobalTable())[g] = 0.5 * (mm.at(0).toDouble() + mm.at(1).toDouble());
                }
                const DataTable* y = data->DependentModel();
                if (nLocal > 0 && y && y->rowCount() > 1 && localGuessGiven.isEmpty()) {
                    for (int j = 0; j < y->columnCount() && j < model->SeriesCount(); ++j) {
                        const double first = y->data(0, j), last = y->data(y->rowCount() - 1, j);
                        for (int l = 0; l < nLocal; ++l) {
                            const double f = nLocal > 1 ? double(l) / (nLocal - 1) : 0.0;
                            model->LocalTable()->data(j, l) = first + f * (last - first);
                        }
                    }
                }
            }
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

    // --- add_reaction_model: mixed stoichiometry from reaction equations (Mutate) ---
    {
        ToolSpec spec;
        spec.name = QStringLiteral("add_reaction_model");
        spec.description = QStringLiteral(
            "Add a titration model whose species are defined by reaction equations -- the way to test any "
            "mixed stoichiometry (1:1, 1:2, 2:1, 2:2, self-aggregation, three components ...). One reaction per "
            "line, complex formed from the free components: 'H + G <=> HG', 'H + 2 G <=> HG2', '2 H + G <=> H2G', "
            "'2 G <=> G2'. Components are numbered in order of first appearance and map to the project's "
            "independent concentration columns (X1, X2, ...), so the number of components must equal the number "
            "of those columns. Each reaction gets its own global parameter lg(beta), in order of the species. "
            "model_id: 34 = NMR (nmr_any), 35 = UV/Vis (uvvis_any), 36 = fluorescence (fl_any), 17 = ITC (itc_any, two components). "
            "The model starts unfitted: run fit_models afterwards and compare candidate stoichiometries with compare_models_aic.");
        spec.category = QStringLiteral("analysis");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), stringProp(QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_id"), intProp(QStringLiteral("34 NMR, 35 UV/Vis, 36 fluorescence, 17 ITC"), 0));
        props.insert(QStringLiteral("reactions"), stringProp(QStringLiteral("Reaction equations, one per line (or separated by ';')")));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("model_id"),
                                                 QStringLiteral("reactions") });
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
            const int modelId = args.value(QStringLiteral("model_id")).toInt(-1);
            if (modelId != 34 && modelId != 35 && modelId != 36 && modelId != 17)
                return ToolResult::failure(QStringLiteral("model_id must be 34 (NMR), 35 (UV/Vis), 36 (fluorescence) or 17 (ITC)"));
            QString reactions = args.value(QStringLiteral("reactions")).toString();
            reactions.replace(QLatin1Char(';'), QLatin1Char('\n'));

            const ReactionSystem system = ReactionParser::Parse(reactions);
            if (!system.valid) {
                QStringList problems;
                for (const ReactionDiagnostic& d : system.diagnostics)
                    if (!d.ok)
                        problems << QStringLiteral("line %1: %2").arg(d.line).arg(d.message);
                return ToolResult::failure(QStringLiteral("the reactions do not parse (%1)")
                        .arg(problems.isEmpty() ? QStringLiteral("no complex formed") : problems.join(QStringLiteral("; "))));
            }
            if (modelId != 17 && system.components.size() != data->IndependentVariableSize())
                return ToolResult::failure(QStringLiteral(
                    "the reactions define %1 component(s) (%2) but the data set has %3 independent concentration column(s)")
                        .arg(system.components.size()).arg(system.components.join(QStringLiteral(", ")))
                        .arg(data->IndependentVariableSize()));

            QSharedPointer<AbstractModel> model = CreateModel(modelId, QPointer<DataClass>(data.data()));
            if (!model)
                return ToolResult::failure(QStringLiteral("model %1 cannot be created on this data set").arg(modelId));
            QJsonObject reactionField;
            reactionField.insert(QStringLiteral("name"), QStringLiteral("Reactions"));
            reactionField.insert(QStringLiteral("type"), 6);
            reactionField.insert(QStringLiteral("value"), reactions);
            QHash<QString, QJsonObject> elements;
            elements.insert(QStringLiteral("Reactions"), reactionField);
            model->setModelDefinition(elements);
            model->DefineModel(QJsonObject());
            auto* titration = qobject_cast<AbstractTitrationModel*>(model.data());
            if (!model->Complete() || (titration && titration->ReactionComponentMismatch() > 0))
                return ToolResult::failure(QStringLiteral("the model could not be defined from these reactions on this data set"));

            model->InitialGuess();
            model->Calculate();
            if (!std::isfinite(model->SSE()))
                return ToolResult::failure(QStringLiteral("the model does not evaluate to finite numbers on this data"));

            data->addModel(model);
            pm->addModelToProject(model, projectId);
            QJsonArray species;
            for (const ReactionSpecies& sp : system.species)
                species.append(sp.label);
            QJsonObject out;
            out.insert(QStringLiteral("project_id"), projectId);
            out.insert(QStringLiteral("model_index"), pm->getProjectModels(projectId).size() - 1);
            out.insert(QStringLiteral("name"), model->Name());
            out.insert(QStringLiteral("components"), QJsonArray::fromStringList(system.components));
            out.insert(QStringLiteral("species"), species);
            out.insert(QStringLiteral("global_parameters"), model->GlobalParameterSize());
            return ToolResult::success(out, QStringLiteral("added '%1' with %2 species").arg(model->Name()).arg(system.species.size()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- fit_model: fit ONE existing model (Compute) ----------------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("fit_model");
        spec.description = QStringLiteral(
            "Fit (optimize the parameters of) one existing model in a project, starting from its current parameters. "
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

            runFit(model);  // the optimisation proper; mutates the model in place

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
                runFit(model);
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
            "Run statistical post-processing on a FITTED model and return the parameter results: "
            "method 1 = Monte Carlo (confidence intervals; max_steps = number of resampled fits, default 1000), "
            "4 = cross-validation (spread of the parameters over left-out data), "
            "6 = simplified model comparison (fast F-test based interval). The run is stored in the model, so "
            "get_model_statistics lists it again. This STARTS a calculation (seconds to minutes) -- Compute tool, "
            "approval applies. Reduction, weakened grid search, full model comparison and global search need a "
            "configuration the agent cannot supply yet and are refused.");
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

            if (method != SupraFit::Method::MonteCarlo && method != SupraFit::Method::CrossValidation
                && method != SupraFit::Method::FastConfidence && !args.contains(QStringLiteral("job_config")))
                return ToolResult::failure(QStringLiteral(
                    "method %1 (%2) needs a job configuration the agent cannot build yet; use method 1 (Monte Carlo), "
                    "4 (cross-validation) or 6 (simplified model comparison)").arg(method).arg(SupraFit::Method2Name(method)));

            // Build the job config: Method id + optional max_steps + caller extras.
            QJsonObject job;
            job.insert(QStringLiteral("Method"), method);
            if (args.contains(QStringLiteral("max_steps")))
                job.insert(QStringLiteral("MaxSteps"), args.value(QStringLiteral("max_steps")).toInt());
            const QJsonObject extra = args.value(QStringLiteral("job_config")).toObject();
            for (auto it = extra.begin(); it != extra.end(); ++it)
                job.insert(it.key(), it.value());

            // Run the job the way the GUI does (JobManager; synchronous) and read the run it stored in the
            // model. AnalysisManager::executePostFitAnalysis only understands its own flag names
            // (monteCarlo/crossValidation/reductionAnalysis), so the earlier call here "succeeded"
            // without computing anything. Claude Generated.
            if (!job.contains(QStringLiteral("MaxSteps")) && method == SupraFit::Method::MonteCarlo)
                job.insert(QStringLiteral("MaxSteps"), 1000);
            if (method == SupraFit::Method::MonteCarlo && !job.contains(QStringLiteral("VarianceSource")))
                job.insert(QStringLiteral("VarianceSource"), 2);
            if (method == SupraFit::Method::CrossValidation && !job.contains(QStringLiteral("CVType")))
                job.insert(QStringLiteral("CVType"), 1);

            JobManager jobManager;
            jobManager.setModel(model);
            int storedIndex = -1;
            QObject::connect(&jobManager, &JobManager::ShowResult, &jobManager,
                [&storedIndex](SupraFit::Method, int i) { storedIndex = i; });
            const int before = model->StatisticCount(static_cast<SupraFit::Method>(method));
            QElapsedTimer timer;
            timer.start();
            jobManager.AddSingleJob(job);
            jobManager.RunJobs();
            const qint64 ms = timer.elapsed();

            QJsonObject out;
            out.insert(QStringLiteral("project_id"), projectId);
            out.insert(QStringLiteral("model_index"), index);
            out.insert(QStringLiteral("method"), SupraFit::Method2Name(method));
            out.insert(QStringLiteral("method_id"), method);
            out.insert(QStringLiteral("seconds"), ms / 1000.0);
            const int after = model->StatisticCount(static_cast<SupraFit::Method>(method));
            if (storedIndex < 0 || after <= before)
                return ToolResult::failure(QStringLiteral(
                    "the %1 run stored no result (the job may need more configuration or the model does not support this method)")
                        .arg(SupraFit::Method2Name(method)));
            const QJsonObject run = model->getStatistic(static_cast<SupraFit::Method>(method), storedIndex);
            int samples = 0;
            const QJsonArray parameters = summariseStatisticParameters(run, &samples);
            if (parameters.isEmpty())
                return ToolResult::failure(QStringLiteral(
                    "the %1 run produced no parameter results with the default configuration").arg(SupraFit::Method2Name(method)));
            out.insert(QStringLiteral("parameters"), parameters);
            if (samples > 0)
                out.insert(QStringLiteral("samples"), samples);
            out.insert(QStringLiteral("stored_run_index"), storedIndex);
            out.insert(QStringLiteral("note"), QStringLiteral(
                "The run is stored in the model; get_model_statistics lists it again."));
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
