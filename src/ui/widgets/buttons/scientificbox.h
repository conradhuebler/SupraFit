/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * ScientificBox thanks to:
 * https://bugreports.qt.io/browse/QTBUG-7521
 */

#ifndef SCIENTIFICBOX_H
#define SCIENTIFICBOX_H

#include <QtWidgets/QDoubleSpinBox>

class ScientificBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    inline QString textFromValue(double val) const
    {
        return QString::number(val, 'E', 2);
    }

    inline double valueFromText(const QString &text) const
    {
        QString value = text;
        value.replace(loc.decimalPoint(), '.');
        return value.toDouble();
    }

    inline QValidator::State validate(QString &input, int &pos) const
    {
        QDoubleValidator validator;
        validator.setBottom(minimum());
        validator.setTop(maximum());
        validator.setDecimals(2);
        validator.setNotation(QDoubleValidator::ScientificNotation);
        input.replace('.', loc.decimalPoint());
        return validator.validate(input, pos);
    }

private:
    QLocale loc;
};

#endif
