// postfitjobs.cpp - see postfitjobs.h
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026
#include "postfitjobs.h"

#include "src/capabilities/jobmanager.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/toolset.h"

#include <QList>
#include <QStringList>
#include <QVector>

#include <cmath>

namespace {

/// Threshold SSE of the F-test: SSE * (F * P / (N - P) + 1), as the dialog shows it.
double maxStatistic(AbstractModel* model, double confidencePercent, double* fValue)
{
    const double f = model->finv(confidencePercent / 100.0);
    if (fValue)
        *fValue = f;
    const double n = model->Points(), p = model->Parameter();
    const double error = model->StatisticVector().value(0);  // ParameterIndex 0 = SSE
    return n > p ? error * (f * p / (n - p) + 1.0) : error;
}

/// Parameter selection lists exactly as the dialog collects them: global parameters that are
/// part of the optimisation (1 = tested), local parameters not tested by default.
void parameterLists(AbstractModel* model, QList<int>* global, QList<int>* local)
{
    for (int i = 0; i < model->GlobalParameterSize(); ++i)
        if (model->GlobalTable()->isChecked(0, i))
            global->append(model->GlobalEnabled(i) ? 1 : 0);
    for (int i = 0; i < model->LocalParameterSize(); ++i)
        for (int j = 0; j < model->SeriesCount(); ++j)
            if (model->LocalTable()->isChecked(j, i))
                local->append(0);
}

}  // namespace

QJsonObject buildPostfitJob(const QSharedPointer<AbstractModel>& modelPtr, int method,
    const PostfitJobOptions& options, QString* error)
{
    AbstractModel* model = modelPtr.data();
    if (!model) {
        *error = QStringLiteral("no model");
        return {};
    }

    switch (method) {
    case SupraFit::Method::WeakenedGridSearch: {
        QJsonObject job = GridSearchConfigBlock;
        double f = 0;
        job["Method"] = method;
        job["MaxSteps"] = options.steps > 0 ? options.steps : 2000;
        job["MaxParameter"] = maxStatistic(model, options.confidence, &f);
        job["f-value"] = f;
        job["confidence"] = options.confidence;
        job["ParameterIndex"] = 0;
        job["ErrorConvergency"] = 1e-9;
        job["OverShotCounter"] = 5;
        job["ErrorDecreaseCounter"] = 50;
        job["ErrorConvergencyCounter"] = 5;
        job["StepScalingFactor"] = -4;
        job["StoreRaw"] = false;
        QList<int> global, local;
        parameterLists(model, &global, &local);
        job["GlobalParameterList"] = ToolSet::IntList2String(global);
        job["LocalParameterList"] = ToolSet::IntList2String(local);
        return job;
    }
    case SupraFit::Method::ModelComparison: {
        QJsonObject job = ModelComparisonConfigBlock;
        double f = 0;
        job["Method"] = method;
        job["MaxSteps"] = options.steps > 0 ? options.steps : 2000;  // the dialog proposes 20000
        job["MaxParameter"] = maxStatistic(model, options.confidence, &f);
        job["f-value"] = f;
        job["confidence"] = options.confidence;
        job["ParameterIndex"] = 0;
        QList<int> global, local;
        parameterLists(model, &global, &local);
        QVector<double> globalBox, localBox;
        for (int i = 0; i < model->GlobalParameterSize(); ++i)
            globalBox << 1.5;
        for (int i = 0; i < model->LocalParameterSize(); ++i)
            for (int j = 0; j < model->SeriesCount(); ++j)
                localBox << 1.5;
        job["GlobalParameterList"] = ToolSet::IntList2String(global);
        job["LocalParameterList"] = ToolSet::IntList2String(local);
        job["GlobalParameterScalingList"] = ToolSet::DoubleVec2String(globalBox);
        job["LocalParameterScalingList"] = ToolSet::DoubleVec2String(localBox);
        return job;
    }
    case SupraFit::Method::Reduction: {
        QJsonObject job = ResampleConfigBlock;
        job["Method"] = method;
        return job;
    }
    case SupraFit::Method::GlobalSearch: {
        // One entry per optimisation parameter, as the global search dialog lists them: the global
        // parameters, then the local ones of every series. Entry = "min max step optimise".
        QJsonObject job;
        job["Method"] = method;
        int count = 0;
        double calculations = 1;
        auto add = [&](double min, double max, double step, bool optimise) {
            QVector<qreal> entry;
            entry << min << max << step << (optimise ? 1.0 : 0.0);
            job[QString::number(count++)] = ToolSet::DoubleVec2String(entry);
            if (max > min && step > 0)
                calculations *= std::ceil((max - min) / step);
        };
        for (int i = 0; i < model->GlobalParameterSize(); ++i) {
            double min = model->GlobalParameter(i) - 1.0, max = model->GlobalParameter(i) + 1.0, step = 0.5;
            if (i < options.searchRanges.size()) {
                const QStringList parts = options.searchRanges.at(i).toString().split(QLatin1Char(';'));
                bool ok1 = false, ok2 = false, ok3 = false;
                const double a = parts.value(0).toDouble(&ok1), b = parts.value(1).toDouble(&ok2), c = parts.value(2).toDouble(&ok3);
                if (parts.size() != 3 || !ok1 || !ok2 || !ok3 || c <= 0 || b < a) {
                    *error = QStringLiteral("search_ranges[%1] must read 'min;max;step' with step > 0 and max >= min").arg(i);
                    return {};
                }
                min = a;
                max = b;
                step = c;
            }
            if (!model->GlobalEnabled(i))
                min = max = model->GlobalParameter(i);
            add(min, max, step, model->GlobalEnabled(i));
        }
        const int seriesForLocals = model->SupportSeries() ? model->SeriesCount() : 1;
        for (int j = 0; j < seriesForLocals; ++j)
            for (int i = 0; i < model->LocalParameterSize(); ++i) {
                const double v = model->LocalParameter(i, j);
                add(v, v, 1, model->LocalEnabled(i));
            }
        job["ParameterSize"] = count;
        if (calculations > options.maxCalculations) {
            *error = QStringLiteral("the search grid has %1 calculations (limit %2): give narrower or coarser search_ranges")
                         .arg(calculations, 0, 'g', 6).arg(options.maxCalculations);
            return {};
        }
        return job;
    }
    default:
        *error = QStringLiteral("method %1 needs no special configuration").arg(method);
        return {};
    }
}
