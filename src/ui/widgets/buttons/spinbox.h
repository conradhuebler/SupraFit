/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QSignalBlocker>
#include <QtCore/QTimer>

#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLineEdit>

class LineEdit : public QLineEdit {
    Q_OBJECT
public:
    LineEdit()
    {
        m_block = new QSignalBlocker(this);
        m_block->unblock();
    }
    virtual ~LineEdit()
    {
        delete m_block;
    }

    inline virtual void mousePressEvent(QMouseEvent* event) override
    {
        m_block->reblock();
        QWidget::mousePressEvent(event);
        QTimer::singleShot(3000, this, [this]() {
            m_block->unblock();
        });
    }

private:
    QSignalBlocker* m_block;
};

class SpinBox : public QDoubleSpinBox {
    Q_OBJECT

public:
    inline SpinBox(QWidget* parent = 0)
        : QDoubleSpinBox(parent)
        , valueBeingSet(false)
    {
        setLineEdit(new LineEdit());
        connect(this, SIGNAL(valueChanged(double)), this, SLOT(On_valueChanged(double)));
        setMaximum(1e27);
        setDecimals(4);
    }
    inline ~SpinBox() {}

    inline QString textFromValue(double val) const override
    {
        QString value;
        if (val > 1e3 || val < -1e3)
            value = QString::number(val, 'E', 3);
        else if ((val < 1e-2 && val > -1e-2) && abs(val) > 1e-8)
            value = QString::number(val, 'E', 2);
        else
            value = QString::number(val, 'f', 4);
        value.replace('.', loc.decimalPoint());
        value.replace(',', loc.decimalPoint());
        return value;
    }
    /*

    */
    /*
    inline QValidator::State validate(QString& input, int& pos) const override
    {
        QDoubleValidator validator;
        validator.setBottom(minimum());
        validator.setTop(maximum());
        validator.setDecimals(4);
        validator.setNotation(QDoubleValidator::ScientificNotation);
        input.replace('.', loc.decimalPoint());
        return validator.validate(input, pos);
    }
    */
protected:
    bool valueBeingSet;

public slots:
    inline void setValue(double val)
    {
        valueBeingSet = true;
        QDoubleSpinBox::setValue(val);
        setSingleStep(val / 100.0 + 0.001);
        valueBeingSet = false;
    }

private:
    QLocale loc;

private slots:
    inline void On_valueChanged(double val)
    {
        setSingleStep(val / 100.0);
        if (!valueBeingSet)
            emit valueChangedNotBySet(val);
    }

signals:
    void valueChangedNotBySet(double val);
};
