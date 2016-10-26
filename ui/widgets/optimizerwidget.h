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
 */

#ifndef OPTIMIZERWIDGET_H
#define OPTIMIZERWIDGET_H
#include <QtWidgets/QWidget>
#include <QtWidgets/QDoubleSpinBox>
#include <QLocale>
#include "core/data/modelclass.h"

struct OptimizerConfig;
class QValidator;
class QSpinBox;
class QCheckBox;
class QTabWidget;

class ScientificBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    QString textFromValue(double val) const
    {
        return QString::number(val, 'E', decimals());
    }

    double valueFromText(const QString &text) const
    {
        QString value = text;
        value.replace(loc.decimalPoint(), '.');
        return value.toDouble();
    }

    QValidator::State validate(QString &input, int &pos) const
    {
        QDoubleValidator validator;
        validator.setBottom(minimum());
        validator.setTop(maximum());
        validator.setDecimals(decimals());
        validator.setNotation(QDoubleValidator::ScientificNotation);
        input.replace('.', loc.decimalPoint());
        return validator.validate(input, pos);
    }

private:
    QLocale loc;
};

class OptimizerWidget : public QWidget
{
    Q_OBJECT
public:
    OptimizerWidget(OptimizerConfig config, QWidget *parent = 0);
    
    ~OptimizerWidget();
     OptimizerConfig Config() const;
private:
    OptimizerConfig m_config;
    QTabWidget *m_tabwidget;
    QSpinBox *m_maxiter, *m_levmar_constants_periter, *m_levmar_shifts_periter, *m_sum_convergence;
    ScientificBox *m_levmarmu, *m_levmar_eps1, *m_levmar_eps2, *m_levmar_eps3, *m_levmar_delta, *m_shift_convergence, *m_constant_convergence, *m_error_convergence;
    QCheckBox *m_optimize_shifts;
    
    void setUi();
    void setGeneral();
    void setLevMar();
};

#endif // OPTIMIZERWIDGET_H
