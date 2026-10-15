/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
    m_message->setLineWrapMode(QTextEdit::WidgetWidth);
    m_message->setFrameShape(QFrame::NoFrame);

    m_clear = new QPushButton(tr("Clear"));
    m_clear->setIcon(Icon("edit-clear-history"));
    m_clear->setToolTip(tr("Remove all entries from the log"));
    m_clear->setMaximumWidth(100);
    m_clear->setFlat(true);
    connect(m_clear, &QPushButton::clicked, m_message, &QTextEdit::clear);

    QLabel* label = new QLabel(tr("<b>Log</b>"));

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->setContentsMargins(6, 2, 2, 2);
    hlayout->addWidget(label);
    hlayout->addStretch();
    hlayout->addWidget(m_clear);
    m_titlebarwidget = new QWidget;
    m_titlebarwidget->setLayout(hlayout);

    m_trayicon = new QSystemTrayIcon(QIcon(":/misc/SupraFit.png"), this);

    QGridLayout* layout = new QGridLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_message, 0, 0);
    setLayout(layout);
    m_message->append("<html>");
}

MessageDock::~MessageDock()
{
}

/*! \brief Append one log line: time, then the message in the level colour. The colours are chosen
 * for the current base colour so that they stay readable in light and dark mode; only the time is
 * shown to keep lines short in the narrow left column. Messages may contain rich text.
 * Claude Generated. */
void MessageDock::Append(const QString& str, Level level)
{
    const bool dark = m_message->palette().color(QPalette::Base).lightness() < 128;
    QString colour;
    switch (level) {
    case Level::Message:
        colour = dark ? "#7fd17f" : "#1e7d32";
        break;
    case Level::Warning:
        colour = dark ? "#ff8a80" : "#c62828";
        break;
    case Level::Info:
        colour = dark ? "#82b1ff" : "#1565c0";
        break;
    }
    const QColor timeColour = m_message->palette().color(QPalette::PlaceholderText);
    m_message->append(QString("<p style='margin:0'><span style='color:%1'>%2</span>&nbsp; <span style='color:%3'>%4</span></p>")
                          .arg(timeColour.name(), QDateTime::currentDateTime().toString("hh:mm:ss"), colour, str));
}

void MessageDock::Message(const QString& str)
{
    QMutexLocker locker(&m_mutex);
    Append(str, Level::Message);
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
    Append(str, Level::Warning);
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
    Append(str, Level::Info);
    if (QGuiApplication::applicationState() == Qt::ApplicationInactive) {
        m_trayicon->show();
        m_trayicon->showMessage("SupraFit - Info", str, QIcon(":/misc/SupraFit.png"), 2500);
        QTimer::singleShot(5000, m_trayicon, &QSystemTrayIcon::hide);
    }

    emit UiInfo();
}
