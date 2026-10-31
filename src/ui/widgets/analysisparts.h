/*
 * SupraFit - parts shared by the analysis dialogs
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
 * Claude Generated (roadmap/statistic_dialogs.md, WP-S1).
 */

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QWeakPointer>

#include <QtWidgets/QWidget>

class AbstractModel;
class QVBoxLayout;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;

/*! \brief Choice of the parameters an analysis tests one by one (Weakened Grid Search, Model Comparison). Claude Generated.
 *
 * With a model: one check box and name per global and per local parameter (the globals ticked, the locals not; a parameter the model does
 * not fit is disabled). Without a model (the dialog of the whole project) there is nothing to pick from: a choice says whether the local
 * parameters are tested as well, and the lists are made when the job runs on a model (`ResolveFor`). The lists of the job have the meaning
 * they always had: one flag per fitted parameter, a local parameter counts for every series.
 */
class ParameterSelector : public QWidget {
    Q_OBJECT

public:
    /*! \param withScaling the Model Comparison scales the box of every parameter (`ScalingWidget`) */
    ParameterSelector(const QSharedPointer<AbstractModel>& model, bool withScaling, QWidget* parent = nullptr);

    /*! \brief `GlobalParameterList` and `LocalParameterList` (and with scaling the two scaling lists) of \p job; without a model the
     * markers `AllParameters`, `AllLocalParameters` and `BoxScaling` for `ResolveFor`. */
    void AddTo(QJsonObject& job) const;
    /*! \brief One labelled box factor per parameter, to put behind "..more.."; nullptr without scaling or model. */
    QWidget* ScalingWidget() const { return m_scaling; }
    bool HasModel() const { return !m_model.isNull(); }

    /*! \brief For a job made without a model (`AllParameters`): the parameter lists, the box scaling (1.5, as the dialog offers), and the
     * limit of the sum of squares (`MaxParameter`) and the F value (`f-value`) of \p confidence for \p model. Other jobs stay as they are. */
    static void ResolveFor(QJsonObject& job, const QSharedPointer<AbstractModel>& model);

private:
    QWeakPointer<AbstractModel> m_model;
    bool m_withScaling;
    QList<QCheckBox*> m_global, m_local;
    QList<QDoubleSpinBox*> m_globalScale, m_localScale;
    QComboBox* m_scope = nullptr;
    QWidget* m_scaling = nullptr;
};

/*! \brief Puts \p more behind a "..more.." toggle at the end of \p layout (the state is a setting under \p id). Claude Generated. */
void AddFolded(QVBoxLayout* layout, QWidget* more, const QString& id);
