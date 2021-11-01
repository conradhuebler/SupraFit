/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/widgets/textwidget.h"

class AbstractModel;
class QLabel;
class QPushButton;
class TextWidget;
struct StatisticResult;

class StatisticWidget : public QWidget {
    Q_OBJECT
public:
    StatisticWidget(const QSharedPointer<AbstractModel> model, QWidget* parent);
    ~StatisticWidget();
    inline QString Overview() const { return m_short; }
    inline QString Statistic() const { return m_statistics; }
    inline QString Text() const { return m_overview->toPlainText(); }
public slots:
    void Update();

private:
    QWeakPointer<AbstractModel> m_model;
    QPushButton* m_show;
    QWidget* m_subwidget;
    TextWidget* m_overview;
    QString m_short, m_statistics;

private slots:
    void toggleView();
};
