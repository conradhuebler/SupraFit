/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models.h"

class QPushButton;

class MetaModelWidget : public QWidget {
    Q_OBJECT
public:
    MetaModelWidget(QWidget* parent = 0);
    inline void setMetaModel(QSharedPointer<AbstractModel> model) { m_model = model; }

signals:

public slots:

private slots:
    void Minimize();

private:
    void setUi();

    QSharedPointer<AbstractModel> m_model;
    QPushButton* m_minimize;
};
