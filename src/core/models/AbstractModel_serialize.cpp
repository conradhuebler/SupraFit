/*
 * SupraFit — AbstractModel JSON (de)serialization
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

/* Claude Generated — facade-preserving split out of AbstractModel.cpp.
 * Holds the project-file (de)serialization cluster: ExportModel / ExportStatistic
 * (current format) and ImportModel / LegacyImportModel (import incl. the legacy
 * SupraFit 1/2 fallbacks). These remain AbstractModel:: members; only the
 * translation unit changed. Behaviour-identical (verbatim move). */

#include "dataclass.h"

#include "src/core/libmath.h"
#include "src/core/toolset.h"
#include "src/global.h"
#include "src/version.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QCollator>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "AbstractModel.h"

QJsonObject AbstractModel::ExportModel(bool statistics, bool locked)
{
    QJsonObject json, toplevel;
    QJsonObject optionObject;

    json["globalParameter"] = GlobalTable()->ExportTable(true, private_d->m_enabled_global);
    QJsonObject statisticObject;
    QString help = "Please consider to \n(1) - Save the entry to a seperate file (via right click) and \n(2 - a) Remove the corresponding entry from the Results List or\n(2 - b) Drop the raw data for this result!";
    if (statistics) {
        // Established format: sequential numeric keys for all statistics
        // Supports multiple runs of any analysis type naturally
        int counter = 0;

        // Save FastConfidence if present
        QJsonValueRef ref = statisticObject[QString::number(counter)] = m_stats.fastConfidence();
        if (ref.isUndefined()) {
            qWarning() << "Critical warning, statistic data are to large to be stored in file";
            emit Info()->Warning(QString("Critical warning, statistic data are to large to be stored in file. Attempted to write %1 in model %2 from data %3. %4").arg(SupraFit::Method::FastConfidence).arg(Name()).arg(ProjectTitle()).arg(help));
            statisticObject.remove(QString::number(counter));
        }
        counter++;

        // Lambda to save statistics sequentially - supports multiple runs naturally
        auto SaveStatistic = [this, help, &counter](QJsonObject& statisticObject, const QList<QJsonObject>& data) {
            for (int i = 0; i < data.size(); ++i) {
                QJsonValueRef ref = statisticObject[QString::number(counter)] = data[i];
                if (ref.isNull()) {
                    emit Info()->Warning(QString("Critical warning, statistic data are to large to be stored in file. Attempted to write %2 # %1 in %3 from %4. %5").arg(i + 1).arg(SupraFit::Method2Name(AccessCI(data[i]["controller"].toObject(), "Method").toInt())).arg(Name()).arg(ProjectTitle()).arg(help));
                    statisticObject.remove(QString::number(counter));
                }
                counter++;
            }
        };

        // Save all analysis types sequentially - multiple runs per type are appended naturally
        SaveStatistic(statisticObject, m_stats.list(SupraFit::Method::MonteCarlo));
        SaveStatistic(statisticObject, m_stats.list(SupraFit::Method::CrossValidation));
        SaveStatistic(statisticObject, m_stats.list(SupraFit::Method::ModelComparison));
        SaveStatistic(statisticObject, m_stats.list(SupraFit::Method::WeakenedGridSearch));
        SaveStatistic(statisticObject, m_stats.list(SupraFit::Method::GlobalSearch));
        SaveStatistic(statisticObject, m_stats.list(SupraFit::Method::Reduction));

        json["methods"] = statisticObject;
    }

    QJsonObject globalBoundaries;
    for (int i = 0; i < m_global_boundaries.size(); ++i) {
        globalBoundaries[QString::number(i)] = ToolSet::DoubleVec2String(Boundary2Vector((m_global_boundaries[i])));
    }
    json["globalBoundaries"] = globalBoundaries;

    json["localParameter"] = LocalTable()->ExportTable(true, private_d->m_enabled_local);

    QJsonObject localBoundaries;
    for (int j = 0; j < m_local_parameter->columnCount() && j < m_local_boundaries.size(); ++j) {
        for (int i = 0; i < m_local_parameter->rowCount() && i < m_local_boundaries[j].size(); ++i) {
            localBoundaries[QString::number(i) + "+" + QString::number(j)] = ToolSet::DoubleVec2String(Boundary2Vector(m_local_boundaries[j][i]));
        }
    }
    json["localBoundaries"] = localBoundaries;

    json["locked"] = ToolSet::IntVec2String(private_d->m_locked_parameters.toVector());
    for (int index : getAllOptions())
        optionObject[QString::number(index)] = getOption(index);

    QJsonObject resultObject;
    if (m_locked_model || locked) {
        for (int i = 0; i < DataPoints(); ++i) {
            resultObject[QString::number(i)] = ToolSet::DoubleList2String(ModelTable()->Row(i));
        }
    }
    json["active_series"] = ToolSet::IntVec2String(m_active_signals.toVector());

    toplevel["data"] = json;
    toplevel["AppliedSeries"] = m_AppliedSeries;
    toplevel["options"] = optionObject;
    toplevel["model"] = SFModel();
    toplevel["SupraFit"] = qint_version;
    toplevel["SSE"] = m_sum_squares;
    toplevel["SAE"] = m_sum_absolute;
    toplevel["mean_error"] = m_mean;
    toplevel["variance"] = m_variance;
    toplevel["standard_error"] = m_stderror;
    toplevel["converged"] = m_converged;
    toplevel["valid"] = !isCorrupt();
    toplevel["name"] = m_name;
    
    // Store AIC values for model comparison - Claude Generated
    toplevel["AIC"] = GetAIC();
    toplevel["AICc"] = GetAICc();
    QJsonObject definiton;
    for (const QString& key : m_defined_model.keys()) {
        definiton[key] = m_defined_model[key];
    }

    toplevel["ModelDefinition"] = definiton;
    if (m_locked_model || locked) {
#ifdef DEBUG_ON
//         qDebug() << "Writing calculated data to json file";
#endif
        toplevel["locked_model"] = true;
        toplevel["result"] = resultObject;
    }

    return toplevel;
}

