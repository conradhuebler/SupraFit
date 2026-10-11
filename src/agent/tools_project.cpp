// tools_project.cpp - Read tools over ProjectManager: list_projects,
// get_project_info, list_models, get_model_fit, describe_model_catalog.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Phase 4 of roadmap/llm_agent_analysis.md: the first tools
// that expose REAL SupraFit functionality (ProjectManager) to the agent layer. All
// five Phase-4 tools are Read-effect and run headless against
// ProjectManager::instance() -- exactly like the CLI, so they are ctest-able without
// a GUI and without a network.
//
// The handlers take a borrowed QPointer<ProjectManager> so a test can hand in its own
// instance; production registration passes &ProjectManager::instance().

#include "tools_project.h"

#include "toolregistry.h"

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/projectmanager.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QPair>
#include <QVector>

using SupraFit::ProjectManager;
// QPointer<T> needs T complete for its destructor; the header only forward-
// declares SupraFit::ProjectManager, so pull the full definition in here.

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

/// One model as the agent sees it: id, name, model-id (into SupraFit::Model),
/// fit quality and convergence. Deliberately NOT the raw parameter tables --
/// they are huge in the context; get_model_parameters is a later Phase-5 tool.
QJsonObject modelSummary(const QSharedPointer<AbstractModel>& model, int index)
{
    QJsonObject m;
    m.insert(QStringLiteral("index"), index);
    m.insert(QStringLiteral("model_id"), model ? static_cast<int>(model->SFModel()) : -1);
    m.insert(QStringLiteral("name"), model ? model->Name() : QString());
    m.insert(QStringLiteral("model_uuid"), model ? model->ModelUUID() : QString());
    m.insert(QStringLiteral("sse"), model ? model->SSE() : 0.0);
    m.insert(QStringLiteral("aic"), model ? model->GetAIC() : 0.0);
    m.insert(QStringLiteral("aicc"), model ? model->GetAICc() : 0.0);
    m.insert(QStringLiteral("converged"), model ? model->isConverged() : false);
    m.insert(QStringLiteral("global_parameters"), model ? model->GlobalParameterSize() : 0);
    // AbstractModel IS a DataClass (inheritance, AbstractModel.h:85); the
    // const IndependentModel() overload returns DataTable* (dataclass.h:178).
    const AbstractModel* cm = model.data();
    m.insert(QStringLiteral("data_points"), cm && cm->IndependentModel()
        ? cm->IndependentModel()->rowCount() : 0);
    return m;
}

}  // namespace

