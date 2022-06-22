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

#include <QtCore/QJsonObject>
#include <QtCore/QPair>
#include <QtCore/QVariant>

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

#include "src/ui/guitools/flowlayout.h"

#include "preparewidget.h"

PrepareBox::PrepareBox(const QJsonObject& object, QWidget* parent)
    : QGroupBox(parent)
{
    QHBoxLayout* layout = new QHBoxLayout;

    QLabel* description = new QLabel;
    description->setWordWrap(true);
    description->setText(object["description"].toString());
    layout->addWidget(description);

    setTitle(object["title"].toString());
    m_type = object["type"].toInt();
    if (m_type == 1) {
        m_spinbox = new QSpinBox;
        m_spinbox->setValue(object["default"].toInt());
        m_element = QPair<QString, QJsonValue>(object["name"].toString(), QJsonValue(m_spinbox->value()));
        layout->addWidget(m_spinbox);
        connect(m_spinbox, &QSpinBox::valueChanged, this, [this, object]() {
            m_element = QPair<QString, QJsonValue>(object["name"].toString(), QJsonValue(m_spinbox->value()));
        });
    } else if (m_type == 2) {
        m_doublespinbox = new QDoubleSpinBox;
        m_doublespinbox->setValue(object["default"].toDouble());
        m_element = QPair<QString, QJsonValue>(object["name"].toString(), QJsonValue(m_doublespinbox->value()));
        layout->addWidget(m_doublespinbox);
        connect(m_doublespinbox, &QDoubleSpinBox::valueChanged, this, [this, object]() {
            m_element = QPair<QString, QJsonValue>(object["name"].toString(), QJsonValue(m_doublespinbox->value()));
        });

    } else if (m_type == 3) {
        m_lineedit = new QLineEdit;
        m_lineedit->setText(object["default"].toString());
        layout->addWidget(m_lineedit);
        m_element = QPair<QString, QJsonValue>(object["name"].toString(), QJsonValue(m_lineedit->text()));
        connect(m_lineedit, &QLineEdit::textChanged, this, [this, object]() {
            m_element = QPair<QString, QJsonValue>(object["name"].toString(), QJsonValue(m_lineedit->text()));
        });
    }
    setLayout(layout);
    setMaximumWidth(350);
    setMinimumWidth(350);
    setMinimumHeight(80);
}

PrepareWidget::PrepareWidget(const QVector<QJsonObject>& objects, QWidget* parent)
    : QWidget{ parent }
{
    FlowLayout* layout = new FlowLayout;

    for (const QJsonObject& object : qAsConst(objects)) {
        PrepareBox* box = new PrepareBox(object, this);
        layout->addWidget(box);
        m_stored_objects << box;
    }
    setLayout(layout);
}

QVector<QPair<QString, QJsonValue>> PrepareWidget::getObject() const
{
    QVector<QPair<QString, QJsonValue>> objects;
    for (const auto i : m_stored_objects)
        objects << i->getElement();
    return objects;
}
