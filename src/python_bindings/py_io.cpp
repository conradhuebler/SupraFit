/*
 * Python Bindings for File I/O Operations
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Created by Claude Code AI Assistant
 *
 * This file provides Python bindings for SupraFit's file loading and saving.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QTextStream>

#include "src/core/filehandler.h"
#include "src/core/jsonhandler.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/AbstractModel.h"

namespace py = pybind11;

// Helper functions
std::string qstring_to_string(const QString& qstr);
QString string_to_qstring(const std::string& str);
std::string qjson_to_string(const QJsonObject& obj);
QJsonObject string_to_qjson(const std::string& str);

void bind_io(py::module_& m) {

    // Load data from file
    m.def("load_data", [](const std::string& filename, const std::string& format) {
        QString qfilename = string_to_qstring(filename);

        FileHandler handler(qfilename);
        DataClass* data = new DataClass();

        bool success = false;
        QString qformat = string_to_qstring(format);

        if (qformat == "auto" || qformat.isEmpty()) {
            success = handler.LoadFile(data);
        } else if (qformat == "json") {
            QFile file(qfilename);
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                success = data->ImportData(doc.object());
                file.close();
            }
        } else if (qformat == "txt" || qformat == "dat") {
            success = handler.LoadFile(data);
        } else if (qformat == "itc") {
            success = handler.LoadITCFile(data);
        }

        if (!success) {
            delete data;
            throw std::runtime_error("Failed to load file: " + filename);
        }

        return data;
    }, "Load data from file",
       py::arg("filename"),
       py::arg("format") = "auto",
       py::return_value_policy::take_ownership);

    // Save data to file
    m.def("save_data", [](const DataClass* data, const std::string& filename, const std::string& format) {
        QString qfilename = string_to_qstring(filename);
        QString qformat = string_to_qstring(format);

        QFile file(qfilename);
        if (!file.open(QIODevice::WriteOnly)) {
            throw std::runtime_error("Cannot open file for writing: " + filename);
        }

        if (qformat == "json") {
            QJsonObject json = data->ExportData();
            QJsonDocument doc(json);
            file.write(doc.toJson(QJsonDocument::Indented));
        } else if (qformat == "txt" || qformat == "dat") {
            QTextStream stream(&file);
            stream << data->Data2Text();
        } else {
            file.close();
            throw std::runtime_error("Unknown format: " + format);
        }

        file.close();
        return true;
    }, "Save data to file",
       py::arg("data"),
       py::arg("filename"),
       py::arg("format") = "json");

    // Load model from file
    m.def("load_model", [](const std::string& filename) {
        QString qfilename = string_to_qstring(filename);

        QFile file(qfilename);
        if (!file.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        QJsonObject json = doc.object();

        // Create DataClass first
        DataClass* data = new DataClass(json["data"].toObject());

        // Then create appropriate model
        // This is simplified - in practice, we'd need to check model type
        AbstractModel* model = nullptr;
        // Model creation logic would go here based on json["model_type"]

        if (!model) {
            delete data;
            throw std::runtime_error("Failed to create model from file");
        }

        return model;
    }, "Load model from JSON file",
       py::arg("filename"),
       py::return_value_policy::take_ownership);

    // Save model to file
    m.def("save_model", [](const AbstractModel* model, const std::string& filename, bool include_statistics) {
        QString qfilename = string_to_qstring(filename);

        QFile file(qfilename);
        if (!file.open(QIODevice::WriteOnly)) {
            throw std::runtime_error("Cannot open file for writing: " + filename);
        }

        QJsonObject json;
        json["data"] = model->ExportData();
        json["model"] = model->ExportModel(include_statistics);

        QJsonDocument doc(json);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        return true;
    }, "Save model to JSON file",
       py::arg("model"),
       py::arg("filename"),
       py::arg("include_statistics") = true);

    // Export to various formats
    m.def("export_results", [](const AbstractModel* model, const std::string& filename, const std::string& format) {
        QString qfilename = string_to_qstring(filename);
        QString qformat = string_to_qstring(format);

        QFile file(qfilename);
        if (!file.open(QIODevice::WriteOnly)) {
            throw std::runtime_error("Cannot open file for writing: " + filename);
        }

        QTextStream stream(&file);

        if (qformat == "csv") {
            // Export as CSV
            auto* indep = model->IndependentModel();
            auto* dep = model->DependentModel();
            auto* calc = model->ModelTable();

            // Header
            stream << "X,Y_observed,Y_calculated\n";

            // Data
            for (int i = 0; i < model->DataPoints(); ++i) {
                stream << indep->data(i, 0) << ","
                       << dep->data(i, 0) << ","
                       << calc->data(i, 0) << "\n";
            }
        } else if (qformat == "txt") {
            // Export as formatted text
            stream << "SupraFit Model Results\n";
            stream << "=====================\n\n";
            stream << "Model: " << model->Name() << "\n";
            stream << "SSE: " << model->SSE() << "\n";
            stream << "SEy: " << model->SEy() << "\n\n";

            // Parameters
            stream << "Global Parameters:\n";
            for (int i = 0; i < model->GlobalParameterSize(); ++i) {
                stream << "  " << i << ": " << model->GlobalParameter(i) << "\n";
            }

            stream << "\nLocal Parameters:\n";
            for (int i = 0; i < model->LocalParameterSize(); ++i) {
                stream << "  " << i << ": " << model->LocalParameter(i) << "\n";
            }
        } else {
            file.close();
            throw std::runtime_error("Unknown export format: " + format);
        }

        file.close();
        return true;
    }, "Export results to various formats (csv, txt)",
       py::arg("model"),
       py::arg("filename"),
       py::arg("format") = "csv");

    // Supported file formats
    m.def("supported_formats", []() {
        py::dict formats;
        formats["input"] = py::list({"txt", "dat", "json", "itc", "csv"});
        formats["output"] = py::list({"json", "txt", "csv"});
        return formats;
    }, "Get list of supported file formats");
}
