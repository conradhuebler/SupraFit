/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QPair>
#include <QtCore/QVariant>

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QWidget>

class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;

class PrepareBox : public QGroupBox {
    Q_OBJECT
public:
    explicit PrepareBox(const QJsonObject& object, QWidget* parent = NULL);

    inline QPair<QString, QJsonValue> getElement() const { return m_element; }

private:
    QLineEdit* m_lineedit;
    QSpinBox* m_spinbox;
    QDoubleSpinBox* m_doublespinbox;

    QPair<QString, QJsonValue> m_element;

    int m_type;
};

class PrepareWidget : public QWidget {
    Q_OBJECT
public:
    explicit PrepareWidget(const QVector<QJsonObject>& objects, QWidget* parent = nullptr);

    QVector<QPair<QString, QJsonValue>> getObject() const;
signals:

private:
    QVector<PrepareBox*> m_stored_objects;
};
