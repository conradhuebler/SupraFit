/*
 * SupraFit - parts shared by the analysis dialogs
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
 * Claude Generated (roadmap/statistic_dialogs.md, WP-S1).
 */

#include "analysisparts.h"

#include "src/core/models/AbstractModel.h"
#include "src/core/toolset.h"
#include "src/ui/guitools/flowlayout.h"
#include "src/ui/guitools/morelesstoggle.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

namespace {
constexpr double DefaultBoxScaling = 1.5; ///< the box of a parameter in the Model Comparison, as a factor of its value
}

ParameterSelector::ParameterSelector(const QSharedPointer<AbstractModel>& model, bool withScaling, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_withScaling(withScaling)
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    if (model.isNull()) { // the project: no parameter to pick, only whether the local ones belong to it
        QHBoxLayout* row = new QHBoxLayout;
        row->addWidget(new QLabel(tr("Parameters:")));
        m_scope = new QComboBox;
        m_scope->addItem(tr("All parameters (global and local)"));
        m_scope->addItem(tr("Global parameters only"));
        m_scope->setToolTip(tr("The parameters of a model are known when the analysis runs on it: every fitted parameter of every model is tested."));
        row->addWidget(m_scope, 1);
        layout->addLayout(row);
        return;
    }

    layout->addWidget(new QLabel(tr("Parameters to test:")));
    // the items go into the layout first and the layout onto the widget last: only then are the check boxes and labels of the items
    // children of the widget (a FlowLayout does not adopt them itself)
    QWidget* chips = new QWidget;
    FlowLayout* flow = new FlowLayout;

    // the scaling of every box, labelled, for "..more.."
    QWidget* scaling = nullptr;
    QFormLayout* scalingLayout = nullptr;
    if (withScaling) {
        scaling = new QWidget;
        scalingLayout = new QFormLayout;
        scalingLayout->setContentsMargins(0, 0, 0, 0);
        scaling->setLayout(scalingLayout);
        m_scaling = scaling;
    }

    auto addParameter = [&](const QString& name, bool checked, bool global, int index) {
        QCheckBox* box = new QCheckBox;
        box->setChecked(checked);
        QHBoxLayout* item = new QHBoxLayout;
        item->addWidget(box);
        item->addWidget(new QLabel(QStringLiteral("<html>%1</html>").arg(name)));
        item->addStretch(100);
        flow->addItem(item);
        (global ? m_global : m_local) << box;

        QDoubleSpinBox* scale = new QDoubleSpinBox;
        scale->setMinimum(0);
        scale->setMaximum(50);
        scale->setValue(DefaultBoxScaling);
        scale->setToolTip(tr("Factor of the box in which %1 is varied.").arg(name));
        (global ? m_globalScale : m_localScale) << scale;
        if (scalingLayout)
            scalingLayout->addRow(new QLabel(QStringLiteral("<html>%1</html>").arg(name)), scale);

        // a parameter the model does not fit cannot be tested
        connect(model.data(), &AbstractModel::Recalculated, this, [this, box, global, index]() {
            const QSharedPointer<AbstractModel> m = m_model.toStrongRef();
            if (m)
                box->setEnabled(global ? m->GlobalEnabled(index) : m->LocalEnabled(index));
        });
    };
    for (int i = 0; i < model->GlobalParameterSize(); ++i)
        addParameter(model->GlobalParameterName(i), true, true, i);
    for (int i = 0; i < model->LocalParameterSize(); ++i)
        addParameter(model->LocalParameterName(i), false, false, i);
    chips->setLayout(flow);
    layout->addWidget(chips);
}

void ParameterSelector::AddTo(QJsonObject& job) const
{
    const QSharedPointer<AbstractModel> model = m_model.toStrongRef();
    if (!model) {
        job["AllParameters"] = true;
        job["AllLocalParameters"] = !m_scope || m_scope->currentIndex() == 0;
        job["BoxScaling"] = m_withScaling;
        return;
    }
    QList<int> globalFlags, localFlags;
    QVector<double> globalBox, localBox;
    for (int i = 0; i < m_global.size(); ++i) {
        if (model->GlobalTable()->isChecked(0, i))
            globalFlags << int(m_global[i]->isChecked() && m_global[i]->isEnabled());
        globalBox << m_globalScale[i]->value();
    }
    for (int i = 0; i < m_local.size(); ++i)
        for (int j = 0; j < model->SeriesCount(); ++j) {
            if (model->LocalTable()->isChecked(j, i))
                localFlags << int(m_local[i]->isChecked() && m_local[i]->isEnabled());
            localBox << m_localScale[i]->value();
        }
    job["GlobalParameterList"] = ToolSet::IntList2String(globalFlags);
    job["LocalParameterList"] = ToolSet::IntList2String(localFlags);
    if (m_withScaling) {
        job["GlobalParameterScalingList"] = ToolSet::DoubleVec2String(globalBox);
        job["LocalParameterScalingList"] = ToolSet::DoubleVec2String(localBox);
    }
}

void ParameterSelector::ResolveFor(QJsonObject& job, const QSharedPointer<AbstractModel>& model)
{
    // read with value(): operator[] on a non-const object would add the key (as null) to every job that passes
    if (!job.value("AllParameters").toBool() || !model)
        return;

    // the limit of the sum of squares is a property of the model (its SSE), the dialog of the project has none to offer
    const double confidence = job.value("confidence").toDouble(95.0) / 100.0;
    job["f-value"] = model->finv(confidence);
    job["MaxParameter"] = model->ErrorfTestThreshold(confidence);

    const bool locals = job.value("AllLocalParameters").toBool(true);
    QList<int> globalFlags, localFlags;
    QVector<double> globalBox, localBox;
    for (int i = 0; i < model->GlobalParameterSize(); ++i) {
        if (model->GlobalTable()->isChecked(0, i))
            globalFlags << int(model->GlobalEnabled(i));
        globalBox << DefaultBoxScaling;
    }
    for (int i = 0; i < model->LocalParameterSize(); ++i)
        for (int j = 0; j < model->SeriesCount(); ++j) {
            if (locals && model->LocalTable()->isChecked(j, i))
                localFlags << int(model->LocalEnabled(i));
            localBox << DefaultBoxScaling;
        }
    job["GlobalParameterList"] = ToolSet::IntList2String(globalFlags);
    job["LocalParameterList"] = ToolSet::IntList2String(localFlags);
    if (job.value("BoxScaling").toBool()) {
        job["GlobalParameterScalingList"] = ToolSet::DoubleVec2String(globalBox);
        job["LocalParameterScalingList"] = ToolSet::DoubleVec2String(localBox);
    }
}

void AddFolded(QVBoxLayout* layout, QWidget* more, const QString& id)
{
    MoreLessToggle* toggle = new MoreLessToggle(more, id);
    layout->addWidget(toggle, 0, Qt::AlignLeft);
    layout->addWidget(more);
}