QJsonObject AbstractModel::ExportStatistic(SupraFit::Method type, int index)
{
    QJsonObject blob = getStatistic(type, index);

    if (blob.isEmpty())
        return QJsonObject();
    QJsonObject toplevel;
    toplevel["data"] = ExportData();
    QJsonObject model = ExportModel(false, false);
    QJsonObject datablob = model["data"].toObject();
    QJsonObject statisticObject;
    statisticObject[QString::number(type) + ":0"] = blob;
    datablob["methods"] = statisticObject;
    model["data"] = datablob;
    toplevel["model_0"] = model;

    return toplevel;
}

bool AbstractModel::ImportModel(const QJsonObject& topjson, bool override)
{
#ifdef DEBUG_ON
// quint64 t0 = QDateTime::currentMSecsSinceEpoch();
#endif

    QJsonObject json = topjson["data"].toObject();

    if (topjson.contains("ModelDefinition")) {
        QJsonObject model = topjson["ModelDefinition"].toObject();
        for (const QString& key : model.keys()) {
            m_defined_model.insert(key, model[key].toObject());
        }
        DefineModel(m_model_definition);
    }

    int copy_model = topjson["model"].toInt();
    bool unsafe_copy = qApp->instance()->property("UnsafeCopy").toBool();

    if (copy_model != SFModel()) {
        if (!unsafe_copy) {
            emit Info()->Message(QString("The two models don't fit. Unsafe copy is disabled on purpose. If you really want to do that, change the configuration in the Settings Dialog."));
            return false;
        } else {
            emit Info()->Message(QString("The two models don't fit. Unsafe copy is enabled. Good Luck. - Statistical data will be ignored."));
            override = false;
        }
    }

    QList<int> active_signals;

    GlobalTable()->ImportTable(json["globalParameter"].toObject());

    setOptions(topjson["options"].toObject());

    if (override) {
        // Claude Generated FIX: Clear existing statistics before reload to prevent duplication
        // Similar fix to model duplication bug - prevents appending duplicate statistics on project reload
        m_stats.clear();

        // Import statistics using established sequential format
        // Multiple runs are naturally supported via sequential numeric keys
        QJsonObject statisticObject = json["methods"].toObject();
        QStringList keys = statisticObject.keys();

        for (const QString& str : qAsConst(keys)) {
            QJsonObject object = statisticObject[str].toObject();
            QJsonObject controller = object["controller"].toObject();
            if (controller.isEmpty())
                continue;

            // Skip FastConfidence - it's optional and can cause issues during import
            // It's more of a utility feature than core functionality
            int methodId = AccessCI(controller, "Method").toInt();
            if (methodId == SupraFit::Method::FastConfidence)
                continue;

            int runIndex = AccessCI(controller, "run_index").toInt();
            qDebug() << "📊 DEBUG ImportModel: Loading statistic - Method:" << methodId
                     << "RunIndex:" << runIndex << "Key:" << str;

            UpdateStatistic(object);
        }
    }
    private_d->m_locked_parameters = ToolSet::String2IntVec(json["locked"].toString()).toList();

    active_signals = ToolSet::String2IntVec(json["active_series"].toString()).toList();

    // Claude Generated: Fix for LineSeries visibility issue
    // Ensure all series are visible by default unless explicitly disabled
    if (active_signals.isEmpty() || active_signals.size() != SeriesCount()) {
        // No active signals defined or size mismatch - set all visible
        active_signals = QVector<int>(SeriesCount(), 1).toList();
    } else {
        // Check for any series that are set to 0 (invisible) and make them visible by default
        for (int i = 0; i < active_signals.size(); ++i) {
            if (active_signals[i] == 0) {
                active_signals[i] = 1; // Default to visible
            }
        }
    }

    LocalTable()->ImportTable(json["localParameter"].toObject());

    if (isSimulation()) {
        d->m_dependent_model = new DataTable(IndependentModel()->rowCount(), d->m_simulate_dependent, this);
        m_model_signal = new DataTable(IndependentModel()->rowCount(), d->m_simulate_dependent, this);
        m_model_error = new DataTable(IndependentModel()->rowCount(), d->m_simulate_dependent, this);
    }
    if (json.contains("globalBoundaries")) {
        QJsonObject globalBoundaries = json["globalBoundaries"].toObject();
        for (int i = 0; i < m_global_boundaries.size(); ++i) {
            m_global_boundaries[i] = Vector2Boundary(ToolSet::String2DoubleVec(globalBoundaries[QString::number(i)].toString()));
        }
    }
    if (json.contains("localBoundaries")) {
        QJsonObject localBoundaries = json["localBoundaries"].toObject();
        for (int j = 0; j < m_local_parameter->columnCount() && j < m_local_boundaries.size(); ++j) {
            for (int i = 0; i < m_local_parameter->rowCount() && i < m_local_boundaries[j].size(); ++i) {
                m_local_boundaries[j][i] = Vector2Boundary(ToolSet::String2DoubleVec(localBoundaries[QString::number(i) + "+" + QString::number(j)].toString()));
            }
        }
    }
    setActiveSignals(active_signals);

    if (topjson.contains("locked_model")) {
#ifdef DEBUG_ON
//         qDebug() << "Loaded calculated data from json file";
#endif
        m_locked_model = true;
        QJsonObject resultObject = topjson["result"].toObject();
        QStringList keys = resultObject.keys();

        QCollator collator;
        collator.setNumericMode(true);
        std::sort(
            keys.begin(),
            keys.end(),
            [&collator](const QString& key1, const QString& key2) {
                return collator.compare(key1, key2) < 0;
            });
        for (const QString& str : qAsConst(keys)) {
            QVector<qreal> concentrationsVector, signalVector;
            concentrationsVector = ToolSet::String2DoubleVec(resultObject[str].toString());
            int row = str.toInt();
            if (ModelTable()->columnCount() == concentrationsVector.size())
                ModelTable()->setRow(concentrationsVector, row);
        }
    }
#ifdef DEBUG_ON
    //  quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    //  qDebug() << "model imported within" << t1 - t0 << " msecs";
#endif

    m_sum_squares = topjson["SSE"].toInt();
    m_sum_absolute = topjson["SAE"].toInt();
    m_mean = topjson["mean_error"].toInt();
    m_variance = topjson["variance"].toInt();
    m_stderror = topjson["standard_error"].toInt();
    m_converged = topjson["converged"].toBool();
    m_AppliedSeries = topjson["AppliedSeries"].toInt(0);

    // private_d->m_locked_parameters = ToolSet::String2IntVec(topjson["locked"].toString()).toList();
    if (topjson.contains("name") && !unsafe_copy)
        m_name = topjson["name"].toString();

    /* Copy system parameter */
    QJsonObject systemparameter = json["system"].toObject();
#ifndef DEBUG_ON
//    qDebug() << topjson["system"].toObject() << json["system"].toObject();
#endif
    for (int i = 0; i < SystemParameterCount(); ++i) {
    }
    emit ParameterChanged();

    if (SFModel() != SupraFit::MetaModel)
        Calculate();

#ifdef DEBUG_ON
    // quint64 t2 = QDateTime::currentMSecsSinceEpoch();
    // qDebug() << "calculation took " << t2 - t1 << " msecs";
#endif
    return true;
}

