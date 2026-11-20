/*
 * ConcentrationEditor - the concentration table of the spectra and NMR import dialogs
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
 * Claude Generated (UX package 2a of the import dialogs).
 */

#pragma once

#include <QtWidgets/QWidget>

class DataTable;
class DropTable;
class QLabel;
class QPushButton;
class StatusLine;

/*! \brief Concentrations of a titration series, one row per spectrum (Claude Generated).
 *
 * A DropTable (Ctrl+V replaces, Ctrl+B appends, Ctrl+Y prepends columns; right-click "Clear Table") with a
 * title, the paste hint, "Load from file" (a table file read as "Open Project" reads one: separator
 * detected, decimal comma accepted, a header row kept as column names) and a line "n rows for m
 * spectra", a warning when the numbers differ. Edited() follows every change (paste, load, clear,
 * setTable). The host reads dataTable(), rows() and columns() as it read the DropTable before. */
class ConcentrationEditor : public QWidget {
    Q_OBJECT
public:
    explicit ConcentrationEditor(const QString& title, QWidget* parent = nullptr);

    inline DropTable* table() const { return m_table; }
    /*! \brief The table shown (nullptr without one). */
    DataTable* dataTable() const;
    /*! \brief Rows of a valid table with at least one column (0 otherwise). */
    int rows() const;
    /*! \brief Columns of a valid table with at least one row (0 otherwise). */
    int columns() const;

    /*! \brief Shows a copy of table (nullptr: empty); emits Edited(). */
    void setTable(const DataTable* table);
    /*! \brief Reads a table file; false with *error (the table stays) if it holds no numbers. */
    bool loadFile(const QString& file, QString* error = nullptr);

    /*! \brief Number of spectra the rows are checked against; used >= 0: a host that also takes one row
     * per used spectrum (NMR with spectra left out). */
    void setSpectraCount(int count, int used = -1);
    inline StatusLine* statusLine() const { return m_status; }

signals:
    void Edited();

private:
    void updateStatus();
    /*! \brief Takes table (parent = the view) into the view; the old own table is deleted. */
    void adopt(DataTable* table);

    DropTable* m_table;
    QLabel* m_title;
    QLabel* m_hint;
    QPushButton* m_load;
    StatusLine* m_status;
    int m_spectra = 0;
    int m_used = -1;
};
