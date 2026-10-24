/*
 * Quick-bar dialog: what the bar of the models you add most shows.
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */


#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>

#include "modelcatalog.h"

#include "quickbardialog.h"

namespace {
QString KindLabel(ModelCatalog::Kind kind)
{
    switch (kind) {
    case ModelCatalog::Kind::Stoichiometry:
        return QObject::tr("stoichiometry");
    case ModelCatalog::Kind::FixedModel:
        return QObject::tr("model");
    case ModelCatalog::Kind::ScriptPreset:
        return QObject::tr("scripted model");
    case ModelCatalog::Kind::ModelFile:
        return QObject::tr("model file");
    }
    return QString();
}
}

QuickBarDialog::QuickBarDialog(const ModelUsage::Preferences& preferences, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Customise the quick bar"));

    m_frequent = new QRadioButton(tr("The models I add most"));
    m_frequent->setObjectName(QStringLiteral("frequent"));
    m_fixed = new QRadioButton(tr("A list I put together"));
    m_fixed->setObjectName(QStringLiteral("fixed"));
    (preferences.mode == ModelUsage::Mode::Fixed ? m_fixed : m_frequent)->setChecked(true);

    m_count = new QSpinBox;
    m_count->setObjectName(QStringLiteral("count"));
    m_count->setRange(1, 20);
    m_count->setValue(preferences.count);
    m_count->setSuffix(tr(" models"));

    // everything that can be added; the checked ones, in the stored order, come first
    m_list = new QListWidget;
    m_list->setObjectName(QStringLiteral("fixed_list"));
    m_list->setToolTip(tr("Tick what the bar shows; the order is the order on the bar."));
    const QList<ModelCatalog::Entry> all = ModelUsage::Candidates(0);
    QStringList added;
    auto addItem = [&](const ModelCatalog::Entry& entry, bool checked) {
        QListWidgetItem* item = new QListWidgetItem(tr("%1  \u00b7  %2").arg(entry.title, KindLabel(entry.kind)), m_list);
        item->setData(Qt::UserRole, entry.key);
        item->setToolTip(entry.tooltip);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        added << entry.key;
    };
    for (const QString& key : preferences.fixed)
        for (const ModelCatalog::Entry& entry : all)
            if (entry.key == key && !added.contains(key))
                addItem(entry, true);
    for (const ModelCatalog::Entry& entry : all)
        if (!added.contains(entry.key))
            addItem(entry, false);

    m_up = new QPushButton(tr("Up"));
    m_up->setObjectName(QStringLiteral("up"));
    m_down = new QPushButton(tr("Down"));
    m_down->setObjectName(QStringLiteral("down"));
    connect(m_up, &QPushButton::clicked, this, [this]() { Move(-1); });
    connect(m_down, &QPushButton::clicked, this, [this]() { Move(1); });

    m_technique = new QComboBox;
    m_technique->setObjectName(QStringLiteral("technique"));
    m_technique->addItem(tr("the one I used last"), QString());
    for (const ModelCatalog::TechniqueInfo& info : ModelCatalog::Techniques())
        m_technique->addItem(info.name, info.key);
    const int at = m_technique->findData(preferences.technique);
    m_technique->setCurrentIndex(at >= 0 ? at : 0);
    m_technique->setToolTip(tr("What a click on a model uses as long as that model was not added with any technique."));

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(new QLabel(tr("The bar shows:")), 0, 0, 1, 3);
    QHBoxLayout* frequent = new QHBoxLayout;
    frequent->addWidget(m_frequent);
    frequent->addWidget(m_count);
    frequent->addStretch();
    layout->addLayout(frequent, 1, 0, 1, 3);
    layout->addWidget(m_fixed, 2, 0, 1, 3);
    layout->addWidget(m_list, 3, 0, 3, 2);
    layout->addWidget(m_up, 3, 2);
    layout->addWidget(m_down, 4, 2);
    layout->setRowStretch(5, 1);
    layout->addWidget(new QLabel(tr("A click on a model that was not used yet uses:")), 6, 0, 1, 3);
    layout->addWidget(m_technique, 7, 0, 1, 3);
    layout->addWidget(buttons, 8, 0, 1, 3);

    connect(m_frequent, &QRadioButton::toggled, this, &QuickBarDialog::UpdateEnabled);
    UpdateEnabled();
    resize(460, 520);
}

void QuickBarDialog::UpdateEnabled()
{
    const bool fixed = m_fixed->isChecked();
    m_count->setEnabled(!fixed);
    m_list->setEnabled(fixed);
    m_up->setEnabled(fixed);
    m_down->setEnabled(fixed);
}

void QuickBarDialog::Move(int by)
{
    const int row = m_list->currentRow();
    const int to = row + by;
    if (row < 0 || to < 0 || to >= m_list->count())
        return;
    QListWidgetItem* item = m_list->takeItem(row);
    m_list->insertItem(to, item);
    m_list->setCurrentRow(to);
}

ModelUsage::Preferences QuickBarDialog::preferences() const
{
    ModelUsage::Preferences p;
    p.mode = m_fixed->isChecked() ? ModelUsage::Mode::Fixed : ModelUsage::Mode::Frequent;
    p.count = m_count->value();
    for (int i = 0; i < m_list->count(); ++i)
        if (m_list->item(i)->checkState() == Qt::Checked)
            p.fixed << m_list->item(i)->data(Qt::UserRole).toString();
    p.technique = m_technique->currentData().toString();
    return p;
}
