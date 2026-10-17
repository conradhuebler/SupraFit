// tools_inspect.cpp - see tools_inspect.h
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026
#include "tools_inspect.h"

#include <cutechat/toolregistry.h>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/projectmanager.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QStringList>

#include <algorithm>
#include <cmath>

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

QJsonObject prop(const QString& type, const QString& description)
{
    return QJsonObject{ { QStringLiteral("type"), type }, { QStringLiteral("description"), description } };
}

QJsonObject intProp(const QString& description, int minimum = 0)
{
    QJsonObject o = prop(QStringLiteral("integer"), description);
    o.insert(QStringLiteral("minimum"), minimum);
    return o;
}

/// The model at @p index of @p projectId, or an error text.
QSharedPointer<AbstractModel> findModel(ProjectManager* pm, const QJsonObject& args, QString* error, int* indexOut = nullptr)
{
    if (!pm) {
        *error = QStringLiteral("no ProjectManager available");
        return {};
    }
    const QString projectId = args.value(QStringLiteral("project_id")).toString();
    const int index = args.value(QStringLiteral("model_index")).toInt(-1);
    const QVector<QSharedPointer<AbstractModel>> models = pm->getProjectModels(projectId);
    if (index < 0 || index >= models.size()) {
        *error = QStringLiteral("model_index %1 out of range: project '%2' has %3 model(s)")
                     .arg(index).arg(projectId).arg(models.size());
        return {};
    }
    if (indexOut)
        *indexOut = index;
    return models.at(index);
}

QString seriesName(const DataTable* table, int series)
{
    if (table && series < table->header().size() && !table->header().at(series).isEmpty())
        return table->header().at(series);
    return QStringLiteral("series %1").arg(series + 1);
}

double rounded(double v)
{
    return std::isfinite(v) ? v : 0.0;
}

}  // namespace

/// Per-parameter summary of one stored statistic run (value, confidence interval, median, stddev).
/// @p samples receives the largest sample count found. Claude Generated.
QJsonArray summariseStatisticParameters(const QJsonObject& body, int* samples)
{
    QJsonArray parameters;
    QStringList keys = body.keys();
    std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) { return a.toInt() < b.toInt(); });
    for (const QString& key : keys) {
        if (key == QLatin1String("controller"))
            continue;
        const QJsonObject e = body.value(key).toObject();
        if (!e.contains(QStringLiteral("name")))
            continue;
        QJsonObject p;
        p.insert(QStringLiteral("name"), e.value(QStringLiteral("name")));
        p.insert(QStringLiteral("type"), e.value(QStringLiteral("type")));
        p.insert(QStringLiteral("value"), e.value(QStringLiteral("value")));
        const QJsonObject ci = e.value(QStringLiteral("confidence")).toObject();
        if (!ci.isEmpty()) {
            // some methods store the bounds the wrong way round; report them ordered
            const double lo = ci.value(QStringLiteral("lower")).toDouble(), hi = ci.value(QStringLiteral("upper")).toDouble();
            p.insert(QStringLiteral("ci_lower"), std::min(lo, hi));
            p.insert(QStringLiteral("ci_upper"), std::max(lo, hi));
            p.insert(QStringLiteral("ci_level"), ci.value(QStringLiteral("error")));
        }
        const QJsonObject box = e.value(QStringLiteral("boxplot")).toObject();
        if (!box.isEmpty()) {
            p.insert(QStringLiteral("median"), box.value(QStringLiteral("median")));
            p.insert(QStringLiteral("stddev"), box.value(QStringLiteral("stddev")));
            if (samples)
                *samples = qMax(*samples, box.value(QStringLiteral("count")).toInt());
        }
        parameters.append(p);
        if (parameters.size() >= 40)
            break;  // keep the answer small; huge parameter lists are not useful in a chat
    }
    return parameters;
}

namespace {

}  // namespace

