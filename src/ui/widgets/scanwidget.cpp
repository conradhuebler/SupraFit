/*
 * SupraFit - the Scan: a grid of start values for the global search
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * Claude Generated (roadmap/statistic_dialogs.md, WP-S3).
 */

#include "scanwidget.h"

#include "analysisparts.h"

#include "src/core/models/AbstractModel.h"
#include "src/core/toolset.h"
#include "src/global.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

#include <cmath>

void ScanWidget::DefaultRange(const QString& name, double value, double& min, double& max, double& step)
{
    if (name.startsWith(QLatin1String("lg"), Qt::CaseInsensitive) || name.contains(QLatin1String("lg "), Qt::CaseInsensitive)) {
        min = value - 3;
        max = value + 3;
        step = (max - min) / 10;
        return;
    }
    const double div = value / 5.0, prod = value * 5;
    min = ToolSet::floor(qMin(div, prod));
    max = ToolSet::ceil(qMax(div, prod));
    step = ToolSet::ceil((max - min) / 10);
}

QWidget* ScanWidget::AddRow(const QString& name, double value, bool global, int index, int series, bool varied)
{
    Row row;
    row.global = global;
    row.index = index;
    row.series = series;
    row.value = value;
    row.plainName = name;

    double min, max, step;
    DefaultRange(name, value, min, max, step);
    auto spin = [](double v, double lo) {
        QDoubleSpinBox* s = new QDoubleSpinBox;
        s->setRange(lo, 1e8);
        s->setValue(v);
        return s;
    };
    row.min = spin(min, -1e8);
    row.max = spin(max, -1e8);
    row.step = spin(step, 0.01);

    row.vary = new QCheckBox;
    row.vary->setChecked(varied);
    row.vary->setToolTip(tr("Checked: the parameter is varied from start to end. Unchecked: its current value is taken."));
    row.fit = new QCheckBox(tr("Fit"));
    row.fit->setChecked(true);
    row.fit->setToolTip(tr("Checked: the parameter is optimised in every fit. Unchecked: it is fixed in the fit, but still varied by the scan."));
    row.name = new QLabel;

    QFrame* frame = new QFrame;
    frame->setFrameShape(QFrame::StyledPanel);
    QGridLayout* grid = new QGridLayout;
    grid->addWidget(row.vary, 0, 0);
    grid->addWidget(row.name, 0, 1, 1, 4);
    grid->addWidget(row.fit, 0, 5);
    grid->addWidget(new QLabel(tr("Start:")), 1, 0);
    grid->addWidget(row.min, 1, 1);
    grid->addWidget(new QLabel(tr("End:")), 1, 2);
    grid->addWidget(row.max, 1, 3);
    grid->addWidget(new QLabel(tr("Step:")), 1, 4);
    grid->addWidget(row.step, 1, 5);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(3, 1);
    grid->setColumnStretch(5, 1);
    frame->setLayout(grid);

    const int at = m_rows.size();
    auto setLabel = [this, at](double v) {
        m_rows[at].value = v;
        m_rows[at].name->setText(tr("<html>%1 (%2)</html>").arg(m_rows[at].plainName).arg(v));
    };
    m_rows << row;
    setLabel(value);

    auto enableRange = [this, at]() {
        const bool on = m_rows[at].vary->isChecked() && m_rows[at].vary->isEnabled();
        m_rows[at].min->setEnabled(on);
        m_rows[at].max->setEnabled(on);
        m_rows[at].step->setEnabled(on);
        emit CalculationsChanged(Calculations());
    };
    connect(row.vary, &QCheckBox::toggled, this, enableRange);
    for (QDoubleSpinBox* s : { row.min, row.max, row.step })
        connect(s, &QDoubleSpinBox::valueChanged, this, [this]() { emit CalculationsChanged(Calculations()); });

    const QSharedPointer<AbstractModel> model = m_model.toStrongRef();
    // "Fit" is the optimisation flag of the model itself
    connect(row.fit, &QCheckBox::toggled, model.data(), [model, global, index, series](bool on) {
        if (global)
            model->GlobalTable()->setChecked(0, index, on);
        else
            model->LocalTable()->setChecked(series, index, on);
    });
    connect(model.data(), &AbstractModel::Recalculated, this, [this, at, global, index, series, setLabel, enableRange]() {
        const QSharedPointer<AbstractModel> m = m_model.toStrongRef();
        if (!m)
            return;
        const bool enabled = global ? m->GlobalEnabled(index) : m->LocalEnabled(index);
        m_rows[at].vary->setEnabled(enabled);
        m_rows[at].fit->setEnabled(enabled);
        setLabel(global ? m->GlobalParameter(index) : m->LocalParameter(index, series));
        enableRange();
    });
    row.vary->setEnabled(global ? model->GlobalEnabled(index) : model->LocalEnabled(index));
    row.fit->setEnabled(row.vary->isEnabled());
    enableRange();
    return frame;
}

