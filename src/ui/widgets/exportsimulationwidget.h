/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/guitools/mime.h"

#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtCore/QTimer>

#include <QtGui/QDrag>
#include <QtGui/QMouseEvent>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

class AbstractModel;

class DnDLabel : public QLabel {
    Q_OBJECT

public:
    DnDLabel(const QString& str, int type, QWeakPointer<AbstractModel> model)
        : m_type(type)
        , m_model(model)
    {
        setText(str);
    }

    inline void setStd(double stdev)
    {
        m_stdev = stdev;
    }

    void setExportAll(int export_all) { m_export_all = export_all; }

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void UpdateContent();
    int m_type = 0;
    double m_stdev = 0;
    int m_export_all = 1;

    QByteArray m_content;
    QWeakPointer<AbstractModel> m_model;
};

class ClickLabel : public QLabel {
    Q_OBJECT

public:
    void Clicked()
    {
        QString txt = text();
        setText("<font color='red'>Copied to Clipboard ...</font>");
        QTimer::singleShot(1800, this, [this, txt]() { setText(txt); });
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        emit MouseClicked();
        QLabel::mousePressEvent(event);
    }

signals:
    void MouseClicked();
};

class ExportSimulationWidget : public QWidget {
    Q_OBJECT
public:
    explicit ExportSimulationWidget(QWeakPointer<AbstractModel> model, QWidget* parent = nullptr);
    /*! \brief SSE, sigma and SEy read-outs (click copies the value); placed with the fit results by
     * the model widget. Owned by this widget until reparented. Claude Generated. */
    QWidget* MetricsWidget() const { return m_metrics; }

signals:
    void AdvancedChanged(bool advanced); ///< the setting advanced_ui changed (the page may show a hint)

private:
    void UpdateVisibility();

    QWeakPointer<AbstractModel> m_model;

    ClickLabel *m_sse, *m_std, *m_sey;
    QWidget* m_metrics;
    DnDLabel *m_ideal, *m_mc_std, *m_mc_sey, *m_mc_user, *m_bs;
    QDoubleSpinBox* m_variance;
    QCheckBox* m_include_deactivated;
};