QJsonArray summariseGlobalSearch(const QJsonObject& run, int globalCount)
{
    struct Entry {
        double sse = 0;
        QJsonObject json;
    };
    QVector<Entry> entries;
    for (auto it = run.begin(); it != run.end(); ++it) {
        if (it.key() == QLatin1String("controller"))
            continue;
        const QJsonObject e = it.value().toObject();
        const QJsonObject model = e.value(QStringLiteral("model")).toObject();
        if (model.isEmpty())
            continue;
        const QStringList initial = e.value(QStringLiteral("initial")).toString().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        QJsonArray start, finalValues;
        for (int i = 0; i < globalCount && i < initial.size(); ++i)
            start.append(initial.at(i).toDouble());
        const QJsonObject table = model.value(QStringLiteral("data")).toObject().value(QStringLiteral("globalParameter")).toObject()
                                      .value(QStringLiteral("data")).toObject();
        // The global parameters are ONE table row, values separated by blanks ("3.38 2.44").
        const QStringList row = table.value(QStringLiteral("0")).toString().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        for (int i = 0; i < globalCount; ++i)
            finalValues.append(row.value(i).toDouble());
        Entry entry;
        entry.sse = model.value(QStringLiteral("SSE")).toDouble();
        entry.json.insert(QStringLiteral("start"), start);
        entry.json.insert(QStringLiteral("final_global_parameters"), finalValues);
        entry.json.insert(QStringLiteral("sse"), entry.sse);
        entry.json.insert(QStringLiteral("converged"), model.value(QStringLiteral("converged")).toBool());
        entries.append(entry);
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.sse < b.sse; });
    QJsonArray out;
    for (int i = 0; i < entries.size() && i < 20; ++i)
        out.append(entries.at(i).json);
    return out;
}