ScanWidget::ScanWidget(const QSharedPointer<AbstractModel>& model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);

    // the frames are collected first: Row copies in the list stay valid, the widgets belong to the frames
    for (int i = 0; i < model->GlobalParameterSize(); ++i)
        layout->addWidget(AddRow(model->GlobalParameterName(i), model->GlobalParameter(i), true, i, 0, true));

    QWidget* more = nullptr;
    QVBoxLayout* moreLayout = nullptr;
    if (!model->SupportSeries()) { // the parameters of the model, one set
        for (int i = 0; i < model->LocalParameterSize(); ++i)
            layout->addWidget(AddRow(model->LocalParameterName(i), model->LocalParameter(i, 0), false, i, 0, true));
    } else if (model->SeriesCount() > 0 && model->LocalParameterSize(0) > 0) {
        more = new QWidget;
        moreLayout = new QVBoxLayout;
        moreLayout->setContentsMargins(0, 0, 0, 0);
        for (int j = 0; j < model->SeriesCount(); ++j)
            for (int i = 0; i < model->LocalParameterSize(j); ++i)
                moreLayout->addWidget(AddRow(tr("%1, series %2").arg(model->LocalParameterName(i)).arg(j + 1), model->LocalParameter(i, j), false, i, j, false));
        more->setLayout(moreLayout);
    }
    if (more)
        AddFolded(layout, more, QStringLiteral("scan_more"));

    m_count = new QLabel;
    m_count->setWordWrap(true);
    layout->addWidget(m_count);
    connect(this, &ScanWidget::CalculationsChanged, m_count, [this](int n) { m_count->setText(tr("No of calculations to be done: %1").arg(n)); });
    m_count->setText(tr("No of calculations to be done: %1").arg(Calculations()));
    setLayout(layout);
}

int ScanWidget::Calculations() const
{
    int count = 1;
    for (const Row& r : m_rows) {
        if (!r.vary->isChecked() || !r.vary->isEnabled())
            continue;
        if (r.max->value() > r.min->value())
            count *= std::ceil((r.max->value() - r.min->value()) / r.step->value());
    }
    return count;
}

QJsonObject ScanWidget::Job() const
{
    QJsonObject job;
    job["ParameterSize"] = m_rows.size();
    job["Method"] = SupraFit::Method::GlobalSearch;
    for (int i = 0; i < m_rows.size(); ++i) {
        const Row& r = m_rows[i];
        double min = r.value, max = r.value, step = 1;
        if (r.vary->isChecked() && r.vary->isEnabled()) {
            min = r.min->value();
            max = r.max->value();
            step = r.step->value();
        }
        job[QString::number(i)] = ToolSet::DoubleVec2String(QVector<qreal>() << min << max << step << (r.fit->isChecked() ? 1 : 0));
    }
    return job;
}
