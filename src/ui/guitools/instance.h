/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#pragma once

#include <QtCore/QObject>

#include <QtGui/QIcon>

#include <QtWidgets/QApplication>

template <class T>
class InstancePointer {
public:
    static T* asPtr(QVariant v)
    {
        return (T*)v.value<void*>();
    }
    static QVariant asQVariant(T* ptr)
    {
        return qVariantFromValue((void*)ptr);
    }
};

class Instance : public QObject {
    Q_OBJECT

public:
    inline static void setInstance(QPointer<Instance> instance) { qApp->instance()->setProperty("InstanceHolder", InstancePointer<Instance>::asQVariant(instance)); }
    inline static Instance* GlobalInstance() { return InstancePointer<Instance>::asPtr(qApp->instance()->property("InstanceHolder")); }

signals:
    void ConfigurationChanged();
};

inline QIcon Icon(const QString& str)
{
#ifdef _Theme
    return QIcon::fromTheme(str);
#else
    return QIcon(":/icons/" + str + ".png");
#endif
}
