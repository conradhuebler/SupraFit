/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QDebug>

#include <QtGui/QTextCursor>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QTextBrowser>

#include "src/global.h"

#include "textwidget.h"

TextWidget::TextWidget()
{
    m_save_action = new QAction("Save selection");
    connect(m_save_action, &QAction::triggered, this, &TextWidget::SaveSelection);
    m_menu = createStandardContextMenu();
    m_menu->addAction(m_save_action);

    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &TextWidget::customContextMenuRequested, this, &TextWidget::customMenuRequested);
}

void TextWidget::SaveSelection()
{

    QString filename = QFileDialog::getSaveFileName(this, "Select file", getDir(), ("Table Files (*.dat)"));
    if (filename.isEmpty())
        return;

    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
        return;

    QTextStream stream(&file);

    stream << textCursor().selectedText();
}

void TextWidget::customMenuRequested(QPoint pos)
{
    m_menu->popup(viewport()->mapToGlobal(pos));
}
