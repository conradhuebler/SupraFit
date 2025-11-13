/*
 * Python Bindings Main Module
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Created by Claude Code AI Assistant
 *
 * This file combines all Python binding modules into a single suprafit module.
 */

#include <pybind11/pybind11.h>

#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QByteArray>

namespace py = pybind11;

// Forward declarations of binding functions
void bind_dataclass(py::module_& m);
void bind_models(py::module_& m);
void bind_statistics(py::module_& m);
void bind_io(py::module_& m);

// Helper function implementations (shared across all binding modules)
std::string qstring_to_string(const QString& qstr) {
    return qstr.toStdString();
}

QString string_to_qstring(const std::string& str) {
    return QString::fromStdString(str);
}

std::string qjson_to_string(const QJsonObject& obj) {
    return QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
}

QJsonObject string_to_qjson(const std::string& str) {
    return QJsonDocument::fromJson(QByteArray::fromStdString(str)).object();
}

// Python module definition
PYBIND11_MODULE(suprafit, m) {
    m.doc() = "SupraFit Python Bindings - Supramolecular Chemistry Analysis Framework";
    m.attr("__version__") = "2.6.0";

    // Create submodules
    py::module_ io = m.def_submodule("io", "Input/Output operations");
    py::module_ data_module = m.def_submodule("data", "Data handling");
    py::module_ models_module = m.def_submodule("models", "Model fitting");
    py::module_ statistics_module = m.def_submodule("statistics", "Statistical analysis");

    // Bind all components
    bind_dataclass(data_module);
    bind_models(models_module);
    bind_statistics(statistics_module);
    bind_io(io);

    // Convenience functions at top level
    m.def("load_data", [](const std::string& filename) {
        return io.attr("load_data")(filename, "auto");
    }, "Load data from file (convenience function)", py::arg("filename"));
}
