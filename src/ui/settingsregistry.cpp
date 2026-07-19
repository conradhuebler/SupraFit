/*
 * SupraFit application-settings registry
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QtCore/QObject>
#include <QtCore/QThread>

#include "settingsregistry.h"

namespace SupraFitSettings {

// Group (tab) names — kept as constants so the ConfigDialog generator and the registry agree.
static const QString gGeneral = QObject::tr("General Settings");
static const QString gSave = QObject::tr("File Save Settings");
static const QString gCalc = QObject::tr("Standard Calculation");
static const QString gChart = QObject::tr("Chart Settings");

namespace {
    /*! \brief Small fluent builder so the table below reads one setting per line. Claude Generated */
    struct Def {
        SettingDef d;
        Def(const QString& key, const QVariant& def, Kind kind)
        {
            d.key = key;
            d.defaultValue = def;
            d.kind = kind;
        }
        Def& group(const QString& g) { d.group = g; return *this; }
        Def& label(const QString& l) { d.label = l; return *this; }
        Def& tip(const QString& t) { d.tooltip = t; return *this; }
        Def& note(const QString& n) { d.note = n; return *this; }
        Def& range(double lo, double hi) { d.min = lo; d.max = hi; return *this; }
        Def& step(double s) { d.step = s; return *this; }
        Def& decimals(int n) { d.decimals = n; return *this; }
        Def& suffix(const QString& s) { d.suffix = s; return *this; }
        Def& dependsOn(const QString& k) { d.dependsOn = k; return *this; }
        Def& custom() { d.kind = Kind::Custom; return *this; }
        Def& hidden() { d.inDialog = false; return *this; } // persisted, but no auto widget
        Def& session() { d.persisted = false; return *this; } // session-only, never in QSettings
        Def& resetIfZero() { d.resetIfZero = true; return *this; }
        operator SettingDef() const { return d; }
    };
}

