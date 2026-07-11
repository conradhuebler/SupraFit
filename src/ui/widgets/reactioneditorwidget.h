/*
 * SupraFit - live-parsed reaction-equation editor for equilibrium models
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

#include <QtCore/QStringList>

#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextCharFormat>

#include <QtWidgets/QWidget>

class QLabel;
class QTextEdit;

/**
 * @brief Live syntax highlighting for reaction equations: arrows, coefficients and known components.
 *
 * The set of known component symbols is fed in from the live ReactionParser result so components are
 * coloured as the user discovers them. Claude Generated.
 */
class ReactionHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit ReactionHighlighter(QTextDocument* parent = nullptr);
    void setComponents(const QStringList& components);

protected:
    void highlightBlock(const QString& text) override;

private:
    QStringList m_components;
    QTextCharFormat m_arrowFormat;
    QTextCharFormat m_coeffFormat;
    QTextCharFormat m_componentFormat;
};

/**
 * @brief Text editor for equilibrium reaction equations with live parsing and a species preview.
 *
 * The user types one reaction per line (arrow syntax, e.g. "A + B <=> AB", "2 A <=> A2"); on every
 * change the text is parsed by ReactionParser and a preview shows the derived components, species and
 * any per-line errors. The raw reaction text is emitted via changed() and stored verbatim in the
 * model definition (re-parsed in DefineModel). Supersedes the spinbox SpeciesEditorWidget.
 * Claude Generated.
 */
class ReactionEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReactionEditorWidget(const QString& initial, QWidget* parent = nullptr);

    /** @brief Current reaction text (verbatim, one reaction per line). */
    QString reactionText() const;

signals:
    /** @brief Emitted on every edit; carries the raw reaction text. */
    void changed(const QString& reactionText);

private:
    void updatePreview();

    QTextEdit* m_editor = nullptr;
    QLabel* m_preview = nullptr;
    ReactionHighlighter* m_highlighter = nullptr;
};
