/*
 * Python Bindings for File I/O Operations
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Created by Claude Code AI Assistant
 *
 * This file provides Python bindings for SupraFit's file I/O functionality.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <QtCore/QString>
#include <QtCore/QJsonObject>

#include "src/core/models/dataclass.h"

namespace py = pybind11;

// Helper functions
std::string qstring_to_string(const QString& qstr);
QString string_to_qstring(const std::string& str);
std::string qjson_to_string(const QJsonObject& obj);
QJsonObject string_to_qjson(const std::string& str);

void bind_io(py::module_& m) {

    // Load model from JSON string
    m.def("load_data_json",
        [](const std::string& json_str) -> DataClass* {
            try {
                QJsonObject json = string_to_qjson(json_str);
                DataClass* data = new DataClass(json);

                if (!data) {
                    throw std::runtime_error("Failed to create DataClass from JSON");
                }

                return data;
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to load data from JSON: " + std::string(e.what()));
            }
        },
        "Create DataClass from JSON string\n\n"
        "Parameters:\n"
        "  json_str (str): JSON string representation of the data\n\n"
        "Returns:\n"
        "  DataClass: Data object created from JSON",
        py::arg("json_str"),
        py::return_value_policy::reference
    );

    // Save model to JSON string
    m.def("save_data_json",
        [](DataClass* data) -> std::string {
            try {
                if (!data) {
                    throw std::invalid_argument("DataClass object is null");
                }

                QJsonObject json = data->ExportData();
                return qjson_to_string(json);

            } catch (const std::invalid_argument& e) {
                throw std::invalid_argument(std::string(e.what()));
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to save data to JSON: " + std::string(e.what()));
            }
        },
        "Export DataClass to JSON string\n\n"
        "Parameters:\n"
        "  data (DataClass): Data object to export\n\n"
        "Returns:\n"
        "  str: JSON string representation of the data",
        py::arg("data")
    );

    // Supported file formats
    m.def("supported_formats",
        []() -> std::vector<std::string> {
            return {
                "json"     // JSON format (main supported format for Python)
            };
        },
        "Get list of supported file formats\n\n"
        "Returns:\n"
        "  list: List of supported format strings\n\n"
        "Note:\n"
        "  Currently JSON is the primary format for Python I/O.\n"
        "  For other formats (txt, dat, itc, csv), use the SupraFit GUI."
    );

    // Placeholder functions for future file I/O
    m.def("load_data",
        [](const std::string& filename) {
            throw std::runtime_error(
                "File I/O not yet implemented in Python bindings. "
                "Please use JSON I/O functions (load_data_json, save_data_json) "
                "or load/save files using the SupraFit GUI."
            );
        },
        "Load data from file (NOT YET IMPLEMENTED)\n\n"
        "Use load_data_json() with JSON strings instead.",
        py::arg("filename")
    );

    m.def("save_data",
        [](DataClass* data, const std::string& filename) {
            throw std::runtime_error(
                "File I/O not yet implemented in Python bindings. "
                "Please use JSON I/O functions (save_data_json) "
                "or save files using the SupraFit GUI."
            );
        },
        "Save data to file (NOT YET IMPLEMENTED)\n\n"
        "Use save_data_json() to get JSON strings instead.",
        py::arg("data"),
        py::arg("filename")
    );
}
