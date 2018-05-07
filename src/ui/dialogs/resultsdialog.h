/*
 * <one line to give the library's name and an idea of what it does.>
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

#include <QtWidgets/QDialog>

#include "src/global.h"

class QListWidget;
class QListWidgetItem;
class QSplitter;
class QTabWidget;

class AbstractModel;
class ChartWrapper;

class ResultsDialog : public QDialog {
    Q_OBJECT
public:
    ResultsDialog(QSharedPointer<AbstractModel> model, ChartWrapper* wrapper, QWidget* parent);

public slots:
    void ShowResult(SupraFit::Statistic type);
    void Attention();

private:
    QWeakPointer<AbstractModel> m_model;
    QSplitter* m_mainwidget;
    QListWidget* m_results;
    QTabWidget* m_tabs;
    QGridLayout* m_layout;
    ChartWrapper* m_wrapper;

private slots:
    void UpdateList();
    void itemDoubleClicked(QListWidgetItem* item);
};
