/*
 * Python Bindings for DataClass and DataTable
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Created by Claude Code AI Assistant
 *
 * This file provides Python bindings for SupraFit's data handling classes.
 * Uses lambda wrappers to avoid Qt type exposure (no QObject base classes).
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>

#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QStringList>

#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"

namespace py = pybind11;

// Helper functions (declared in bindings_main.cpp)
std::string qstring_to_string(const QString& qstr);
QString string_to_qstring(const std::string& str);
std::string qjson_to_string(const QJsonObject& obj);
QJsonObject string_to_qjson(const std::string& str);

void bind_dataclass(py::module_& m) {

    // DataTable bindings - WITHOUT QObject base class
    // Uses lambda wrappers to avoid Qt type exposure
    py::class_<DataTable>(m, "DataTable",
        "Data table for numerical data with row/column operations.\n"
        "Uses Eigen matrices internally for efficient numerical operations.")

        .def(py::init<>(),
             "Create empty DataTable")
        .def(py::init<int, int, QObject*>(),
             py::arg("rows"), py::arg("cols"), py::arg("parent") = nullptr,
             "Create DataTable with specified dimensions")

        // Core dimensions
        .def("rowCount",
             [](const DataTable& self) { return self.rowCount(); },
             "Get number of rows")
        .def("columnCount",
             [](const DataTable& self) { return self.columnCount(); },
             "Get number of columns")

        // Data access - Eigen matrix reference
        .def("Table",
             [](DataTable& self) -> Eigen::MatrixXd& { return self.Table(); },
             py::return_value_policy::reference_internal,
             "Get reference to internal Eigen matrix.\n"
             "CAUTION: Direct modifications affect the original data.")
        .def("data",
             py::overload_cast<int, int>(&DataTable::data, py::const_),
             py::arg("row"), py::arg("col") = 0,
             "Get data value at (row, col)")

        // Headers
        .def("header",
             [](const DataTable& self) {
                 QStringList qlist = self.header();
                 std::vector<std::string> result;
                 for (const QString& s : qlist) result.push_back(s.toStdString());
                 return result;
             },
             "Get column headers as Python list")
        .def("setHeader",
             [](DataTable& self, const std::vector<std::string>& headers) {
                 QStringList qlist;
                 for (const std::string& s : headers) qlist << QString::fromStdString(s);
                 self.setHeader(qlist);
             },
             py::arg("header"),
             "Set column headers from Python list")

        // Conversions
        .def("toVector",
             [](const DataTable& self) {
                 QVector<qreal> qv = self.toVector();
                 std::vector<double> result;
                 for (qreal v : qv) result.push_back(static_cast<double>(v));
                 return result;
             },
             "Convert entire table to Python list")
        .def("toList",
             [](const DataTable& self) {
                 QList<qreal> ql = self.toList();
                 std::vector<double> result;
                 for (qreal v : ql) result.push_back(static_cast<double>(v));
                 return result;
             },
             "Convert entire table to Python list (alternative)")

        // Row/Column operations
        .def("Row",
             [](const DataTable& self, int row) {
                 Eigen::VectorXd v = self.Row(row);
                 // Convert Eigen vector to std::vector
                 std::vector<double> result(v.data(), v.data() + v.size());
                 return result;
             },
             py::arg("row"),
             "Get row as Python list")
        .def("Column",
             [](const DataTable& self, int col) {
                 Eigen::VectorXd v = self.Column(col);
                 // Convert Eigen vector to std::vector
                 std::vector<double> result(v.data(), v.data() + v.size());
                 return result;
             },
             py::arg("col"),
             "Get column as Python list")
        .def("setRow",
             py::overload_cast<const Eigen::VectorXd&, int>(&DataTable::setRow),
             py::arg("vector"), py::arg("row"),
             "Set row from Eigen::VectorXd")
        .def("setColumn",
             py::overload_cast<const Eigen::VectorXd&, int>(&DataTable::setColumn),
             py::arg("vector"), py::arg("column"),
             "Set column from Eigen::VectorXd")

        // Row manipulation
        .def("insertRow",
             py::overload_cast<const Eigen::VectorXd&, bool>(&DataTable::insertRow),
             py::arg("row"), py::arg("zero") = false,
             "Insert a new row with given values")
        .def("appendColumns",
             [](DataTable& self, const DataTable& columns, bool keep_header) {
                 self.appendColumns(columns, keep_header);
             },
             py::arg("columns"), py::arg("keep_header") = true,
             "Append columns from another DataTable")

        // Checked state (for data filtering/selection)
        .def("isChecked", &DataTable::isChecked,
             py::arg("row"), py::arg("column") = 0,
             "Check if row is enabled for analysis")
        .def("isRowChecked", &DataTable::isRowChecked,
             py::arg("row"),
             "Check if entire row is enabled")
        .def("CheckRow",
             py::overload_cast<int, bool>(&DataTable::CheckRow),
             py::arg("row"), py::arg("check"),
             "Enable/disable a row")
        .def("EnableAllRows", &DataTable::EnableAllRows,
             "Enable all rows for analysis")
        .def("setCheckedAll", &DataTable::setCheckedAll,
             py::arg("checked"),
             "Set all rows to checked or unchecked")
        .def("EnabledRows", &DataTable::EnabledRows,
             "Get count of enabled rows")

        // Checked state query
        .def("CheckedTable",
             [](const DataTable& self) {
                 return self.CheckedTable();  // Returns Eigen matrix
             },
             "Get checked state matrix as Eigen::MatrixXd")
        .def("setCheckedTable",
             [](DataTable& self, const Eigen::MatrixXd& checked) {
                 self.setCheckedTable(checked);
             },
             py::arg("checked"),
             "Set checked state from Eigen matrix")

        // Block operations (simplified version)
        // Note: BlockRows/BlockColumns removed due to pybind11 limitations
        // Use Table() and slice the Eigen matrix instead

        // Import/Export
        .def("ExportTable",
             [](const DataTable& self, bool checked) {
                 return qjson_to_string(self.ExportTable(checked));
             },
             py::arg("checked") = false,
             "Export table data as JSON string")
        .def("ImportTable",
             [](DataTable& self, const std::string& json_str) {
                 return self.ImportTable(string_to_qjson(json_str));
             },
             py::arg("json"),
             "Import table data from JSON string")

        // Debugging
        .def("Debug",
             [](const DataTable& self, const std::string& str = "None") {
                 self.Debug(QString::fromStdString(str));
             },
             py::arg("label") = "None",
             "Print debug information");

    // SystemParameter bindings (no changes needed - no Qt base class)
    py::class_<SystemParameter>(m, "SystemParameter",
        "System parameter with name, value, and metadata")
        .def(py::init<>())
        .def("Name", [](const SystemParameter& self) {
            return qstring_to_string(self.Name());
        }, "Get parameter name")
        .def("Description", [](const SystemParameter& self) {
            return qstring_to_string(self.Description());
        }, "Get parameter description")
        .def("Double", &SystemParameter::Double, "Get value as double")
        .def("Bool", &SystemParameter::Bool, "Get value as boolean")
        .def("getString", [](const SystemParameter& self) {
            return qstring_to_string(self.getString());
        }, "Get value as string")
        .def("setValue", [](SystemParameter& self, double value) {
            self.setValue(QVariant(value));
        }, "Set value");

    // DataClass bindings - WITHOUT QObject base class
    py::class_<DataClass>(m, "DataClass",
        "Main data container for SupraFit projects.\n"
        "Contains independent and dependent data tables, system parameters,\n"
        "and project metadata.")

        .def(py::init<>(), "Create empty DataClass")
        .def(py::init([](const std::string& json_str) {
            QJsonObject json = string_to_qjson(json_str);
            return new DataClass(json);
        }), "Create DataClass from JSON string")

        // Basic properties
        .def("UUID", [](const DataClass& self) {
            return qstring_to_string(self.UUID());
        }, "Get unique identifier")
        .def("Size", &DataClass::Size,
             "Get size (number of data points)")
        .def("DataPoints", &DataClass::DataPoints,
             "Get number of data points")
        .def("SeriesCount", &DataClass::SeriesCount,
             "Get number of data series")
        .def("IndependentVariableSize", &DataClass::IndependentVariableSize,
             "Get size of independent variables")

        // Data access - CRITICAL: Return DataTable references
        .def("IndependentModel", [](DataClass& self) {
            return self.IndependentModel();
        }, py::return_value_policy::reference,
           "Get independent variable DataTable (reference)")
        .def("DependentModel", [](DataClass& self) {
            return self.DependentModel();
        }, py::return_value_policy::reference,
           "Get dependent variable DataTable (reference)")
        .def("IndependentRawModel", [](DataClass& self) {
            return self.IndependentRawModel();
        }, py::return_value_policy::reference,
           "Get raw independent variable DataTable (reference)")
        .def("DependentRawModel", [](DataClass& self) {
            return self.DependentRawModel();
        }, py::return_value_policy::reference,
           "Get raw dependent variable DataTable (reference)")

        // Data range
        .def("setDataBegin", &DataClass::setDataBegin,
             py::arg("begin"),
             "Set data range start index")
        .def("setDataEnd", &DataClass::setDataEnd,
             py::arg("end"),
             "Set data range end index")
        .def("DataBegin", &DataClass::DataBegin,
             "Get data range start index")
        .def("DataEnd", &DataClass::DataEnd,
             "Get data range end index")

        // Data manipulation
        // Note: setIndependentModel/setDependentModel not available in public API

        // Import/Export
        .def("ExportData", [](const DataClass& self) {
            return qjson_to_string(self.ExportData());
        }, "Export data as JSON string")
        .def("ImportData", [](DataClass& self, const std::string& json_str) {
            QJsonObject json = string_to_qjson(json_str);
            return self.ImportData(json);
        }, "Import data from JSON string")

        // System parameters
        .def("getSystemParameter", &DataClass::getSystemParameter,
             py::arg("index"),
             "Get system parameter by index")
        .def("setSystemParameterValue",
             [](DataClass& self, int index, double value) {
                 self.setSystemParameterValue(index, QVariant(value));
             },
             py::arg("index"), py::arg("value"),
             "Set system parameter value")

        // Project info
        .def("ProjectTitle", [](const DataClass& self) {
            return qstring_to_string(self.ProjectTitle());
        }, "Get project title")
        .def("setProjectTitle", [](DataClass& self, const std::string& title) {
            self.setProjectTitle(string_to_qstring(title));
        }, "Set project title");
}
