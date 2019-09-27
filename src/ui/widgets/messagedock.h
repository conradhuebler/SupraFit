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

#pragma once

#include <QtCore/QMutex>
#include <QtCore/QSharedPointer>

#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

#include "src/ui/guitools/instance.h"

class MessageDock : public QWidget {
    Q_OBJECT
public:
    MessageDock();
    virtual ~MessageDock();
    QWidget* TitleBarWidget() { return m_titlebarwidget; }

public slots:
    void Message(const QString& str);
    void Warning(const QString& str);
    void Info(const QString& str);

private:
    QTextEdit* m_message;
    QPushButton* m_clear;
    QMutex m_mutex;
    QWidget* m_titlebarwidget;

private slots:

signals:
    void Attention();
    void Presence();
    void UiInfo();
};
