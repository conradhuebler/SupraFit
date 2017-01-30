/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifndef OPTIMIZERFLAGWIDGET_H
#define OPTIMIZERFLAGWIDGET_H

#include <QtCore/QPointer>
#include <QtWidgets/QGroupBox>


#include "src/global.h"

class QVBoxLayout;
class QCheckBox;

class OptimizerFlagWidget : public QGroupBox
{
    Q_OBJECT
public:
    OptimizerFlagWidget();
    OptimizerFlagWidget(OptimizationType type);
    ~OptimizerFlagWidget();
    OptimizationType getFlags() const;
    void DisableOptions(OptimizationType type);

private:
    OptimizationType m_type;
    void setUi();
    QPointer<QCheckBox > m_ComplexationConstants, m_IgnoreAllShifts, m_ConstrainedShifts, m_IntermediateShifts, m_IgnoreZeroConcentrations;
    QPushButton *m_more;
    QVBoxLayout *m_main_layout;
    QWidget *m_first_row;
private slots:
    void EnableShiftSelection();
    void ConstrainedChanged();
    void ShowFirst();
};

#endif // OPTIMIZERFLAGWIDGET_H