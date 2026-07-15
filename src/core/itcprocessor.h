/*
 * SupraFit ITC thermogram import orchestrator
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

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QVector>

class ThermogramHandler;
class DataTable;

/*! \brief GUI-independent orchestrator for ITC thermogram import. Claude Generated
 *
 * Owns the experiment (and optional dilution) `ThermogramHandler`, the per-injection volumes,
 * and the experiment-minus-dilution join, producing the final (injection volume, net heat) table
 * the ITC models consume. This is the single home for the ITC-specific logic that previously lived
 * only in the `Thermogram` import dialog, so the whole pipeline runs **without a GUI** — the same
 * object is used by `FileHandler::ReadITC`, the GUI dialog, and the headless test.
 *
 * The two handlers are owned (QObject children); a GUI can hand them to `ThermogramWidget`s for
 * interactive editing. Whenever either handler re-integrates, the net heat is recomputed and
 * `resultChanged()` is emitted. */
class ItcProcessor : public QObject {
    Q_OBJECT
public:
    explicit ItcProcessor(QObject* parent = nullptr);
    ~ItcProcessor();

    ThermogramHandler* experiment() const { return m_experiment; }
    ThermogramHandler* dilution() const { return m_dilution; }

    void setInjectionVolumes(const QVector<qreal>& inject) { m_inject = inject; }
    QVector<qreal> injectionVolumes() const { return m_inject; }

    void setDilutionEnabled(bool enabled) { m_use_dilution = enabled; }
    bool dilutionEnabled() const { return m_use_dilution; }

    //! Set the cal->J heat scaling factor on both handlers.
    void setScalingFactor(qreal factor);

    //! Drive both handlers through the full integration pipeline and compute the net heat (headless).
    void process();

    //! Scaled experiment heat minus (optional) scaled dilution heat, per injection.
    QVector<qreal> netHeat() const { return m_net_heat; }

    //! New, caller-owned DataTable with columns (injection volume, net heat [J]).
    DataTable* resultTable() const;

    //! Canonical JSON of the full ITC import (experiment + optional dilution fit blocks + inject volumes).
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);

signals:
    void resultChanged();

private slots:
    void recomputeNetHeat();

private:
    ThermogramHandler* m_experiment = nullptr;
    ThermogramHandler* m_dilution = nullptr;
    QVector<qreal> m_inject; //!< per-injection volumes (owned here; the handlers have no notion of them)
    QVector<qreal> m_net_heat;
    bool m_use_dilution = false;
};