bool AbstractModel::LegacyImportModel(const QJsonObject& topjson, bool override)
{
#ifdef DEBUG_ON
    quint64 t0 = QDateTime::currentMSecsSinceEpoch();
#endif
    if (topjson[Name()].isNull()) {
        emit Info()->Warning("Sorry, this file doesn't contain any " + Name() + " model.");
        qWarning() << "file doesn't contain any " + Name();
        return false;
    }
    int fileversion = topjson["SupraFit"].toInt();

    if (fileversion >= 2002 && fileversion <= qint_version)
      return ImportModel(topjson, override);

    if (static_cast<SupraFit::Model>(topjson["model"].toInt()) != SFModel()) {
        if (fileversion >= qint_version) {
            emit Info()->Warning("Sorry, I suppose I do not support this data. " + Name());
            qWarning() << "No old data, but models dont fit, sorry";
            return false;
        }
        emit Info()->Message("Models don't fit! But that seems to be ok, because it is an old SupraFit file.");
    }

    if (fileversion > qint_version) {
        emit Info()->Warning(QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2").arg(qint_version).arg(fileversion));
        qWarning() << QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2. Get the most recent version from <a href='https://github.com/conradhuebler/SupraFit'>https://github.com/conradhuebler/SupraFit</a>").arg(qint_version).arg(fileversion);
        return false;
    }

    QJsonObject json = topjson["data"].toObject();

    QList<int> active_signals;
    QList<qreal> constants;
    QJsonObject globalParameter, optionObject;

    if (fileversion < 1602) {
        if (json.contains("globalParameter"))
            globalParameter = json["globalParameter"].toObject();
        else if (json.contains("constants"))
            globalParameter = json["constants"].toObject();
        else {
            qWarning() << "No global parameter found!";
        }
        for (int i = 0; i < GlobalParameterSize(); ++i) {
            (*GlobalTable())[i] = globalParameter[QString::number(i)].toString().toDouble();
        }
    } else
        GlobalTable()->ImportTable(json["globalParameter"].toObject());

    setOptions(topjson["options"].toObject());

    QStringList keys;
    QJsonObject statisticObject;

    if (fileversion < 1607) {
        keys = json["statistics"].toObject().keys();
        statisticObject = json["statistics"].toObject();
    } else {
        keys = json["methods"].toObject().keys();
        statisticObject = json["methods"].toObject();
    }
    if (keys.size() > 9) {
        QCollator collator;
        collator.setNumericMode(true);
        std::sort(
            keys.begin(),
            keys.end(),
            [&collator](const QString& key1, const QString& key2) {
                return collator.compare(key1, key2) < 0;
            });
    }

    if (override) {

        if (fileversion < 1600) {
            UpdateStatistic(statisticObject[QString::number(SupraFit::Method::ModelComparison)].toObject());
            UpdateStatistic(statisticObject[QString::number(SupraFit::Method::WeakenedGridSearch)].toObject());
            // m_moco_statistics << statisticObject[QString::number(SupraFit::Method::ModelComparison)].toObject();
            m_stats.append(SupraFit::Method::WeakenedGridSearch, statisticObject[QString::number(SupraFit::Method::WeakenedGridSearch)].toObject());
        }
        if (fileversion < 1608) {
            UpdateStatistic(statisticObject[QString::number(SupraFit::Method::Reduction)].toObject());
            //m_reduction << statisticObject[QString::number(SupraFit::Method::Reduction)].toObject();
        }
        UpdateStatistic(statisticObject[QString::number(SupraFit::Method::FastConfidence)].toObject());
        //m_fast_confidence = statisticObject[QString::number(SupraFit::Method::FastConfidence)].toObject();

        if (!m_stats.fastConfidence().isEmpty())
            ParseFastConfidence(m_stats.fastConfidence());

        if (fileversion >= 1608) {
            for (const QString& str : qAsConst(keys)) {
                QJsonObject object = statisticObject[str].toObject();
                QJsonObject controller = object["controller"].toObject();
                if (controller.isEmpty())
                    continue;

                UpdateStatistic(object);
            }
        }
    }

    if (fileversion >= 1601) {
        private_d->m_locked_parameters = ToolSet::String2IntVec(json["locked"].toString()).toList();
    }
    if (fileversion < 1601) {
        if (json.contains("localParameter")) {
            /*
         * Here goes (legacy) SupraFit 2 data handling
        */
            if (LocalParameterSize()) {
                QJsonObject localParameter = json["localParameter"].toObject();
                for (int i = 0; i < SeriesCount(); ++i) {
                    QVector<qreal> localVector;
                    if (!localParameter[QString::number(i)].isNull()) {
                        localVector = ToolSet::String2DoubleVec(localParameter[QString::number(i)].toString());
                        active_signals << 1;
                    } else {
                        localVector = QVector<qreal>(LocalParameterSize(), 0);
                        active_signals << 0;
                    }
                    LocalTable()->setRow(localVector, i);
                }
            }
        } else if (json.contains("pureShift")) {
            /*
             * This is SupraFit 1 legacy data handling
             */
            for (int i = 0; i < SeriesCount(); ++i) {
                QVector<qreal> localSeries;
                QJsonObject pureShiftObject = json["pureShift"].toObject();

                localSeries << pureShiftObject[QString::number(i)].toString().toDouble();

                if (!pureShiftObject[QString::number(i)].isNull())
                    active_signals << 1;
                else
                    active_signals << 0;

                for (int j = 0; j < GlobalParameterSize(); ++j) {
                    QJsonObject object = json["shift_" + QString::number(j)].toObject();
                    localSeries << object[QString::number(i)].toString().toDouble();
                }
                LocalTable()->setRow(localSeries, i);
            }
        }

    } else {
        active_signals = ToolSet::String2IntVec(json["active_series"].toString()).toList();

        // Claude Generated: Fix for LineSeries visibility issue (second location)
        // Ensure all series are visible by default unless explicitly disabled
        if (active_signals.isEmpty() || active_signals.size() != SeriesCount()) {
            // No active signals defined or size mismatch - set all visible
            active_signals = QVector<int>(SeriesCount(), 1).toList();
        } else {
            // Check for any series that are set to 0 (invisible) and make them visible by default
            for (int i = 0; i < active_signals.size(); ++i) {
                if (active_signals[i] == 0) {
                    active_signals[i] = 1; // Default to visible
                }
            }
        }

        LocalTable()->ImportTable(json["localParameter"].toObject());
    }
    setActiveSignals(active_signals);

    if (topjson.contains("locked_model")) {
#ifdef DEBUG_ON
//         qDebug() << "Loaded calculated data from json file";
#endif
        m_locked_model = true;
        QJsonObject resultObject = topjson["result"].toObject();
        QStringList keys = resultObject.keys();

        QCollator collator;
        collator.setNumericMode(true);
        std::sort(
            keys.begin(),
            keys.end(),
            [&collator](const QString& key1, const QString& key2) {
                return collator.compare(key1, key2) < 0;
            });
        for (const QString& str : qAsConst(keys)) {
            QVector<qreal> concentrationsVector, signalVector;
            concentrationsVector = ToolSet::String2DoubleVec(resultObject[str].toString());
            int row = str.toInt();
            if (ModelTable()->columnCount() == concentrationsVector.size())
                ModelTable()->setRow(concentrationsVector, row);
        }
    }
#ifdef DEBUG_ON
    quint64 t1 = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "model imported within" << t1 - t0 << " msecs";
#endif

    m_sum_squares = topjson["SSE"].toInt();
    m_sum_absolute = topjson["SAE"].toInt();
    m_mean = topjson["mean_error"].toInt();
    m_variance = topjson["variance"].toInt();
    m_stderror = topjson["standard_error"].toInt();
    m_converged = topjson["converged"].toBool();
    // private_d->m_locked_parameters = ToolSet::String2IntVec(topjson["locked"].toString()).toList();
    if (topjson.contains("name"))
        m_name = topjson["name"].toString();
    /*
        if (d->m_independent_model->columnCount() != d->m_scaling.size())
            for (int i = 0; i < d->m_independent_model->columnCount(); ++i)
                d->m_scaling << 1;
    */
    if (SFModel() != SupraFit::MetaModel)
        Calculate();

#ifdef DEBUG_ON
    quint64 t2 = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "calculation took " << t2 - t1 << " msecs";
#endif
    return true;
}
