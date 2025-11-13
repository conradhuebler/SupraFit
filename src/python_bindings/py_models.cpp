/*
 * Python Bindings for Model Fitting
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Created by Claude Code AI Assistant
 *
 * This file provides Python bindings for SupraFit's model fitting functionality.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QSharedPointer>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/models.h"
#include "src/core/minimizer.h"

namespace py = pybind11;

// Helper functions
std::string qstring_to_string(const QString& qstr);
QString string_to_qstring(const std::string& str);
std::string qjson_to_string(const QJsonObject& obj);
QJsonObject string_to_qjson(const std::string& str);

void bind_models(py::module_& m) {

    // AbstractModel bindings (base class for all models)
    py::class_<AbstractModel, DataClass>(m, "AbstractModel")
        .def("Name", [](const AbstractModel& self) {
            return qstring_to_string(self.Name());
        }, "Get model name")

        // Parameters
        .def("GlobalParameterSize", &AbstractModel::GlobalParameterSize)
        .def("LocalParameterSize", &AbstractModel::LocalParameterSize)
        .def("GlobalParameter", &AbstractModel::GlobalParameter)
        .def("LocalParameter", &AbstractModel::LocalParameter)
        .def("setGlobalParameter", &AbstractModel::setGlobalParameter)
        .def("setLocalParameter", &AbstractModel::setLocalParameter)

        // Fitting
        .def("Calculate", &AbstractModel::Calculate, "Calculate model with current parameters")
        .def("SSE", &AbstractModel::SSE, "Sum of squared errors")
        .def("SEy", &AbstractModel::SEy, "Standard error")
        .def("isCalculated", &AbstractModel::isCalculated)

        // Statistics
        .def("StatisticVector", [](const AbstractModel& self) {
            auto vec = self.StatisticVector();
            return std::vector<double>(vec.begin(), vec.end());
        }, "Get statistics vector")

        // Model output
        .def("ModelTable", &AbstractModel::ModelTable,
             py::return_value_policy::reference,
             "Get calculated model values")

        // Export
        .def("ExportModel", [](const AbstractModel& self, bool statistics) {
            return qjson_to_string(self.ExportModel(statistics));
        }, "Export model as JSON string", py::arg("statistics") = true);

    // Model creation function
    m.def("create_model", [](const std::string& model_type, DataClass* data) {
        QString type = string_to_qstring(model_type);

        // Map string names to model types
        QSharedPointer<AbstractModel> model;

        if (type == "nmr_1_1") {
            model = QSharedPointer<AbstractModel>(new NMR_ItoI_Model(data));
        } else if (type == "nmr_2_1") {
            model = QSharedPointer<AbstractModel>(new NMR_IItoI_Model(data));
        } else if (type == "itc_1_1") {
            model = QSharedPointer<AbstractModel>(new ITC_ItoI_Model(data));
        } else if (type == "itc_1_2") {
            model = QSharedPointer<AbstractModel>(new ITC_ItoI_2_Model(data));
        } else if (type == "itc_2_1") {
            model = QSharedPointer<AbstractModel>(new ITC_IItoI_Model(data));
        } else if (type == "fl_1_1") {
            model = QSharedPointer<AbstractModel>(new FL_ItoI_Model(data));
        } else if (type == "uv_1_1") {
            model = QSharedPointer<AbstractModel>(new UVVIS_ItoI_Model(data));
        } else {
            throw std::runtime_error("Unknown model type: " + model_type);
        }

        return model;
    }, "Create a model by type name",
       py::arg("model_type"), py::arg("data"),
       py::return_value_policy::reference);

    // Fit function
    m.def("fit_model", [](AbstractModel* model) {
        // Perform optimization
        Minimizer minimizer;
        minimizer.setModel(model);
        bool success = minimizer.Minimize();

        // Return result
        py::dict result;
        result["success"] = success;
        result["sse"] = model->SSE();
        result["sey"] = model->SEy();

        return result;
    }, "Fit model to data", py::arg("model"));

    // Available models list
    m.def("available_models", []() {
        return std::vector<std::string>{
            "nmr_1_1", "nmr_2_1", "nmr_1_1_1_2", "nmr_2_1_1_1",
            "itc_1_1", "itc_1_2", "itc_2_1", "itc_2_2",
            "fl_1_1", "fl_1_1_1_2",
            "uv_1_1", "uv_1_1_1_2"
        };
    }, "Get list of available model types");
}
