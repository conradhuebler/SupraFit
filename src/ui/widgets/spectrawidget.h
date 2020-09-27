/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <charts.h>

#include <QtCore/QJsonObject>

#include <src/core/spectrahandler.h>

class DropTable;

class QLineEdit;
class QListWidget;
class QTableWidget;
class QSplitter;
class QPushButton;
class QTabWidget;

class SpectraWidget : public QWidget {
    Q_OBJECT
public:
    explicit SpectraWidget(QWidget* parent = nullptr);

    void setDirectory(const QString& directry);
    void addFile(const QString& file);

    void setUI();
    QJsonObject ProjectData() const { return m_project; }
    QJsonObject InputTable() const { return m_input_table; }
    void setData(const QJsonObject& data);

public slots:
    void UpdateSpectra();
    void UpdateData();

signals:

private:
    QTabWidget* m_views;
    QListWidget *m_files, *m_xvalues;
    ChartView* m_spectra_view;
    DropTable *m_indep, *m_datatable;
    QSplitter *m_main_splitter, *m_list_splitter;
    QLineEdit* m_add_xvalue;
    QPushButton* m_accept_x;
    SpectraHandler* m_handler;
    QJsonObject m_project, m_input_table;
private slots:
    void PointDoubleClicked(const QPointF& point);
};
