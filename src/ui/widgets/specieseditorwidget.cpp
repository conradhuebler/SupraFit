/*
 * SupraFit - graphical editor for an equilibrium species list
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
 *
 */

#include <QtGui/QFont>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>

#include "specieseditorwidget.h"

SpeciesEditorWidget::SpeciesEditorWidget(const QString& initial, QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    QLabel* hint = new QLabel(tr("Deprecated — prefer the reaction-equation editor. Optional; leave "
                                 "empty to use the MaxA/MaxB/MaxSelfA grid. Add rows for an explicit "
                                 "species list — coefficients of host A and guest B per species, e.g. "
                                 "A2 (2, 0) for host dimerisation, AB (1, 1) for 1:1 binding."));
    hint->setWordWrap(true);
    outer->addWidget(hint);

    m_rows_layout = new QVBoxLayout;
    m_rows_layout->setContentsMargins(0, 0, 0, 0);
    outer->addLayout(m_rows_layout);

    QPushButton* add = new QPushButton(tr("+ Add species"));
    add->setToolTip(tr("Add another equilibrium species"));
    connect(add, &QPushButton::clicked, this, [this]() { addRow(1, 1); });
    outer->addWidget(add);
    outer->addStretch();

    // Populate from an existing "a,b|a,b" definition.
    const QStringList tokens = initial.trimmed().split("|", Qt::SkipEmptyParts);
    for (const QString& token : tokens) {
        const QStringList ab = token.split(",");
        if (ab.size() != 2)
            continue;
        addRow(ab[0].trimmed().toInt(), ab[1].trimmed().toInt());
    }
    // Intentionally start empty when there is no initial list: an empty editor serialises to "" so
    // nmr_any falls back to the MaxA/MaxB/MaxSelfA grid (backward compatible).
}

void SpeciesEditorWidget::addRow(int a, int b)
{
    Row row;
    row.container = new QWidget(this);
    QHBoxLayout* h = new QHBoxLayout(row.container);
    h->setContentsMargins(0, 0, 0, 0);

    row.a = new QSpinBox;
    row.a->setRange(0, 12);
    row.a->setValue(a);
    row.b = new QSpinBox;
    row.b->setRange(0, 12);
    row.b->setValue(b);

    h->addWidget(new QLabel(tr("A")));
    h->addWidget(row.a);
    h->addWidget(new QLabel(tr("B")));
    h->addWidget(row.b);

    row.name = new QLabel;
    row.name->setMinimumWidth(48);
    QFont bold = row.name->font();
    bold.setBold(true);
    row.name->setFont(bold);
    h->addWidget(row.name);
    h->addStretch();

    QPushButton* remove = new QPushButton(QStringLiteral("−"));
    remove->setFixedWidth(28);
    remove->setToolTip(tr("Remove this species"));
    QWidget* container = row.container;
    connect(remove, &QPushButton::clicked, this, [this, container]() { removeRow(container); });
    h->addWidget(remove);

    auto onChange = [this, row]() {
        updateRowName(row);
        emitChanged();
    };
    connect(row.a, &QSpinBox::valueChanged, this, onChange);
    connect(row.b, &QSpinBox::valueChanged, this, onChange);

    m_rows_layout->addWidget(row.container);
    m_rows.append(row);
    updateRowName(row);
    emitChanged();
}

void SpeciesEditorWidget::removeRow(QWidget* container)
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].container == container) {
            m_rows.removeAt(i);
            break;
        }
    }
    m_rows_layout->removeWidget(container);
    container->deleteLater();
    emitChanged();
}

void SpeciesEditorWidget::updateRowName(const Row& row)
{
    row.name->setText(speciesLabel(row.a->value(), row.b->value()));
}

QString SpeciesEditorWidget::speciesLabel(int a, int b)
{
    auto part = [](const QString& symbol, int n) -> QString {
        if (n == 0)
            return QString();
        if (n == 1)
            return symbol;
        return symbol + QString::number(n);
    };
    const QString label = part(QStringLiteral("A"), a) + part(QStringLiteral("B"), b);
    return label.isEmpty() ? QStringLiteral("—") : label;
}

QString SpeciesEditorWidget::toSpeciesString() const
{
    QStringList tokens;
    for (const Row& row : m_rows) {
        const int a = row.a->value();
        const int b = row.b->value();
        if (a + b < 1)
            continue; // drop the empty row
        tokens << QStringLiteral("%1,%2").arg(a).arg(b);
    }
    return tokens.join("|");
}

void SpeciesEditorWidget::emitChanged()
{
    emit changed(toSpeciesString());
}
