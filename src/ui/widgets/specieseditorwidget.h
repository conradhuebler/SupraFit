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

#pragma once

#include <QtCore/QVector>

#include <QtWidgets/QWidget>

class QSpinBox;
class QVBoxLayout;
class QLabel;

/**
 * @brief Compact editor for an equilibrium species list (Claude Generated).
 *
 * @deprecated Superseded by ReactionEditorWidget (PrepareBox type 6), which parses free-text reaction
 * equations into an arbitrary N-component system. This spinbox editor is limited to the 2-component
 * host/guest case and is kept only so existing type-5 model definitions still load; it will be
 * removed once the reaction editor is validated in the field.
 *
 * Lets the user assemble the species of a general equilibrium model row by row: each row holds the
 * stoichiometric coefficients (a of host A, b of guest B) of one species, so mixed complexes A_aB_b
 * and self-aggregates such as A2 (a=2, b=0) can be mixed freely. Serialises to the "a,b|a,b" string
 * consumed by nmr_any_Model::DefineModel() (the optional "Species" field).
 */
class SpeciesEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit SpeciesEditorWidget(const QString& initial, QWidget* parent = nullptr);

    /** @brief Current species list encoded as "a,b|a,b|..." (empty rows are dropped). */
    QString toSpeciesString() const;

signals:
    /** @brief Emitted whenever the species list changes; carries the encoded string. */
    void changed(const QString& speciesString);

private:
    struct Row {
        QWidget* container = nullptr;
        QSpinBox* a = nullptr;
        QSpinBox* b = nullptr;
        QLabel* name = nullptr;
    };

    void addRow(int a, int b);
    void removeRow(QWidget* container);
    void updateRowName(const Row& row);
    void emitChanged();

    /** @brief Human-readable species label, e.g. (2,0) -> "A2", (1,1) -> "AB", (2,1) -> "A2B". */
    static QString speciesLabel(int a, int b);

    QVBoxLayout* m_rows_layout = nullptr;
    QVector<Row> m_rows;
};