int registerProjectTools(ToolRegistry& registry, QPointer<ProjectManager> pm)
{
    int registered = 0;

    // --- list_projects: every loaded project, one line each -------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("list_projects");
        spec.description = QStringLiteral(
            "List every project currently loaded in SupraFit: its id (UUID), title, "
            "and how many models it carries. Call this first to find the project id "
            "the other tools need.");
        spec.category = QStringLiteral("project");
        spec.paramSchema = objectSchema({});
        spec.effect = ToolEffect::Read;
        spec.affinity = ToolAffinity::Any;  // ProjectManager is thread-safe (singleton)
        spec.handler = [pm](const QJsonObject&) -> ToolResult {
            if (!pm)
                return ToolResult::failure(QStringLiteral("no ProjectManager available"));
            QJsonArray projects;
            const QStringList ids = pm->getLoadedProjectIds();
            for (const QString& id : ids) {
                QJsonObject p;
                p.insert(QStringLiteral("id"), id);
                p.insert(QStringLiteral("model_count"), pm->getModelCount(id));
                const QSharedPointer<DataClass> data = pm->getProjectData(id);
                p.insert(QStringLiteral("title"), data ? data->ProjectTitle() : QString());
                p.insert(QStringLiteral("has_data"), data && data->IndependentModel());
                projects.append(p);
            }
            return ToolResult::success(
                QJsonObject{ { QStringLiteral("projects"), projects },
                             { QStringLiteral("count"), projects.size() } },
                QStringLiteral("%1 project(s) loaded").arg(projects.size()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- get_project_info: one project in detail --------------------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("get_project_info");
        spec.description = QStringLiteral(
            "Details about one loaded project: title, data dimensions (independent "
            "and dependent rows/columns), and the list of its models with fit "
            "quality. Needs the project id from list_projects.");
        spec.category = QStringLiteral("project");
        spec.paramSchema = objectSchema(
            { { QStringLiteral("project_id"), stringProp(QStringLiteral("Project UUID from list_projects")) } },
            { QStringLiteral("project_id") });
        spec.effect = ToolEffect::Read;
        spec.affinity = ToolAffinity::Any;
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            if (!pm)
                return ToolResult::failure(QStringLiteral("no ProjectManager available"));
            const QString projectId = args.value(QStringLiteral("project_id")).toString();
            const QSharedPointer<DataClass> data = pm->getProjectData(projectId);
            if (!data)
                return ToolResult::failure(QStringLiteral("no project with id '%1' is loaded").arg(projectId));

            QJsonObject info;
            info.insert(QStringLiteral("id"), projectId);
            info.insert(QStringLiteral("title"), data->ProjectTitle());
            if (const DataTable* indep = data->IndependentModel()) {
                info.insert(QStringLiteral("independent_rows"), indep->rowCount());
                info.insert(QStringLiteral("independent_cols"), indep->columnCount());
            }
            if (const DataTable* dep = data->DependentModel()) {
                info.insert(QStringLiteral("dependent_rows"), dep->rowCount());
                info.insert(QStringLiteral("dependent_cols"), dep->columnCount());
            }
            const QVector<QSharedPointer<AbstractModel>> models = pm->getProjectModels(projectId);
            QJsonArray modelList;
            for (int i = 0; i < models.size(); ++i)
                modelList.append(modelSummary(models.at(i), i));
            info.insert(QStringLiteral("models"), modelList);
            info.insert(QStringLiteral("model_count"), models.size());

            return ToolResult::success(info,
                QStringLiteral("project '%1': %2 model(s), %3 independent x %4 dependent data")
                    .arg(data->ProjectTitle())
                    .arg(models.size())
                    .arg(info.value(QStringLiteral("independent_rows")).toInt())
                    .arg(info.value(QStringLiteral("dependent_cols")).toInt()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- list_models: just the model list, no data dimensions -------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("list_models");
        spec.description = QStringLiteral(
            "List the models of one loaded project: index, model id, name, SSE, AIC, "
            "AICc and convergence. Lighter than get_project_info when only the "
            "models matter.");
        spec.category = QStringLiteral("project");
        spec.paramSchema = objectSchema(
            { { QStringLiteral("project_id"), stringProp(QStringLiteral("Project UUID from list_projects")) } },
            { QStringLiteral("project_id") });
        spec.effect = ToolEffect::Read;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            if (!pm)
                return ToolResult::failure(QStringLiteral("no ProjectManager available"));
            const QString projectId = args.value(QStringLiteral("project_id")).toString();
            if (!pm->hasProject(projectId))
                return ToolResult::failure(QStringLiteral("no project with id '%1' is loaded").arg(projectId));
            const QVector<QSharedPointer<AbstractModel>> models = pm->getProjectModels(projectId);
            QJsonArray modelList;
            for (int i = 0; i < models.size(); ++i)
                modelList.append(modelSummary(models.at(i), i));
            return ToolResult::success(
                QJsonObject{ { QStringLiteral("project_id"), projectId },
                             { QStringLiteral("models"), modelList },
                             { QStringLiteral("count"), models.size() } });
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- get_model_fit: one model's fit result -----------------------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("get_model_fit");
        spec.description = QStringLiteral(
            "The fit result of one model in a project: SSE, AIC, AICc, R-squared, "
            "converged flag, parameter counts, data points. Identify the model by "
            "its index within the project (from list_models / get_project_info).");
        spec.category = QStringLiteral("project");
        QJsonObject modelIndexProp;
        modelIndexProp.insert(QStringLiteral("type"), QStringLiteral("integer"));
        modelIndexProp.insert(QStringLiteral("minimum"), 0);
        modelIndexProp.insert(QStringLiteral("description"),
            QStringLiteral("Index of the model within the project (from list_models)"));
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), stringProp(QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_index"), modelIndexProp);
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
                    QStringLiteral("model_index %1 out of range: project '%2' has %3 model(s)")
                        .arg(index).arg(projectId).arg(models.size()));
            const QSharedPointer<AbstractModel> model = models.at(index);
            if (!model)
                return ToolResult::failure(QStringLiteral("model %1 is null").arg(index));

            QJsonObject fit = modelSummary(model, index);
            // R^2 = 1 - SSE/SST; SST needs the dependent data -- guard against an
            // empty table so a fresh project does not divide by zero.
            const qreal sse = model->SSE();
            const QSharedPointer<DataClass> data = pm->getProjectData(projectId);
            qreal sst = 0.0;
            if (data && data->DependentModel() && data->DependentModel()->rowCount() > 0) {
                qreal sum = 0.0, sumSq = 0.0;
                const int rows = data->DependentModel()->rowCount();
                const int cols = data->DependentModel()->columnCount();
                for (int r = 0; r < rows; ++r)
                    for (int c = 0; c < cols; ++c) {
                        const qreal v = data->DependentModel()->data(r, c);
                        sum += v;
                        sumSq += v * v;
                    }
                const qreal n = qreal(rows) * qreal(cols);
                if (n > 0) {
                    const qreal mean = sum / n;
                    sst = sumSq - n * mean * mean;
                }
            }
            fit.insert(QStringLiteral("r_squared"), sst > 0.0 ? 1.0 - sse / sst : 0.0);
            fit.insert(QStringLiteral("sst"), sst);

            return ToolResult::success(fit,
                QStringLiteral("model '%1': SSE=%2, AIC=%3, converged=%4")
                    .arg(model->Name())
                    .arg(sse, 0, 'g', 6)
                    .arg(model->GetAIC(), 0, 'g', 6)
                    .arg(model->isConverged()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- describe_model_catalog: which model ids exist ---------------------------
    {
        // The catalogue is compiled from SupraFit::Model + a hand-maintained
        // name table. Keeping it in ONE place (here) means a new model id added
        // to global.h only needs a name here -- and the agent gets it for free.
        // The authoritative reference is src/core/models/MODEL_ID_REFERENCE.md.
        ToolSpec spec;
        spec.name = QStringLiteral("describe_model_catalog");
        spec.description = QStringLiteral(
            "List the model types SupraFit can fit, as id + name + technique. Use "
            "this to pick a model id for create_model / fit_models; do not guess ids.");
        spec.category = QStringLiteral("project");
        spec.paramSchema = objectSchema({});
        spec.effect = ToolEffect::Read;
        spec.affinity = ToolAffinity::Any;
        spec.handler = [](const QJsonObject&) -> ToolResult {
            // (id, name, technique) -- mirrors MODEL_ID_REFERENCE.md.
            static const QVector<QPair<int, QPair<QString, QString>>> catalog = {
                { 1,  { QStringLiteral("¹H 1:1-Model"),               QStringLiteral("NMR") } },
                { 2,  { QStringLiteral("¹H 2:1/1:1-Model"),            QStringLiteral("NMR") } },
                { 3,  { QStringLiteral("¹H 1:1/1:2-Model"),            QStringLiteral("NMR") } },
                { 4,  { QStringLiteral("¹H 2:1/1:1/1:2-Model"),        QStringLiteral("NMR") } },
                { 10, { QStringLiteral("ITC 1:1-Model"),               QStringLiteral("ITC") } },
                { 11, { QStringLiteral("ITC 2:1/1:1-Model"),            QStringLiteral("ITC") } },
                { 12, { QStringLiteral("ITC 1:1/1:2-Model"),            QStringLiteral("ITC") } },
                { 13, { QStringLiteral("ITC 2:1/1:1/1:2-Model"),        QStringLiteral("ITC") } },
                { 17, { QStringLiteral("flexible ITC Model"),          QStringLiteral("ITC") } },
                { 20, { QStringLiteral("Φ 1:1-Model"),                 QStringLiteral("Fluorescence") } },
                { 21, { QStringLiteral("Φ 2:1/1:1-Model"),              QStringLiteral("Fluorescence") } },
                { 22, { QStringLiteral("Φ 1:1/1:2-Model"),              QStringLiteral("Fluorescence") } },
                { 23, { QStringLiteral("Φ 2:1/1:1/1:2-Model"),          QStringLiteral("Fluorescence") } },
                { 30, { QStringLiteral("UV/VIS 1:1-Model"),            QStringLiteral("UV-Vis") } },
                { 31, { QStringLiteral("UV/VIS 2:1/1:1-Model"),         QStringLiteral("UV-Vis") } },
                { 32, { QStringLiteral("UV/VIS 1:1/1:2-Model"),         QStringLiteral("UV-Vis") } },
                { 33, { QStringLiteral("UV/VIS 2:1/1:1/1:2-Model"),    QStringLiteral("UV-Vis") } },
                { 34, { QStringLiteral("¹H flexible NMR-Model"),        QStringLiteral("NMR") } },
                { 35, { QStringLiteral("flexible UV/VIS Model"),       QStringLiteral("UV-Vis") } },
                { 36, { QStringLiteral("flexible fluorescence Model"), QStringLiteral("Fluorescence") } },
                { 100, { QStringLiteral("ScriptModel"),                QStringLiteral("Scripting") } },
                { 200, { QStringLiteral("Meta Model"),                QStringLiteral("Meta") } },
            };
            QJsonArray entries;
            for (const auto& entry : catalog) {
                QJsonObject e;
                e.insert(QStringLiteral("model_id"), entry.first);
                e.insert(QStringLiteral("name"), entry.second.first);
                e.insert(QStringLiteral("technique"), entry.second.second);
                entries.append(e);
            }
            return ToolResult::success(
                QJsonObject{ { QStringLiteral("models"), entries },
                             { QStringLiteral("count"), entries.size() },
                             { QStringLiteral("reference"), QStringLiteral("src/core/models/MODEL_ID_REFERENCE.md") } });
        };
        if (registry.add(spec))
            ++registered;
    }

    return registered;
}
