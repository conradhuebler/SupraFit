/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/AbstractModel.h"

#include <QtGui/QPainter>
#include <QtGui/QTextDocument>

#include <QtWidget/QStyledItemDelegate>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLayout>
#include <QtWidgets/QLineEdit>

class HTMLDelegate : public QStyledItemDelegate {
protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyleOptionViewItemV4 options = option;
        initStyleOption(&options, index);

        painter->save();

        QTextDocument doc;
        doc.setHtml(options.text);

        options.text = "";
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);
        options.widget->style()->drawControl(QStyle::CE_ComboBoxLabel, &options, painter);

        painter->translate(options.rect.left(), options.rect.top());
        QRect clip(0, 0, options.rect.width(), options.rect.height());
        doc.drawContents(painter, clip);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyleOptionViewItemV4 options = option;
        initStyleOption(&options, index);

        QTextDocument doc;
        doc.setHtml(options.text);
        doc.setTextWidth(options.rect.width());
        return QSize(doc.idealWidth(), doc.size().height());
    }
};

class SystemParameterWidget : public QGroupBox {
    Q_OBJECT

public:
    SystemParameterWidget(const SystemParameter& parameter, bool readonly, QWidget* parent = 0);
    ~SystemParameterWidget();

    SystemParameter Value();
    void setValue(const SystemParameter& variant);

private:
    QLineEdit* m_textfield;
    QCheckBox* m_boolbox;
    QComboBox* m_list;
    SystemParameter m_parameter;
    bool m_change, m_readonly;

private slots:
    void PrepareChanged();

signals:
    void valueChanged();
};

class SPOverview : public QWidget {
    Q_OBJECT
public:
    inline SPOverview(DataClass* data, bool readonly)
        : m_data(data)
        , m_readonly(readonly)
    {
        QVBoxLayout* layout = new QVBoxLayout;
        QHBoxLayout* hlayout = new QHBoxLayout;
        layout->setAlignment(Qt::AlignTop);
        int counter = 1;
        for (int index : m_data->getSystemParameterList()) {
            QPointer<SystemParameterWidget> widget = new SystemParameterWidget(m_data->getSystemParameter(index), m_readonly, this);
            hlayout->addWidget(widget);
            connect(widget, &SystemParameterWidget::valueChanged,
                [index, widget, this]() {
                    if (widget) {
                        m_data->setSystemParameter(widget->Value());
                        m_data->WriteSystemParameter();
                    }
                });

            connect(m_data, &DataClass::SystemParameterChanged,
                [index, widget, this]() {
                    if (widget) {
                        widget->setValue(m_data->getSystemParameter(index));
                    }
                });
            if (counter % 2 == 0) {
                layout->addLayout(hlayout);
                hlayout = new QHBoxLayout;
            }
            counter++;
        }
        layout->addLayout(hlayout);
        setLayout(layout);
    }

private:
    DataClass* m_data;
    bool m_readonly;
};
