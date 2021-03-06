/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * ScientificBox thanks to:
 * https://bugreports.qt.io/browse/QTBUG-7521
 */
#pragma once

#include "src/global_config.h"

#include "src/core/models/AbstractModel.h"

#include <QtCore/QJsonObject>

#include <QLocale>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QWidget>

class QValidator;
class QSpinBox;
class QCheckBox;
class QTabWidget;
class ScientificBox;

class OptimizerWidget : public QWidget {
    Q_OBJECT
public:
    OptimizerWidget(QJsonObject config, QWidget* parent = 0);

    ~OptimizerWidget();
    QJsonObject Config() const;

private:
    QWidget* GeneralWidget();
    QWidget* LevmarWidget();
    QWidget* AdvancedWidget();

    QJsonObject m_config;
    QTabWidget* m_tabwidget;
    QSpinBox *m_maxiter, *m_levmar_constants_periter, *m_sum_convergence, *m_levmar_factor, *m_single_iter, *m_levmar_maxfev;
    QCheckBox* m_skip_corrupt_concentrations;
    ScientificBox *m_concen_convergency, *m_constant_convergence, *m_error_convergence;
    ScientificBox *m_levmar_eps1, *m_levmar_eps2, *m_levmar_eps3, *m_levmar_delta;

    void setUi();
};