int registerInspectTools(ToolRegistry& registry, QPointer<ProjectManager> pm)
{
    int registered = 0;

    // --- get_model_parameters ----------------------------------------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("get_model_parameters");
        spec.description = QStringLiteral(
            "The fitted parameter VALUES of one model: the global parameters (e.g. the binding constants lg K, "
            "with their names) and the local parameters per data series (e.g. the chemical shifts of the pure "
            "species, with the series name). Use it to report numbers; get_model_fit only has the quality metrics.");
        spec.category = QStringLiteral("project");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), prop(QStringLiteral("string"), QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_index"), intProp(QStringLiteral("Index of the model within the project (from list_models)")));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("model_index") });
        spec.effect = ToolEffect::Read;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            QString error;
            int index = 0;
            const QSharedPointer<AbstractModel> model = findModel(pm, args, &error, &index);
            if (!model)
                return ToolResult::failure(error);

            QJsonArray globals;
            for (int i = 0; i < model->GlobalParameterSize(); ++i) {
                QJsonObject p;
                p.insert(QStringLiteral("index"), i);
                p.insert(QStringLiteral("name"), model->GlobalParameterName(i));
                p.insert(QStringLiteral("value"), rounded(model->GlobalParameter(i)));
                p.insert(QStringLiteral("optimised"), bool(model->GlobalEnabled(i)));
                globals.append(p);
            }
            QJsonArray localNames;
            for (int l = 0; l < model->LocalParameterSize(); ++l)
                localNames.append(model->LocalParameterName(l));
            QJsonArray series;
            if (model->LocalParameterSize() > 0) {
                for (int s = 0; s < model->SeriesCount(); ++s) {
                    QJsonArray values;
                    for (int l = 0; l < model->LocalParameterSize(); ++l)
                        values.append(rounded(model->LocalParameter(l, s)));
                    QJsonObject entry;
                    entry.insert(QStringLiteral("series"), s);
                    entry.insert(QStringLiteral("name"), seriesName(model->DependentModel(), s));
                    entry.insert(QStringLiteral("included"), bool(model->ActiveSignals(s)));
                    entry.insert(QStringLiteral("values"), values);
                    series.append(entry);
                }
            }
            QJsonObject out;
            out.insert(QStringLiteral("model_index"), index);
            out.insert(QStringLiteral("model_name"), model->Name());
            out.insert(QStringLiteral("converged"), model->isConverged());
            out.insert(QStringLiteral("global_parameters"), globals);
            out.insert(QStringLiteral("local_parameter_names"), localNames);
            out.insert(QStringLiteral("local_parameters_per_series"), series);
            return ToolResult::success(out,
                QStringLiteral("%1 global, %2 local parameter(s) per series of '%3'")
                    .arg(globals.size()).arg(localNames.size()).arg(model->Name()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- get_model_statistics ------------------------------------------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("get_model_statistics");
        spec.description = QStringLiteral(
            "The STORED statistical post-processing results of one model (Monte Carlo, weakened grid search, "
            "model comparison, cross validation, reduction ...): per run the method, number of samples and for "
            "each parameter the value, confidence interval, median and standard deviation where the method "
            "provides them. Nothing is computed; use run_postfit_analysis to create new results. Only the "
            "newest runs per method are returned.");
        spec.category = QStringLiteral("project");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), prop(QStringLiteral("string"), QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_index"), intProp(QStringLiteral("Index of the model within the project (from list_models)")));
        props.insert(QStringLiteral("max_runs"), intProp(QStringLiteral("Newest runs per method to return (default 2)"), 1));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("model_index") });
        spec.effect = ToolEffect::Read;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            QString error;
            int index = 0;
            const QSharedPointer<AbstractModel> model = findModel(pm, args, &error, &index);
            if (!model)
                return ToolResult::failure(error);
            const int maxRuns = args.value(QStringLiteral("max_runs")).toInt(2);

            const QJsonObject exported = model->ExportModel(true);
            const QJsonObject methods = exported.value(QStringLiteral("data")).toObject().value(QStringLiteral("methods")).toObject();

            struct Run {
                QString key;
                qint64 timestamp = 0;
                int method = 0;
                QJsonObject body;
            };
            QHash<int, QVector<Run>> byMethod;
            for (auto it = methods.begin(); it != methods.end(); ++it) {
                const QJsonObject run = it.value().toObject();
                const QJsonObject controller = run.value(QStringLiteral("controller")).toObject();
                Run r;
                r.key = it.key();
                r.timestamp = static_cast<qint64>(controller.value(QStringLiteral("timestamp")).toDouble());
                r.method = controller.value(QStringLiteral("Method")).toInt();
                r.body = run;
                byMethod[r.method].append(r);
            }

            QJsonArray runsOut;
            for (auto m = byMethod.begin(); m != byMethod.end(); ++m) {
                QVector<Run> runs = m.value();
                std::sort(runs.begin(), runs.end(), [](const Run& a, const Run& b) { return a.timestamp > b.timestamp; });
                for (int k = 0; k < runs.size() && k < maxRuns; ++k) {
                    const Run& r = runs.at(k);
                    int samples = 0;
                    const QJsonArray parameters = summariseStatisticParameters(r.body, &samples);
                    QJsonObject entry;
                    entry.insert(QStringLiteral("method"), SupraFit::Method2Name(r.method));
                    entry.insert(QStringLiteral("method_id"), r.method);
                    entry.insert(QStringLiteral("run"), r.key);
                    entry.insert(QStringLiteral("timestamp_ms"), double(r.timestamp));
                    if (samples > 0)
                        entry.insert(QStringLiteral("samples"), samples);
                    entry.insert(QStringLiteral("parameters"), parameters);
                    runsOut.append(entry);
                }
            }
            QJsonObject out;
            out.insert(QStringLiteral("model_index"), index);
            out.insert(QStringLiteral("model_name"), model->Name());
            out.insert(QStringLiteral("runs"), runsOut);
            return ToolResult::success(out, QStringLiteral("%1 stored statistic run(s) for '%2'").arg(runsOut.size()).arg(model->Name()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- get_data_table --------------------------------------------------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("get_data_table");
        spec.description = QStringLiteral(
            "Look at a project's data: the independent table (e.g. total concentrations of host and guest) or "
            "the dependent table (the measured signals, one column per series). Returns the headers, per column "
            "the minimum and maximum, and a window of rows (start_row, max_rows). Points excluded from the fit "
            "are flagged.");
        spec.category = QStringLiteral("project");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), prop(QStringLiteral("string"), QStringLiteral("Project UUID from list_projects")));
        QJsonObject tableProp = prop(QStringLiteral("string"), QStringLiteral("independent or dependent"));
        tableProp.insert(QStringLiteral("enum"), QJsonArray{ QStringLiteral("independent"), QStringLiteral("dependent") });
        props.insert(QStringLiteral("table"), tableProp);
        props.insert(QStringLiteral("start_row"), intProp(QStringLiteral("First row to return (0-based, default 0)")));
        props.insert(QStringLiteral("max_rows"), intProp(QStringLiteral("Number of rows to return (default 25, at most 100)"), 1));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("table") });
        spec.effect = ToolEffect::Read;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            if (!pm)
                return ToolResult::failure(QStringLiteral("no ProjectManager available"));
            const QString projectId = args.value(QStringLiteral("project_id")).toString();
            const QSharedPointer<DataClass> data = pm->getProjectData(projectId);
            if (!data)
                return ToolResult::failure(QStringLiteral("unknown project_id '%1'").arg(projectId));
            const bool dependent = args.value(QStringLiteral("table")).toString() == QLatin1String("dependent");
            const DataTable* table = dependent ? data->DependentModel() : data->IndependentModel();
            if (!table)
                return ToolResult::failure(QStringLiteral("the project has no such table"));

            const int rows = table->rowCount(), cols = table->columnCount();
            const int start = qBound(0, args.value(QStringLiteral("start_row")).toInt(0), qMax(0, rows - 1));
            const int count = qBound(1, args.value(QStringLiteral("max_rows")).toInt(25), 100);

            QJsonArray header, ranges;
            for (int c = 0; c < cols; ++c) {
                header.append(seriesName(table, c));
                double lo = 0, hi = 0;
                bool first = true;
                for (int r = 0; r < rows; ++r) {
                    const double v = table->data(r, c);
                    if (!std::isfinite(v))
                        continue;
                    lo = first ? v : std::min(lo, v);
                    hi = first ? v : std::max(hi, v);
                    first = false;
                }
                ranges.append(QJsonObject{ { QStringLiteral("min"), lo }, { QStringLiteral("max"), hi } });
            }
            QJsonArray rowData;
            int excluded = 0;
            for (int r = 0; r < rows; ++r)
                for (int c = 0; c < cols; ++c)
                    if (!table->isChecked(r, c))
                        ++excluded;
            for (int r = start; r < rows && r < start + count; ++r) {
                QJsonArray values, flags;
                bool anyExcluded = false;
                for (int c = 0; c < cols; ++c) {
                    values.append(rounded(table->data(r, c)));
                    if (!table->isChecked(r, c)) {
                        flags.append(c);
                        anyExcluded = true;
                    }
                }
                QJsonObject row{ { QStringLiteral("row"), r }, { QStringLiteral("values"), values } };
                if (anyExcluded)
                    row.insert(QStringLiteral("excluded_columns"), flags);
                rowData.append(row);
            }
            QJsonObject out;
            out.insert(QStringLiteral("table"), dependent ? QStringLiteral("dependent") : QStringLiteral("independent"));
            out.insert(QStringLiteral("rows_total"), rows);
            out.insert(QStringLiteral("columns"), cols);
            out.insert(QStringLiteral("header"), header);
            out.insert(QStringLiteral("column_ranges"), ranges);
            out.insert(QStringLiteral("excluded_points"), excluded);
            out.insert(QStringLiteral("start_row"), start);
            out.insert(QStringLiteral("rows"), rowData);
            out.insert(QStringLiteral("truncated"), start + count < rows);
            return ToolResult::success(out, QStringLiteral("%1 rows x %2 columns").arg(rows).arg(cols));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- get_residuals ---------------------------------------------------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("get_residuals");
        spec.description = QStringLiteral(
            "Judge HOW the model fits, not only how well on average: per data series the root-mean-square "
            "residual, the mean residual (a systematic offset), the largest residual with its row, and the "
            "lag-1 autocorrelation of the residuals (values near +1 or -1 indicate systematic structure that "
            "the model misses). Residual = measured - modelled, over the points used in the fit.");
        spec.category = QStringLiteral("project");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), prop(QStringLiteral("string"), QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_index"), intProp(QStringLiteral("Index of the model within the project (from list_models)")));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("model_index") });
        spec.effect = ToolEffect::Read;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            QString error;
            int index = 0;
            const QSharedPointer<AbstractModel> model = findModel(pm, args, &error, &index);
            if (!model)
                return ToolResult::failure(error);
            const DataTable* measured = model->DependentModel();
            const DataTable* modelled = model->ModelTable();
            if (!measured || !modelled)
                return ToolResult::failure(QStringLiteral("the model has no calculated values"));

            const int rows = qMin(measured->rowCount(), modelled->rowCount());
            const int cols = qMin(measured->columnCount(), modelled->columnCount());
            QJsonArray seriesOut;
            double sumSquares = 0;
            int used = 0;
            for (int c = 0; c < cols; ++c) {
                QVector<double> res;
                QVector<int> rowOf;
                for (int r = 0; r < rows; ++r) {
                    if (!measured->isChecked(r, c))
                        continue;
                    res.append(measured->data(r, c) - modelled->data(r, c));
                    rowOf.append(r);
                }
                if (res.isEmpty())
                    continue;
                double mean = 0, ss = 0, worst = 0;
                int worstRow = rowOf.first();
                for (int i = 0; i < res.size(); ++i) {
                    mean += res[i];
                    ss += res[i] * res[i];
                    if (std::abs(res[i]) > std::abs(worst)) {
                        worst = res[i];
                        worstRow = rowOf[i];
                    }
                }
                mean /= res.size();
                double num = 0, den = 0;
                for (int i = 0; i < res.size(); ++i) {
                    const double d = res[i] - mean;
                    den += d * d;
                    if (i > 0)
                        num += d * (res[i - 1] - mean);
                }
                QJsonObject s;
                s.insert(QStringLiteral("series"), c);
                s.insert(QStringLiteral("name"), seriesName(measured, c));
                s.insert(QStringLiteral("included"), bool(model->ActiveSignals(c)));
                s.insert(QStringLiteral("points"), res.size());
                s.insert(QStringLiteral("rms"), std::sqrt(ss / res.size()));
                s.insert(QStringLiteral("mean"), mean);
                s.insert(QStringLiteral("max_abs"), worst);
                s.insert(QStringLiteral("max_abs_row"), worstRow);
                s.insert(QStringLiteral("lag1_autocorrelation"), den > 0 ? num / den : 0.0);
                seriesOut.append(s);
                sumSquares += ss;
                used += res.size();
            }
            QJsonObject out;
            out.insert(QStringLiteral("model_index"), index);
            out.insert(QStringLiteral("model_name"), model->Name());
            out.insert(QStringLiteral("points_used"), used);
            out.insert(QStringLiteral("rms_overall"), used > 0 ? std::sqrt(sumSquares / used) : 0.0);
            out.insert(QStringLiteral("series"), seriesOut);
            return ToolResult::success(out, QStringLiteral("residuals of %1 series for '%2'").arg(seriesOut.size()).arg(model->Name()));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- set_model_parameter ---------------------------------------------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("set_model_parameter");
        spec.description = QStringLiteral(
            "Set ONE parameter value of a model (a start value for the next fit, or a fixed value to compare "
            "against). scope 'global' addresses a global parameter by name or index (e.g. 'lg K11' or 0); scope "
            "'local' needs the local parameter (name or index) and the series index. The model is recalculated, "
            "the new SSE is returned. This changes the model; it does not fit.");
        spec.category = QStringLiteral("analysis");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), prop(QStringLiteral("string"), QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_index"), intProp(QStringLiteral("Index of the model within the project (from list_models)")));
        QJsonObject scopeProp = prop(QStringLiteral("string"), QStringLiteral("global or local"));
        scopeProp.insert(QStringLiteral("enum"), QJsonArray{ QStringLiteral("global"), QStringLiteral("local") });
        props.insert(QStringLiteral("scope"), scopeProp);
        props.insert(QStringLiteral("parameter"), prop(QStringLiteral("string"), QStringLiteral("Parameter name as returned by get_model_parameters, or its index as text")));
        props.insert(QStringLiteral("value"), prop(QStringLiteral("number"), QStringLiteral("The new value")));
        props.insert(QStringLiteral("series"), intProp(QStringLiteral("Series index (required for scope local)")));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("model_index"),
                                                 QStringLiteral("scope"), QStringLiteral("parameter"), QStringLiteral("value") });
        spec.effect = ToolEffect::Mutate;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            QString error;
            int index = 0;
            const QSharedPointer<AbstractModel> model = findModel(pm, args, &error, &index);
            if (!model)
                return ToolResult::failure(error);
            const bool local = args.value(QStringLiteral("scope")).toString() == QLatin1String("local");
            const QString name = args.value(QStringLiteral("parameter")).toString().trimmed();
            const double value = args.value(QStringLiteral("value")).toDouble();
            if (!std::isfinite(value))
                return ToolResult::failure(QStringLiteral("value must be a finite number"));

            const int count = local ? model->LocalParameterSize() : model->GlobalParameterSize();
            int parameter = -1;
            for (int i = 0; i < count && parameter < 0; ++i)
                if ((local ? model->LocalParameterName(i) : model->GlobalParameterName(i)) == name)
                    parameter = i;
            bool isNumber = false;
            const int asIndex = name.toInt(&isNumber);
            if (parameter < 0 && isNumber && asIndex >= 0 && asIndex < count)
                parameter = asIndex;
            if (parameter < 0) {
                QStringList known;
                for (int i = 0; i < count; ++i)
                    known << (local ? model->LocalParameterName(i) : model->GlobalParameterName(i));
                return ToolResult::failure(QStringLiteral("no %1 parameter '%2' (available: %3)")
                        .arg(local ? QStringLiteral("local") : QStringLiteral("global"), name, known.join(QStringLiteral(", "))));
            }
            if (local) {
                const int series = args.value(QStringLiteral("series")).toInt(-1);
                if (series < 0 || series >= model->SeriesCount())
                    return ToolResult::failure(QStringLiteral("series %1 out of range (0..%2)").arg(series).arg(model->SeriesCount() - 1));
                model->setLocalParameter(value, parameter, series);
            } else {
                model->setGlobalParameter(value, parameter);
            }
            model->Calculate();
            QJsonObject out;
            out.insert(QStringLiteral("model_index"), index);
            out.insert(QStringLiteral("sse"), rounded(model->SSE()));
            out.insert(QStringLiteral("value_now"), local ? model->LocalParameter(parameter, args.value(QStringLiteral("series")).toInt())
                                                           : model->GlobalParameter(parameter));
            return ToolResult::success(out, QStringLiteral("parameter set, SSE now %1").arg(model->SSE(), 0, 'g', 6));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- set_series_included -----------------------------------------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("set_series_included");
        spec.description = QStringLiteral(
            "Include or exclude ONE data series (a measured signal, i.e. a column of the dependent table) in the "
            "fit of ONE model; the other models of the project are not touched. Use it to test whether a single "
            "signal drives or disturbs a result: exclude it, run fit_model, compare get_model_parameters and the SSE, "
            "then include it again. The model is recalculated but not fitted.");
        spec.category = QStringLiteral("analysis");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), prop(QStringLiteral("string"), QStringLiteral("Project UUID from list_projects")));
        props.insert(QStringLiteral("model_index"), intProp(QStringLiteral("Index of the model within the project (from list_models)")));
        props.insert(QStringLiteral("series"), intProp(QStringLiteral("Series index (0-based, see get_model_parameters / get_data_table header)")));
        props.insert(QStringLiteral("included"), prop(QStringLiteral("boolean"), QStringLiteral("true = take part in the fit, false = leave out")));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("model_index"),
                                                 QStringLiteral("series"), QStringLiteral("included") });
        spec.effect = ToolEffect::Mutate;
        spec.affinity = ToolAffinity::Any;
        spec.available = [pm]() { return pm && pm->getProjectCount() > 0; };
        spec.handler = [pm](const QJsonObject& args) -> ToolResult {
            QString error;
            int index = 0;
            const QSharedPointer<AbstractModel> model = findModel(pm, args, &error, &index);
            if (!model)
                return ToolResult::failure(error);
            const int series = args.value(QStringLiteral("series")).toInt(-1);
            const bool included = args.value(QStringLiteral("included")).toBool(true);
            if (series < 0 || series >= model->SeriesCount())
                return ToolResult::failure(QStringLiteral("series %1 out of range (0..%2)").arg(series).arg(model->SeriesCount() - 1));
            QList<int> active = model->ActiveSignals();
            while (active.size() < model->SeriesCount())
                active.append(1);
            active[series] = included ? 1 : 0;
            int remaining = 0;
            for (int v : active)
                remaining += v ? 1 : 0;
            if (remaining == 0)
                return ToolResult::failure(QStringLiteral("at least one series has to stay in the fit"));
            model->setActiveSignals(active);
            model->Calculate();
            QJsonArray states;
            for (int s = 0; s < model->SeriesCount(); ++s)
                states.append(bool(model->ActiveSignals(s)));
            QJsonObject out;
            out.insert(QStringLiteral("model_index"), index);
            out.insert(QStringLiteral("series"), series);
            out.insert(QStringLiteral("series_name"), seriesName(model->DependentModel(), series));
            out.insert(QStringLiteral("included"), included);
            out.insert(QStringLiteral("series_in_fit"), remaining);
            out.insert(QStringLiteral("series_included"), states);
            out.insert(QStringLiteral("sse_now"), rounded(model->SSE()));
            out.insert(QStringLiteral("note"), QStringLiteral("Recalculated, not refitted: run fit_model to fit without this series."));
            return ToolResult::success(out, QStringLiteral("series %1 %2").arg(series).arg(included ? QStringLiteral("included") : QStringLiteral("excluded")));
        };
        if (registry.add(spec))
            ++registered;
    }

    // --- set_rows_included -------------------------------------------------------
    {
        ToolSpec spec;
        spec.name = QStringLiteral("set_rows_included");
        spec.description = QStringLiteral(
            "Include or exclude whole DATA POINTS (rows of the data set, e.g. titration steps) for the fit. This "
            "changes the PROJECT'S data selection and therefore applies to ALL models of the project. Use it to test "
            "outliers or the influence of single points: exclude rows, run fit_models, compare, then include them "
            "again. At least 4 rows have to stay. Models are recalculated but not fitted.");
        spec.category = QStringLiteral("analysis");
        QJsonObject props;
        props.insert(QStringLiteral("project_id"), prop(QStringLiteral("string"), QStringLiteral("Project UUID from list_projects")));
        QJsonObject rowsProp;
        rowsProp.insert(QStringLiteral("type"), QStringLiteral("array"));
        rowsProp.insert(QStringLiteral("items"), QJsonObject{ { QStringLiteral("type"), QStringLiteral("integer") } });
        rowsProp.insert(QStringLiteral("description"), QStringLiteral("Row indices (0-based, see get_data_table)"));
        props.insert(QStringLiteral("rows"), rowsProp);
        props.insert(QStringLiteral("included"), prop(QStringLiteral("boolean"), QStringLiteral("true = take part in the fit, false = leave out")));
        spec.paramSchema = objectSchema(props, { QStringLiteral("project_id"), QStringLiteral("rows"), QStringLiteral("included") });
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
            const QJsonArray rows = args.value(QStringLiteral("rows")).toArray();
            const bool included = args.value(QStringLiteral("included")).toBool(true);
            if (rows.isEmpty())
                return ToolResult::failure(QStringLiteral("rows is empty"));
            const int total = data->IndependentModel()->rowCount();
            QSet<int> wanted;
            for (const QJsonValue& v : rows) {
                const int r = v.toInt(-1);
                if (r < 0 || r >= total)
                    return ToolResult::failure(QStringLiteral("row %1 out of range (0..%2)").arg(r).arg(total - 1));
                wanted.insert(r);
            }
            int inUse = 0;
            for (int r = 0; r < total; ++r)
                if (data->IndependentModel()->isChecked(r) != 0)
                    ++inUse;
            int after = inUse;
            for (int r : wanted) {
                const bool now = data->IndependentModel()->isChecked(r) != 0;
                if (!included && now)
                    --after;
                else if (included && !now)
                    ++after;
            }
            if (after < 4)
                return ToolResult::failure(QStringLiteral("only %1 row(s) would stay in the fit; at least 4 are needed").arg(after));
            for (int r : wanted)
                data->ReReadCheckedState(r, included);
            const QVector<QSharedPointer<AbstractModel>> models = pm->getProjectModels(projectId);
            for (const QSharedPointer<AbstractModel>& m : models)
                if (m)
                    m->Calculate();
            QJsonArray excluded;
            for (int r = 0; r < total; ++r)
                if (data->IndependentModel()->isChecked(r) == 0)
                    excluded.append(r);
            QJsonObject out;
            out.insert(QStringLiteral("rows_total"), total);
            out.insert(QStringLiteral("rows_in_fit"), total - excluded.size());
            out.insert(QStringLiteral("excluded_rows"), excluded);
            out.insert(QStringLiteral("models_recalculated"), models.size());
            out.insert(QStringLiteral("note"), QStringLiteral("Applies to all models of the project; recalculated, not refitted: run fit_models."));
            return ToolResult::success(out, QStringLiteral("%1 row(s) %2").arg(wanted.size()).arg(included ? QStringLiteral("included") : QStringLiteral("excluded")));
        };
        if (registry.add(spec))
            ++registered;
    }

    return registered;
}
