/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QPointer>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QTextEdit>

class QPushButton;
class QTextEdit;

class CompareDialog : public QDialog {
    Q_OBJECT
public:
    CompareDialog(QWidget* parent);
    void setCutoff(qreal cutoff);
    inline qreal CutOff() const { return m_cutoff_box->value(); }
    inline bool Local() const { return m_local->isChecked(); }
    inline void SetComparison(const QString& text) { m_overview->setHtml(text); }

    int CVType() const { return m_cvtype; }

private:
    void setUi();

    QPushButton *m_reduction, *m_aic, *m_hide, *m_crossvalidation;
    QDoubleSpinBox* m_cutoff_box;
    QCheckBox* m_local;
    QRadioButton *m_cv_loo, *m_cv_l2o;
    qreal m_cutoff;
    int m_cvtype = 1;

    QTextEdit* m_overview;

signals:
    void CompareReduction();
    void CompareAIC();
    void CompareCV();
};
