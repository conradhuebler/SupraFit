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

#include <QtCore/QDateTime>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

#include "src/global.h"
#include "src/version.h"

#include "src/ui/guitools/instance.h"

#include "messagedock.h"

MessageDock::MessageDock()
{
    m_message = new QTextEdit;
    m_clear = new QPushButton(tr("Clear"));
    m_clear->setIcon(Icon("edit-clear-history"));
    connect(m_clear, &QPushButton::clicked, m_message, &QTextEdit::clear);

    QLabel* label = new QLabel(tr("Message from SupraFit"));

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(label);
    hlayout->addWidget(m_clear);
    m_titlebarwidget = new QWidget;
    m_titlebarwidget->setLayout(hlayout);

    QGridLayout* layout = new QGridLayout;
    layout->addWidget(m_message, 0, 0);
    setLayout(layout);
    m_message->append("<html>");
}

MessageDock::~MessageDock()
{
}

void MessageDock::Message(const QString& str)
{
    QMutexLocker locker(&m_mutex);
    m_message->append(tr("<pre><p>%1&emsp;<font color='green'> %2 </font></p></pre>").arg(QDateTime::currentDateTime().toString()).arg(str));
}

void MessageDock::Warning(const QString& str)
{
    QMutexLocker locker(&m_mutex);
    m_message->append(tr("<pre><p>%1&emsp;<font color='red'> %2 </font></p></pre>").arg(QDateTime::currentDateTime().toString()).arg(str));
}
