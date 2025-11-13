/*
 * Python Bindings for DataClass and DataTable
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Created by Claude Code AI Assistant
 *
 * This file provides Python bindings for SupraFit's data handling classes.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>

#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"

namespace py = pybind11;

// Helper functions
std::string qstring_to_string(const QString& qstr);
QString string_to_qstring(const std::string& str);
std::string qjson_to_string(const QJsonObject& obj);
QJsonObject string_to_qjson(const std::string& str);

void bind_dataclass(py::module_& m) {

    // DataTable bindings
    py::class_<DataTable, QObject>(m, "DataTable")
        .def(py::init<>())
        .def("rowCount", &DataTable::rowCount, "Get number of rows")
        .def("columnCount", &DataTable::columnCount, "Get number of columns")
        .def("data", [](const DataTable& self, int row, int col) {
            return self.data(row, col);
        }, "Get data at row, column")
        .def("setData", [](DataTable& self, int row, int col, double value) {
            self.setData(row, col, value);
        }, "Set data at row, column")
        .def("Table", &DataTable::Table, "Get data as Eigen matrix")
        .def("toList", [](const DataTable& self) {
            // Convert Eigen matrix to Python list of lists
            auto matrix = self.Table();
            std::vector<std::vector<double>> result;
            for (int i = 0; i < matrix.rows(); ++i) {
                std::vector<double> row;
                for (int j = 0; j < matrix.cols(); ++j) {
                    row.push_back(matrix(i, j));
                }
                result.push_back(row);
            }
            return result;
        }, "Convert data to Python list of lists");

    // SystemParameter bindings
    py::class_<SystemParameter>(m, "SystemParameter")
        .def(py::init<>())
        .def("Name", [](const SystemParameter& self) {
            return qstring_to_string(self.Name());
        })
        .def("Description", [](const SystemParameter& self) {
            return qstring_to_string(self.Description());
        })
        .def("Double", &SystemParameter::Double)
        .def("Bool", &SystemParameter::Bool)
        .def("getString", [](const SystemParameter& self) {
            return qstring_to_string(self.getString());
        })
        .def("setValue", [](SystemParameter& self, double value) {
            self.setValue(QVariant(value));
        });

    // DataClass bindings
    py::class_<DataClass, QObject>(m, "DataClass")
        .def(py::init<>())
        .def(py::init([](const std::string& json_str) {
            QJsonObject json = string_to_qjson(json_str);
            return new DataClass(json);
        }), "Create DataClass from JSON string")

        // Basic properties
        .def("UUID", [](const DataClass& self) {
            return qstring_to_string(self.UUID());
        })
        .def("Size", &DataClass::Size, "Get size (number of data points)")
        .def("DataPoints", &DataClass::DataPoints, "Get number of data points")
        .def("SeriesCount", &DataClass::SeriesCount, "Get number of series")
        .def("IndependentVariableSize", &DataClass::IndependentVariableSize)

        // Data access
        .def("IndependentModel", [](DataClass& self) {
            return self.IndependentModel();
        }, py::return_value_policy::reference)
        .def("DependentModel", [](DataClass& self) {
            return self.DependentModel();
        }, py::return_value_policy::reference)

        // Data range
        .def("setDataBegin", &DataClass::setDataBegin, "Set data range start")
        .def("setDataEnd", &DataClass::setDataEnd, "Set data range end")
        .def("DataBegin", &DataClass::DataBegin, "Get data range start")
        .def("DataEnd", &DataClass::DataEnd, "Get data range end")

        // Import/Export
        .def("ExportData", [](const DataClass& self) {
            return qjson_to_string(self.ExportData());
        }, "Export data as JSON string")
        .def("ImportData", [](DataClass& self, const std::string& json_str) {
            QJsonObject json = string_to_qjson(json_str);
            return self.ImportData(json);
        }, "Import data from JSON string")

        // System parameters
        .def("getSystemParameter", &DataClass::getSystemParameter)
        .def("setSystemParameterValue", [](DataClass& self, int index, double value) {
            self.setSystemParameterValue(index, QVariant(value));
        })

        // Project info
        .def("ProjectTitle", [](const DataClass& self) {
            return qstring_to_string(self.ProjectTitle());
        })
        .def("setProjectTitle", [](DataClass& self, const std::string& title) {
            self.setProjectTitle(string_to_qstring(title));
        });
}
