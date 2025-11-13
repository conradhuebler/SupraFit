/*
 * Python Bindings for SupraFit using pybind11
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Created by Claude Code AI Assistant
 *
 * This file provides Python bindings for the SupraFit C++/Qt framework,
 * enabling Python users to:
 * - Load and manipulate data
 * - Create and fit models
 * - Perform statistical analysis (Monte Carlo, Cross-Validation, AIC)
 * - Export results
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/functional.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QString>
#include <QtCore/QVector>

#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"
#include "src/core/models/AbstractModel.h"
#include "src/core/models/models.h"
#include "src/core/filehandler.h"
#include "src/core/analyse.h"
#include "src/core/minimizer.h"
#include "src/capabilities/jobmanager.h"
#include "src/capabilities/montecarlostatistics.h"
#include "src/capabilities/modelcomparison.h"

namespace py = pybind11;

// Global QCoreApplication instance for Qt functionality
static QCoreApplication* g_app = nullptr;

// Initialize Qt application (required for Qt functionality)
void initialize_qt(int argc = 0, char** argv = nullptr) {
    if (!g_app) {
        static int _argc = argc;
        static char** _argv = argv;
        g_app = new QCoreApplication(_argc, _argv);
    }
}

// Helper functions for QString/Python string conversion
std::string qstring_to_string(const QString& qstr) {
    return qstr.toStdString();
}

QString string_to_qstring(const std::string& str) {
    return QString::fromStdString(str);
}

// Helper to convert QJsonObject to Python dict (as JSON string for simplicity)
std::string qjson_to_string(const QJsonObject& obj) {
    return QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
}

QJsonObject string_to_qjson(const std::string& str) {
    return QJsonDocument::fromJson(QByteArray::fromStdString(str)).object();
}

// Python module definition
PYBIND11_MODULE(suprafit, m) {
    m.doc() = "SupraFit Python Bindings - Supramolecular Chemistry Analysis";

    // Initialize Qt
    m.def("initialize", &initialize_qt,
          "Initialize Qt application (call once at startup)",
          py::arg("argc") = 0, py::arg("argv") = nullptr);

    // Version information
    m.attr("__version__") = "2.6.0";

    // Include submodules
    py::module_ io = m.def_submodule("io", "Input/Output operations");
    py::module_ data = m.def_submodule("data", "Data handling");
    py::module_ models = m.def_submodule("models", "Model fitting");
    py::module_ statistics = m.def_submodule("statistics", "Statistical analysis");
}
