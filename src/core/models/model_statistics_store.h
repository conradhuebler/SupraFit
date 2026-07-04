/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QtGlobal>

#include "src/global.h"

/*!
 * \brief The model's post-processing results store (Claude Generated 2026, R2).
 *
 * Extracted from AbstractModel: the six per-method result lists (MonteCarlo, CrossValidation,
 * WeakenedGridSearch, ModelComparison, GlobalSearch, Reduction) plus the single FastConfidence
 * object, with the timestamp-deduplicated update, keyed get/remove/clear/size, and the const
 * list accessors used by the model's export loop. Behaviour is a faithful port of the previous
 * inline switch logic — including Reduction's ReductionRuntype-based replace and its
 * out-of-range-remove-returns-true quirk, and the selective CopyStatistics subset.
 */
class ModelStatisticsStore {
public:
    /*! \brief Store a result for \a method. MC/CV/WGS/MoCo/GlobalSearch: timestamp-deduplicated
     * update-or-append. Reduction: replace the entry whose ReductionRuntype matches, else append.
     * FastConfidence: replace the single object (sets \a fastConfidenceStored). Returns the index. */
    int update(SupraFit::Method method, const QJsonObject& object, bool& fastConfidenceStored)
    {
        fastConfidenceStored = false;
        if (method == SupraFit::Method::FastConfidence) {
            QJsonObject updatedObject = object;
            QJsonObject updatedController = object["controller"].toObject();
            updatedController["run_index"] = 0;
            updatedObject["controller"] = updatedController;
            m_fast_confidence = updatedObject;
            fastConfidenceStored = true;
            return 0;
        }
        if (method == SupraFit::Method::Reduction) {
            QJsonObject updatedObject = object;
            QJsonObject updatedController = object["controller"].toObject();
            int index = 0, match = 0;
            for (int i = 0; i < m_reduction.size(); ++i) {
                if (m_reduction[i]["controller"].toObject()["ReductionRuntype"].toInt() == updatedController["ReductionRuntype"].toInt()) {
                    m_reduction[i] = updatedObject;
                    index = i;
                    match++;
                }
            }
            if (match == 0) {
                updatedController["run_index"] = m_reduction.size();
                updatedObject["controller"] = updatedController;
                m_reduction << updatedObject;
                index = m_reduction.size() - 1;
            }
            return index;
        }
        return upsertByTimestamp(listFor(method), object);
    }

    QJsonObject get(SupraFit::Method type, int index) const
    {
        if (type == SupraFit::Method::FastConfidence)
            return m_fast_confidence;
        const QList<QJsonObject>& l = listFor(type);
        if (index < l.size())
            return l[index];
        return QJsonObject();
    }

    bool remove(SupraFit::Method type, int index)
    {
        if (type == SupraFit::Method::FastConfidence) {
            m_fast_confidence = QJsonObject();
            return true;
        }
        QList<QJsonObject>& l = listFor(type);
        if (index < l.size()) {
            l.takeAt(index);
            return true;
        }
        // Reduction historically treats an out-of-range remove as a no-op success; others fail.
        return type == SupraFit::Method::Reduction;
    }

    void clear()
    {
        m_mc_statistics.clear();
        m_cv_statistics.clear();
        m_wg_statistics.clear();
        m_moco_statistics.clear();
        m_search_results.clear();
        m_reduction.clear();
        m_fast_confidence = QJsonObject();
    }

    int size(SupraFit::Method type) const
    {
        if (type == SupraFit::Method::FastConfidence)
            return m_fast_confidence.isEmpty() ? 0 : 1;
        return listFor(type).size();
    }

    //! Append a result to a method list without deduplication (used by legacy import).
    void append(SupraFit::Method method, const QJsonObject& object) { listFor(method) << object; }

    const QJsonObject& fastConfidence() const { return m_fast_confidence; }
    const QList<QJsonObject>& list(SupraFit::Method type) const { return listFor(type); }

    //! Selective copy matching the previous CopyStatistic: MC, WGS, MoCo, Reduction, FastConfidence
    //! (deliberately not CrossValidation / GlobalSearch, preserving prior behaviour).
    void copyStatisticsFrom(const ModelStatisticsStore& other)
    {
        m_mc_statistics = other.m_mc_statistics;
        m_wg_statistics = other.m_wg_statistics;
        m_moco_statistics = other.m_moco_statistics;
        m_reduction = other.m_reduction;
        m_fast_confidence = other.m_fast_confidence;
    }

private:
    QList<QJsonObject>& listFor(SupraFit::Method method)
    {
        switch (method) {
        case SupraFit::Method::MonteCarlo:
            return m_mc_statistics;
        case SupraFit::Method::CrossValidation:
            return m_cv_statistics;
        case SupraFit::Method::WeakenedGridSearch:
            return m_wg_statistics;
        case SupraFit::Method::ModelComparison:
            return m_moco_statistics;
        case SupraFit::Method::GlobalSearch:
            return m_search_results;
        case SupraFit::Method::Reduction:
            return m_reduction;
        default:
            break;
        }
        static QList<QJsonObject> none; // FastConfidence / unknown have no backing list
        return none;
    }

    const QList<QJsonObject>& listFor(SupraFit::Method method) const
    {
        return const_cast<ModelStatisticsStore*>(this)->listFor(method);
    }

    int upsertByTimestamp(QList<QJsonObject>& list, const QJsonObject& object)
    {
        // Timestamp-based deduplication: update the entry with a matching timestamp, else append.
        QJsonObject updatedObject = object;
        QJsonObject updatedController = object["controller"].toObject();
        const double timestamp = updatedController["timestamp"].toDouble();

        for (int i = 0; i < list.size(); ++i) {
            const double existing = list[i]["controller"].toObject()["timestamp"].toDouble();
            if (qAbs(timestamp - existing) < 0.001) {
                updatedController["run_index"] = i;
                updatedObject["controller"] = updatedController;
                list[i] = updatedObject;
                return i;
            }
        }
        updatedController["run_index"] = list.size();
        updatedObject["controller"] = updatedController;
        list << updatedObject;
        return list.size() - 1;
    }

    QList<QJsonObject> m_mc_statistics;
    QList<QJsonObject> m_cv_statistics;
    QList<QJsonObject> m_wg_statistics;
    QList<QJsonObject> m_moco_statistics;
    QList<QJsonObject> m_search_results;
    QList<QJsonObject> m_reduction;
    QJsonObject m_fast_confidence;
};