const QVector<SettingDef>& registry()
{
    static const QVector<SettingDef> r = [] {
        const int ideal = QThread::idealThreadCount();
        QVector<SettingDef> v;

        // ---- General Settings ----
        v << Def("threads", ideal, Kind::Int).group(gGeneral).label(QObject::tr("Threads:")).range(1, 2 * ideal);
        v << Def("ScriptTimeout", 500, Kind::Int).group(gGeneral).label(QObject::tr("Timeout for Script Model (mscs):")).range(-1, 1e6);
        v << Def("ModelParameterColums", 2, Kind::Int).group(gGeneral).label(QObject::tr("Columns for Model Parameter:")).range(1, 1e6);
        v << Def("dirlevel", 1, Kind::Int).group(gGeneral).note(QObject::tr("Set directory behavior to:")).custom(); // directory-mode radio group (hand-built)
        v << Def("workingdir", QVariant(), Kind::String).group(gGeneral).custom(); // working-dir line edit + picker (hand-built)
        v << Def("auto_thermo_dialog", false, Kind::Bool).group(gGeneral).label(QObject::tr("Automatic open Thermogram Dialog")).tip(QObject::tr("Open automatically the thermogram dialog, if the loaded or imported data look like a thermogram."));
        v << Def("thermogram_guidelines", true, Kind::Bool).group(gGeneral).label(QObject::tr("Show guidelines in Thermogram Dialog")).tip(QObject::tr("If checked, some guidelines in the Thermogram will be shown. Errors will shown regardless this option."));
        v << Def("energy_unit_kcal", false, Kind::Bool).group(gGeneral).label(QObject::tr("Report thermodynamics in kcal/mol (instead of kJ/mol)")).tip(QObject::tr("If checked, ΔG, ΔH and −TΔS are shown in kcal/mol and ΔS in cal/(mol·K), matching e.g. TA NanoAnalyze. Unchecked uses SI units (kJ/mol, J/(mol·K))."));
        v << Def("ColorFullSearch", true, Kind::Bool).group(gGeneral).label(QObject::tr("Colorfull global search table")).tip(QObject::tr("Indicate fully optimised models in global search with light green backgroud color, not-fully optimised models with light yellow background color and invalid models with a light red background color."));
        v << Def("advanced_ui", false, Kind::Bool).group(gGeneral).label(QObject::tr("Show advanced options and simulation tools in GUI.")).tip(QObject::tr("If checked, some more advanced options and tools are available via Graphical User Interface. To apply, please restart the SupraFit."));
        v << Def("tooltips", true, Kind::Bool).group(gGeneral).label(QObject::tr("Show Tooltips as quick help on selected widgets.")).tip(QObject::tr("If this checkbox were not checked, you would not see this tooltip.\nAnd now for something complete different:\nIf three witches had three watches, which witch would watch which watch?"));
        v << Def("InitialiseRandom", true, Kind::Bool).group(gGeneral).label(QObject::tr("Initialise Simulation with random numbers.")).tip(QObject::tr("If a new model is added to a simulation set, all parameter will be initialised as random numbers."));
        v << Def("UnsafeCopy", false, Kind::Bool).group(gGeneral).label(QObject::tr("Unsafe Copy of Models")).tip(QObject::tr("If this is enabled, on can copy the model parameter of any model into another via Drag 'n' Drop. If disabled, only Models of the same type are compatible. - Please, don't enable without purpose. SupraFit may behave inappropriately and crash ...."));
        v << Def("ask_on_exit", true, Kind::Bool).group(gGeneral).label(QObject::tr("Confirm quit of SupraFit")).tip(QObject::tr("Confirm quit of SupraFit"));
        v << Def("save_on_exit", true, Kind::Bool).session().hidden(); // widget currently not shown; not persisted (legacy behaviour)

        // ---- File Save Settings ----
        v << Def("StoreRawData", true, Kind::Bool).group(gSave).label(QObject::tr("Store raw data (thermograms etc) in SupraFit Project files.")).tip(QObject::tr("Store the original data within the SupraFit project. This may increase the file size and impact the save/write performance negativly or even crash the file for Qt < 5.15 due to the json bug."));
        v << Def("StoreFileName", true, Kind::Bool).group(gSave).label(QObject::tr("Store the filename, where the raw data were imported from, in SupraFit Project files."));
        v << Def("StoreAbsolutePath", false, Kind::Bool).group(gSave).label(QObject::tr("Store the absolute path of the file containing the raw data.")).note(QObject::tr("The above options are checked as default. Additionally, the absolute path\ncan be stored supplementary.This may be an infringement\nof your privacy, if you share the project file.")).dependsOn("StoreFileName").tip(QObject::tr("You can store the absolute path of the file with the raw data. If the project files gets moved around without the original data,\nSupraFit still knows where to look after the data. However, if you share your files, other people will know sth. about the file system structure on the original machine."));
        v << Def("StoreFileHash", false, Kind::Bool).dependsOn("StoreFileName").hidden(); // widget currently not shown
        v << Def("FindFileRecursive", false, Kind::Bool).hidden(); // widget currently not shown

        // ---- Standard Calculation ----
        v << Def("auto_confidence", true, Kind::Bool).group(gCalc).label(QObject::tr("Automatic Confidence Calculation using Simplified Model Comparison."));
        v << Def("series_confidence", false, Kind::Bool).group(gCalc).label(QObject::tr("Include Series in Automatic Confidence Calculation")).dependsOn("auto_confidence");
        v << Def("p_value", 0.95, Kind::Double).group(gCalc).label(QObject::tr("Confidence Interval")).range(0, 100).step(1E-2).decimals(2).suffix("%");
        v << Def("FastConfidenceSteps", 100, Kind::Int).group(gCalc).label(QObject::tr("Maximal Steps")).range(1, 100000);
        v << Def("FastConfidenceScaling", -4, Kind::Int).group(gCalc).label(QObject::tr("Single Steps Length")).range(-10, 1);
        v << Def("EntropyBins", 30, Kind::Int).group(gCalc).label(QObject::tr("# bins for Shannon Entropy Calculation")).range(10, 100000);
        v << Def("OverwriteBins", false, Kind::Bool).group(gCalc).label(QObject::tr("Overwrite stored bin number"));
        v << Def("FullShannon", false, Kind::Bool).group(gCalc).label(QObject::tr("Calculate full Shannon entropy!")).tip(QObject::tr("Calculate Shannon entropy including the discretisation term. Not recommended, as the ordering of appropriate parameters and models is reversed."));

        // ---- Chart Settings ----
        v << Def("MaxSeriesPoints", 200, Kind::Int).group(gChart).note(QObject::tr("General Chart Settings:")).label(QObject::tr("Maximal number of visualised points per series.")).range(0, 2147483647).resetIfZero();
        v << Def("markerSize", 6, Kind::Double).group(gChart).note(QObject::tr("Configure Chart Export Settings:")).label(QObject::tr("Define marker size for exported charts:")).range(0, 30);
        v << Def("lineWidth", 20, Kind::Double).group(gChart).label(QObject::tr("Define line width for exported charts:")).range(0, 30);
        v << Def("xSize", 600, Kind::Int).group(gChart).label(QObject::tr("Size of Chart x-Axis :")).range(0, 1e5);
        v << Def("ySize", 400, Kind::Int).group(gChart).label(QObject::tr("Size of Chart y-Axis :")).range(0, 1e5);
        v << Def("chartScaling", 4, Kind::Double).group(gChart).label(QObject::tr("Define the chart scaling ration :")).range(0, 30);
        v << Def("transparentChart", true, Kind::Bool).group(gChart).label(QObject::tr("Transparent Charts"));
        v << Def("cropedChart", true, Kind::Bool).group(gChart).label(QObject::tr("Remove Transparent Border from Charts")).dependsOn("transparentChart");
        v << Def("noGrid", true, Kind::Bool).group(gChart).label(QObject::tr("Remove Grid in exported Charts"));
        v << Def("empAxis", true, Kind::Bool).group(gChart).label(QObject::tr("Stronger Axis"));
        v << Def("PointFeedback", false, Kind::Bool).hidden(); // chart hover feedback (widget currently not shown)
        v << Def("ModuloPointFeedback", 0, Kind::Int).dependsOn("PointFeedback").hidden();
        v << Def("MarkerPointFeedback", 0, Kind::Double).hidden();

        // ---- Persisted-only (no dialog widget) ----
        v << Def("chartanimation", true, Kind::Bool).hidden();
        v << Def("charttheme", 0, Kind::Int).hidden();
        v << Def("calibration_start", 0, Kind::Int).hidden();
        v << Def("calibration_heat", 0, Kind::Int).hidden();
        v << Def("MetaSeries", false, Kind::Bool).hidden();
        v << Def("LastSpectraType", "csv", Kind::String).hidden();
        v << Def("lastSize", 2, Kind::Int).hidden();
        v << Def("lastdir", QVariant(), Kind::String).hidden(); // no default (runtime)
        v << Def("recent", QVariant(), Kind::String).hidden(); // no default (recent-files list)

        return v;
    }();
    return r;
}

const SettingDef* find(const QString& key)
{
    for (const SettingDef& d : registry()) {
        if (d.key == key)
            return &d;
    }
    return nullptr;
}

QStringList persistedKeys()
{
    QStringList keys;
    for (const SettingDef& d : registry()) {
        if (d.persisted)
            keys << d.key;
    }
    return keys;
}

QStringList applyDefaults(QObject* app)
{
    QStringList newlyInitialised;
    if (!app)
        return newlyInitialised;

    for (const SettingDef& d : registry()) {
        if (!d.defaultValue.isValid()) // e.g. lastdir/recent/workingdir have no built-in default
            continue;

        const QVariant current = app->property(qPrintable(d.key));
        const bool unset = (current == QVariant());
        const bool zeroReset = d.resetIfZero && current.toInt() == 0;

        if (unset || zeroReset)
            app->setProperty(qPrintable(d.key), d.defaultValue);
        if (unset)
            newlyInitialised << d.key;
    }
    return newlyInitialised;
}

}
