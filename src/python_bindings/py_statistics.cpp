/*
 * Python Bindings for Statistical Analysis
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Created by Claude Code AI Assistant
 *
 * This file provides Python bindings for SupraFit's statistical analysis tools.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>

#include "src/core/models/AbstractModel.h"
#include "src/core/analyse.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/modelcomparison.h"
#include "src/capabilities/resampleanalyse.h"

namespace py = pybind11;

// Helper functions
std::string qstring_to_string(const QString& qstr);
QString string_to_qstring(const std::string& str);
std::string qjson_to_string(const QJsonObject& obj);
QJsonObject string_to_qjson(const std::string& str);

void bind_statistics(py::module_& m) {

    // Monte Carlo Statistics
    m.def("monte_carlo", [](AbstractModel* model, int iterations, double confidence) {
        MonteCarloStatistics mc;
        mc.setModel(model);
        mc.setMaxIterations(iterations);
        mc.setConfidence(confidence);

        bool success = mc.Evaluate();

        py::dict result;
        result["success"] = success;
        result["iterations"] = mc.Iterations();
        result["converged"] = mc.Converged();

        // Extract parameter statistics
        QJsonObject mc_result = mc.ExportStatistics();
        result["statistics"] = qjson_to_string(mc_result);

        return result;
    }, "Perform Monte Carlo analysis",
       py::arg("model"),
       py::arg("iterations") = 10000,
       py::arg("confidence") = 0.95);

    // Cross Validation
    m.def("cross_validation", [](AbstractModel* model, int cv_type, int folds) {
        ModelComparison comparison;
        comparison.setModel(model);
        comparison.setCrossValidationType(cv_type);
        comparison.setFolds(folds);

        bool success = comparison.PerformCrossValidation();

        py::dict result;
        result["success"] = success;
        result["cv_score"] = comparison.CVScore();
        result["cv_error"] = comparison.CVError();

        QJsonObject cv_result = comparison.ExportCV();
        result["details"] = qjson_to_string(cv_result);

        return result;
    }, "Perform cross-validation",
       py::arg("model"),
       py::arg("cv_type") = 1,  // Leave-one-out by default
       py::arg("folds") = 5);

    // AIC Comparison
    m.def("calculate_aic", [](const std::vector<AbstractModel*>& models) {
        QVector<QWeakPointer<AbstractModel>> qmodels;
        for (auto* model : models) {
            qmodels.append(QWeakPointer<AbstractModel>(model));
        }

        QJsonObject aic_result = StatisticTool::CalculateAICMetrics(qmodels);

        py::dict result;
        result["aic_values"] = qjson_to_string(aic_result);

        return result;
    }, "Calculate AIC for model comparison", py::arg("models"));

    // Model Comparison with Multiple Criteria
    m.def("compare_models", [](const std::vector<AbstractModel*>& models) {
        py::dict result;

        // AIC Comparison
        QVector<QWeakPointer<AbstractModel>> qmodels;
        for (auto* model : models) {
            qmodels.append(QWeakPointer<AbstractModel>(model));
        }
        QJsonObject aic = StatisticTool::CalculateAICMetrics(qmodels);
        result["aic"] = qjson_to_string(aic);

        // Model statistics
        py::list model_stats;
        for (auto* model : models) {
            py::dict stats;
            stats["name"] = qstring_to_string(model->Name());
            stats["sse"] = model->SSE();
            stats["sey"] = model->SEy();
            stats["parameters"] = model->GlobalParameterSize() + model->LocalParameterSize();
            model_stats.append(stats);
        }
        result["models"] = model_stats;

        return result;
    }, "Compare multiple models using various criteria", py::arg("models"));

    // Percentile-based Confidence Intervals (Monte Carlo)
    m.def("confidence_intervals", [](AbstractModel* model, int iterations, double lower, double upper) {
        MonteCarloStatistics mc;
        mc.setModel(model);
        mc.setMaxIterations(iterations);

        bool success = mc.Evaluate();

        py::dict result;
        result["success"] = success;

        if (success) {
            QJsonObject stats = mc.ExportStatistics();
            result["intervals"] = qjson_to_string(stats);
        }

        return result;
    }, "Calculate confidence intervals using Monte Carlo percentiles",
       py::arg("model"),
       py::arg("iterations") = 10000,
       py::arg("lower") = 0.025,  // 2.5th percentile
       py::arg("upper") = 0.975); // 97.5th percentile

    // Statistical Summary
    m.def("statistical_summary", [](AbstractModel* model) {
        py::dict summary;

        // Basic statistics
        summary["sse"] = model->SSE();
        summary["sey"] = model->SEy();
        summary["data_points"] = model->DataPoints();
        summary["parameters"] = model->GlobalParameterSize() + model->LocalParameterSize();

        // Model info
        summary["name"] = qstring_to_string(model->Name());
        summary["uuid"] = qstring_to_string(model->UUID());

        // Parameters
        py::list global_params;
        for (int i = 0; i < model->GlobalParameterSize(); ++i) {
            global_params.append(model->GlobalParameter(i));
        }
        summary["global_parameters"] = global_params;

        py::list local_params;
        for (int i = 0; i < model->LocalParameterSize(); ++i) {
            local_params.append(model->LocalParameter(i));
        }
        summary["local_parameters"] = local_params;

        // Export full statistics
        QJsonObject stats = model->ExportStatistics();
        summary["full_statistics"] = qjson_to_string(stats);

        return summary;
    }, "Get comprehensive statistical summary of model", py::arg("model"));
}
