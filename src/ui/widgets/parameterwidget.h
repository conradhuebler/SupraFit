/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QSharedPointer>

#include <QtWidgets/QWidget>

class QCheckBox;

class AbstractModel;
class SpinBox;

class LocalParameterWidget : public QWidget {
    Q_OBJECT
public:
    LocalParameterWidget(QSharedPointer<AbstractModel> model);

signals:

public slots:

private:
    QSharedPointer<AbstractModel> m_model;
    QList<QPointer<SpinBox>> m_parameter;
    QList<QPointer<QCheckBox>> m_locked;
};
