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

    /*! \brief How many injections the volume vector should describe: the experiment's peak count.
     *
     * Every integrated peak is one titrant addition, so the volumes and the peaks must line up for
     * the (volume, heat) table to mean anything. Claude Generated */
    int injectionCount() const;

    /*! \brief Set one injection's volume, growing the vector with zeros if it is still short.
     *
     * For titrations whose step size varies and needs a per-point correction. Volumes feed
     * resultTable() only, never the net heat, so this deliberately emits nothing. Claude Generated */
    void setInjectionVolume(int index, qreal volume);

    /*! \brief Give every injection the same volume (a uniform-step titration).
     *
     * Resizes to injectionCount() and overwrites, so what gets stored and exported is exactly what
     * the caller asked for - no scalar that a renderer has to re-expand later. Claude Generated */
    void setUniformInjectionVolume(qreal volume);

    /*! \brief Extend the vector to injectionCount(), filling only the entries that are missing.
     *
     * Volumes already known - from the .itc file's @-lines or a manual edit - are kept, and the
     * vector is never shortened, so a temporary drop in the peak count cannot destroy them.
     * Claude Generated */
    void padInjectionVolumes(qreal fill);

    /*! \brief Enable or disable the dilution subtraction; recomputes on a real change.
     *
     * The flag is an input to the join, so flipping it has to re-join - otherwise the subtraction
     * silently stops (or starts) without the result following.
     *
     * It selects whether an **already integrated** dilution takes part; it does not integrate one.
     * Enabling it on a dilution that process() never ran over subtracts nothing (an empty integral
     * list reads as zero heat), so enable it before process(), or after the handler has been
     * integrated - which is what loading a dilution file does. Claude Generated */
    void setDilutionEnabled(bool enabled);
    bool dilutionEnabled() const { return m_use_dilution; }

    /*! \brief Set the cal->J scaling factor on both handlers and re-scale them.
     *
     * cal->J converts the *measured* heat, so experiment and dilution must share it: subtracting a
     * joule-scaled dilution from a calorie-scaled experiment is meaningless. The re-scale is folded
     * in because a caller that forgets it gets a stale net heat with no signal to say so.
     * Idempotent, which also stops two UI controls bound to this from ping-ponging. Claude Generated */
    void setScalingFactor(qreal factor);

    //! The shared cal->J factor; both handlers hold the same value by construction. Claude Generated
    qreal scalingFactor() const;

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
    /*! \brief The net heat changed. Injection-volume changes do not emit: they are an input to
     * resultTable(), not to the join. */
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
