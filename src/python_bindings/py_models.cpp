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

#include <QtCore/QString>

#include "src/core/models/AbstractModel.h"
#include "src/core/models/dataclass.h"

namespace py = pybind11;

// Helper functions
std::string qstring_to_string(const QString& qstr);
QString string_to_qstring(const std::string& str);

void bind_models(py::module_& m) {

    // Minimal AbstractModel bindings - WITHOUT QObject base class
    py::class_<AbstractModel, std::shared_ptr<AbstractModel>>(m, "AbstractModel",
        "Base class for all fitting models in SupraFit")

        .def("Name", [](const AbstractModel& self) {
            return qstring_to_string(self.Name());
        }, "Get model name")

        .def("GlobalParameterSize", [](const AbstractModel& self) {
            return self.GlobalParameterSize();
        }, "Get number of global parameters")

        .def("LocalParameterSize", [](const AbstractModel& self) {
            return self.LocalParameterSize();
        }, "Get number of local parameters")

        .def("DataPoints", [](const AbstractModel& self) {
            return self.DataPoints();
        }, "Get number of data points")

        .def("SeriesCount", [](const AbstractModel& self) {
            return self.SeriesCount();
        }, "Get number of series")

        .def("Calculate", [](AbstractModel& self) {
            self.Calculate();
        }, "Perform model calculation with current parameters")

        // Statistical properties
        .def("SSE", [](const AbstractModel& self) {
            return self.SSE();
        }, "Get sum of squared errors")

        .def("SEy", [](const AbstractModel& self) {
            return self.SEy();
        }, "Get standard error of y")

        .def("ChiSquared", [](const AbstractModel& self) {
            return self.ChiSquared();
        }, "Get chi-squared value")

        .def("sigma", [](const AbstractModel& self) {
            return self.sigma();
        }, "Get sigma (standard deviation)")

        // Access to results
        .def("ModelTable", [](AbstractModel& self) {
            return self.ModelTable();
        }, py::return_value_policy::reference,
           "Get calculated model table (DataTable reference)");

    // fit_model function - actual implementation
    m.def("fit_model",
        [](AbstractModel& model) -> py::dict {
            try {
                // Perform model calculation
                model.Calculate();

                // Build result dictionary
                py::dict result;
                result["success"] = true;
                result["sse"] = model.SSE();
                result["sey"] = model.SEy();
                result["chi_squared"] = model.ChiSquared();
                result["sigma"] = model.sigma();
                result["data_points"] = model.DataPoints();
                result["series_count"] = model.SeriesCount();

                return result;
            } catch (const std::exception& e) {
                py::dict result;
                result["success"] = false;
                result["error"] = std::string("Model calculation failed: ") + e.what();
                return result;
            } catch (...) {
                py::dict result;
                result["success"] = false;
                result["error"] = "Unknown error during model calculation";
                return result;
            }
        },
        "Fit/calculate a model with current parameters\n\n"
        "Parameters:\n"
        "  model (AbstractModel): Model to fit\n\n"
        "Returns:\n"
        "  dict: Result dictionary with keys:\n"
        "    - success (bool): Whether calculation succeeded\n"
        "    - sse (float): Sum of squared errors\n"
        "    - sey (float): Standard error of y\n"
        "    - chi_squared (float): Chi-squared value\n"
        "    - sigma (float): Sigma (standard deviation)\n"
        "    - data_points (int): Number of data points\n"
        "    - series_count (int): Number of series\n"
        "    - error (str): Error message if failed",
        py::arg("model")
    );

    // get_available_models function
    m.def("available_models",
        []() -> std::vector<std::string> {
            return {
                // NMR models
                "nmr_1_1",
                "nmr_2_1",
                "nmr_1_1_1_2",
                "nmr_2_1_1_1",
                "nmr_any",

                // ITC models
                "itc_1_1",
                "itc_1_2",
                "itc_2_1",
                "itc_2_2",
                "itc_n_1_1",
                "itc_n_1_2",
                "itc_any",

                // Fluorescence models
                "fl_1_1",
                "fl_1_1_1_2",
                "fl_2_1_1_1",

                // UV-Vis models
                "uv_1_1",
                "uv_2_1_1_1_1_2"
            };
        },
        "Get list of available model type names\n\n"
        "Returns:\n"
        "  list: Model type strings (e.g., 'nmr_1_1', 'itc_2_1')"
    );

    // Utility function: get model info
    m.def("model_info",
        [](const std::string& model_type) -> py::dict {
            py::dict info;
            info["type"] = model_type;

            // Provide basic info based on model type
            if (model_type.find("nmr") != std::string::npos) {
                info["category"] = "NMR Titration";
                info["method"] = "NMR (Nuclear Magnetic Resonance)";
            } else if (model_type.find("itc") != std::string::npos) {
                info["category"] = "ITC";
                info["method"] = "ITC (Isothermal Titration Calorimetry)";
            } else if (model_type.find("fl") != std::string::npos) {
                info["category"] = "Fluorescence";
                info["method"] = "Fluorescence Spectroscopy";
            } else if (model_type.find("uv") != std::string::npos) {
                info["category"] = "UV-Vis";
                info["method"] = "UV-Vis Spectroscopy";
            } else {
                info["category"] = "Unknown";
                info["method"] = "Unknown";
            }

            return info;
        },
        "Get information about a model type\n\n"
        "Parameters:\n"
        "  model_type (str): Model type name\n\n"
        "Returns:\n"
        "  dict: Model information",
        py::arg("model_type")
    );
}
