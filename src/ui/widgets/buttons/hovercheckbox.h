/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifndef HOVERCHECK_H
#define HOVERCHECK_H

#include <QtCore/QEvent>

#include <QtWidgets/QCheckBox>

class HoverCheckBox : public QCheckBox
{
    Q_OBJECT
    
protected:

    inline bool event(QEvent * e)
    {
        switch(e->type())
        {
            case QEvent::HoverEnter:
                emit hovered();
                return true;
                break;
            
            default:
                return QWidget::event(e);
                break;
        }
        return QWidget::event(e);
    }
    
signals:
    void hovered();
};

#endif
