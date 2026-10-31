/*
 * SupraFit - the Scan: a grid of start values for the global search
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
 * Claude Generated (roadmap/statistic_dialogs.md, WP-S3).
 */

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QWeakPointer>

#include <QtWidgets/QWidget>

class AbstractModel;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;

/*! \brief The grid of the Scan: for each parameter a start, end and step, whether it is varied and whether it is fitted afterwards. Claude Generated.
 *
 * The global parameters (and the parameters of a model without series) are in sight; the local parameters of the series are behind
 * "..more.." (not varied by default). `Job()` is the job of the global search: `ParameterSize`, and for each parameter "min max step optimise".
 * "Fit" also sets the parameter as (not) optimised in the model, as the Scan always did.
 */
class ScanWidget : public QWidget {
    Q_OBJECT

public:
    explicit ScanWidget(const QSharedPointer<AbstractModel>& model, QWidget* parent = nullptr);

    /*! \brief Job of the global search for the current values; parameters that are not varied (or not fitted by the model) contribute their value. */
    QJsonObject Job() const;
    /*! \brief Number of calculations the grid asks for. */
    int Calculations() const;
    /*! \brief Range proposed for a parameter: a logarithmic constant (name starts with "lg") value ± 3, others from value/5 to 5 value; the step a tenth. */
    static void DefaultRange(const QString& name, double value, double& min, double& max, double& step);

signals:
    void CalculationsChanged(int count);

private:
    struct Row {
        QCheckBox *vary = nullptr, *fit = nullptr;
        QDoubleSpinBox *min = nullptr, *max = nullptr, *step = nullptr;
        QLabel* name = nullptr;
        QString plainName;
        bool global = true;
        int index = 0, series = 0;
        double value = 0;
    };
    QWidget* AddRow(const QString& name, double value, bool global, int index, int series, bool varied);
    QList<Row> m_rows;
    QLabel* m_count = nullptr;
    QWeakPointer<AbstractModel> m_model;
};
