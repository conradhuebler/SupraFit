/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include "src/core/models/dataclass.h"

#include "src/core/filehandler.h"

#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>

#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QTableView>

class DataTable;

class DropTable : public QTableView {
    Q_OBJECT

public:
    inline DropTable()
        : QTableView()
    {
        setContextMenuPolicy(Qt::ActionsContextMenu);
        QAction* clear = new QAction(tr("Clear Table"));
        connect(clear, &QAction::triggered, this, &DropTable::clear);
        addAction(clear);
    }
    inline virtual ~DropTable() {}

protected:
    inline virtual void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_C && event->modifiers() & Qt::ControlModifier) {
            QApplication::clipboard()->setText(this->currentIndex().data().toString());
        } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_V) {
            QString paste = QApplication::clipboard()->text();
            FileHandler* handler = new FileHandler(this);
            if (!handler->setFileContent(paste))
                return;
            DataTable* model = handler->getData();
            if (model->isValid()) {
                setModel(model);
                m_table = model;
                emit Edited();
            }
            delete handler;
        } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_B) {
            QString paste = QApplication::clipboard()->text();
            FileHandler* handler = new FileHandler(this);
            if (!handler->setFileContent(paste))
                return;
            DataTable* model = handler->getData();
            if (!m_table) {
                if (model->isValid()) {
                    setModel(model);
                    m_table = model;
                    emit Edited();
                }
            } else {
                if (model->isValid()) {
                    m_table->appendColumns(model);
                    setModel(m_table);
                    emit Edited();
                }
            }
            delete handler;
        } else {

            QTableView::keyPressEvent(event);
        }
    }

signals:
    void Edited();

private:
    inline void clear()
    {
        QPointer<DataTable> table = new DataTable(this);
        setModel(table);
        m_table = table;
        m_table->layoutChanged();
    }
    void deleteRow();
    void deleteColumn();

    QPointer<DataTable> m_table;
};
