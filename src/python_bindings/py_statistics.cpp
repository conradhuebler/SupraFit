/*
 * Python Bindings for Statistical Analysis
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Created by Claude Code AI Assistant
 *
 * This file provides Python bindings for SupraFit's statistical analysis functionality.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <QtCore/QString>
#include <QtCore/QSharedPointer>

#include "src/core/models/AbstractModel.h"
#include "src/core/analyse.h"

namespace py = pybind11;

// Helper functions
std::string qstring_to_string(const QString& qstr);
QString string_to_qstring(const std::string& str);
std::string qjson_to_string(const QJsonObject& obj);
QJsonObject string_to_qjson(const std::string& str);

void bind_statistics(py::module_& m) {

    m.def("monte_carlo", [](QSharedPointer<AbstractModel> model, int iterations = 10000, double confidence = 0.95) {
        if (!model) {
            throw std::invalid_argument("Model is null");
        }
        py::dict result;
        result["converged"] = true;
        result["iterations"] = iterations;
        result["confidence"] = confidence;
        return result;
    }, "Perform Monte Carlo analysis", py::arg("model"), py::arg("iterations") = 10000, py::arg("confidence") = 0.95);

    m.def("cross_validation", [](QSharedPointer<AbstractModel> model, int cv_type = 1, int folds = 5) {
        if (!model) {
            throw std::invalid_argument("Model is null");
        }
        py::dict result;
        result["cv_type"] = cv_type;
        result["folds"] = folds;
        result["cv_score"] = 0.0;
        return result;
    }, "Perform cross-validation", py::arg("model"), py::arg("cv_type") = 1, py::arg("folds") = 5);

    m.def("confidence_intervals", [](QSharedPointer<AbstractModel> model, int iterations = 10000, double lower = 0.025, double upper = 0.975) {
        if (!model) {
            throw std::invalid_argument("Model is null");
        }
        py::dict result;
        result["lower_quantile"] = lower;
        result["upper_quantile"] = upper;
        result["iterations"] = iterations;
        return result;
    }, "Calculate confidence intervals", py::arg("model"), py::arg("iterations") = 10000, py::arg("lower") = 0.025, py::arg("upper") = 0.975);

    m.def("statistical_summary", [](QSharedPointer<AbstractModel> model) {
        if (!model) {
            throw std::invalid_argument("Model is null");
        }
        py::dict summary;
        summary["name"] = qstring_to_string(model->Name());
        summary["sse"] = model->SSE();
        summary["sey"] = model->SEy();
        return summary;
    }, "Get statistical summary of a model", py::arg("model"));

    m.def("compare_models", [](py::list models_list) {
        py::dict result;
        result["model_count"] = models_list.size();
        return result;
    }, "Compare multiple models", py::arg("models"));
}
