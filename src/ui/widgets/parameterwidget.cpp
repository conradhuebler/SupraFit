/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/models.h"
#include "src/global.h"

#include "src/ui/widgets/buttons/spinbox.h"

#include <QtCore/QPointer>
#include <QtCore/QRandomGenerator>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

#include "parameterwidget.h"

LocalParameterWidget::LocalParameterWidget(QSharedPointer<AbstractModel> model)
    : m_model(model)
{
    int columns = qApp->instance()->property("ModelParameterColums").toInt();
    QGridLayout* layout = new QGridLayout;
    for (int i = 0; i < m_model.toStrongRef().data()->LocalParameterSize(); i++) {
        QWidget* widget = new QWidget;
        QHBoxLayout* hlayout = new QHBoxLayout;
        widget->setLayout(hlayout);

        QPointer<QCheckBox> check = new QCheckBox;
        check->setChecked(true);
        connect(this, &LocalParameterWidget::LocalCheckState, check, &QCheckBox::setChecked);

        QPointer<SpinBox> box = new SpinBox;
        box->setMinimum(-1e10);
        box->setMaximum(1e15);
        box->setDecimals(5);
        box->setValue(m_model.toStrongRef().data()->LocalParameter(i, 0));

        connect(m_model.toStrongRef().data(), &AbstractModel::Recalculated, box,
            [i, box, check, this, widget]() {
                if (this->m_model && check) {
                    if (!m_model.toStrongRef().data()->isSimulation())
                        if (!qFuzzyCompare(box->value(), m_model.toStrongRef().data()->LocalParameter(i, 0))) {
                            box->setValue(m_model.toStrongRef().data()->LocalParameter(i, 0));
                        }
                    if (this->m_model.toStrongRef().data()->LocalEnabled(i)) {
                        box->setStyleSheet("background-color: " + included());
                        check->setEnabled(true);
                        check->setChecked(m_model.toStrongRef().data()->LocalTable()->isChecked(0, i));
                    } else {
                        box->setStyleSheet("background-color: " + excluded());
                        check->setEnabled(false);
                    }
                }
            });
        connect(box, &SpinBox::valueChangedNotBySet, box,
            [i, box, this]() {
                if (this->m_model) {
                    m_model.toStrongRef().data()->forceLocalParameter(box->value(), i, 0);
                    m_model.toStrongRef().data()->Calculate();
                }

            });

        connect(check, &QCheckBox::stateChanged, check, [i, this](int state) {
            if (this->m_model) {
                m_model.toStrongRef().data()->LocalTable()->setChecked(0, i, state);
            }

        });
        hlayout->addWidget(new QLabel(m_model.toStrongRef().data()->LocalParameterName(i)));
        hlayout->addWidget(box);
        m_parameter << box;
        hlayout->addWidget(check);
        check->setHidden(m_model.toStrongRef().data()->isSimulation());
        layout->addWidget(widget, i / columns, i % columns);
    }

    setLayout(layout);
}

void LocalParameterWidget::setReadOnly(bool readonly)
{
    for (int i = 0; i < m_parameter.size(); ++i)
        m_parameter[i]->setReadOnly(readonly);
}
