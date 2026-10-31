/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include <QtCore/QSharedPointer>

#include <QtWidgets/QDialog>

class AbstractModel;
class ScanWidget;

/*! \brief Modal dialog around the Scan (`ScanWidget`) for the places that have no Statistics page (the meta model). Claude Generated. */
class AdvancedSearch : public QDialog {
    Q_OBJECT

public:
    AdvancedSearch(QWidget* parent = nullptr);
    void setModel(const QSharedPointer<AbstractModel> model);

signals:
    void RunCalculation(const QJsonObject& controller);
    void Interrupt();

private:
    ScanWidget* m_scan = nullptr;
};
