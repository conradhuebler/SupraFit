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
#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

#include "qtimer.h"
#include "src/global.h"
#include "src/version.h"

#include "src/ui/guitools/guitools.h"

#include "messagedock.h"

MessageDock::MessageDock()
{
    m_message = new QTextEdit;

    m_message->setReadOnly(true);
    QPalette p = m_message->palette();

    p.setColor(QPalette::Active, QPalette::Base, Qt::lightGray);
    p.setColor(QPalette::Inactive, QPalette::Base, Qt::lightGray);
    p.setColor(QPalette::Inactive, QPalette::Text, Qt::black);
    p.setColor(QPalette::Inactive, QPalette::Window, Qt::black);
    m_message->setPalette(p);

    m_clear = new QPushButton(tr("Clear"));
    m_clear->setIcon(Icon("edit-clear-history"));
    m_clear->setMaximumWidth(100);
    m_clear->setFlat(true);
    connect(m_clear, &QPushButton::clicked, m_message, &QTextEdit::clear);

    QLabel* label = new QLabel(tr("Message from SupraFit"));

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(label);
    //hlayout->add();
    hlayout->addWidget(m_clear);
    m_titlebarwidget = new QWidget;
    m_titlebarwidget->setLayout(hlayout);

    m_trayicon = new QSystemTrayIcon(QIcon(":/misc/SupraFit.png"), this);

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
    if (QGuiApplication::applicationState() == Qt::ApplicationInactive) {
        m_trayicon->show();
        m_trayicon->showMessage("SupraFit - Message", str, QIcon(":/misc/SupraFit.png"), 2500);
        QTimer::singleShot(5000, m_trayicon, &QSystemTrayIcon::hide);
    }
    emit Presence();
}

void MessageDock::Warning(const QString& str)
{
    QMutexLocker locker(&m_mutex);
    m_message->append(tr("<pre><p>%1&emsp;<font color='red'> %2 </font></p></pre>").arg(QDateTime::currentDateTime().toString()).arg(str));
    if (QGuiApplication::applicationState() == Qt::ApplicationInactive) {
        m_trayicon->show();
        m_trayicon->showMessage("SupraFit - Warning", str, QIcon(":/misc/SupraFit.png"), 2500);
        QTimer::singleShot(5000, m_trayicon, &QSystemTrayIcon::hide);
    }
    emit Attention();
}

void MessageDock::Info(const QString& str)
{
    QMutexLocker locker(&m_mutex);
    m_message->append(tr("<pre><p>%1&emsp;<font color='blue'> %2 </font></p></pre>").arg(QDateTime::currentDateTime().toString()).arg(str));
    if (QGuiApplication::applicationState() == Qt::ApplicationInactive) {
        m_trayicon->show();
        m_trayicon->showMessage("SupraFit - Info", str, QIcon(":/misc/SupraFit.png"), 2500);
        QTimer::singleShot(5000, m_trayicon, &QSystemTrayIcon::hide);
    }

    emit UiInfo();
}
