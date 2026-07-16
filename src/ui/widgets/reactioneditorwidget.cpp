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

#include <QtCore/QChar>
#include <QtCore/QRegularExpression>

#include <QtGui/QFont>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#include "src/core/reactionparser.h"

#include "reactioneditorwidget.h"

ReactionHighlighter::ReactionHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    m_arrowFormat.setForeground(QColor(0x1e, 0x88, 0xe5));
    m_arrowFormat.setFontWeight(QFont::Bold);
    m_coeffFormat.setForeground(QColor(0xd8, 0x1b, 0x60));
    m_componentFormat.setFontWeight(QFont::Bold);
    m_componentFormat.setForeground(QColor(0x2e, 0x7d, 0x32));
}

void ReactionHighlighter::setComponents(const QStringList& components)
{
    // rehighlight() edits the document, which makes QTextEdit re-emit textChanged; that is wired back
    // to updatePreview() -> setComponents(), so rehighlighting unconditionally would recurse forever.
    // Only rehighlight when the component set actually changed, which also breaks that loop. CG.
    if (m_components == components)
        return;
    m_components = components;
    rehighlight();
}

void ReactionHighlighter::highlightBlock(const QString& text)
{
    // equilibrium arrows (longest first so the multi-char tokens win)
    static const QStringList arrows = { QStringLiteral("<=>"), QStringLiteral("<->"),
        QStringLiteral("->"), QStringLiteral("=>"), QString(QChar(0x21CC)), QStringLiteral("=") };
    int pos = 0;
    while (pos < text.size()) {
        bool matched = false;
        for (const QString& arrow : arrows) {
            if (text.mid(pos, arrow.size()) == arrow) {
                setFormat(pos, arrow.size(), m_arrowFormat);
                pos += arrow.size();
                matched = true;
                break;
            }
        }
        if (!matched)
            ++pos;
    }

    // integer coefficients
    static const QRegularExpression coeff(QStringLiteral("\\b\\d+\\b"));
    auto it = coeff.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), m_coeffFormat);
    }

    // known component symbols (whole-word)
    for (const QString& comp : m_components) {
        if (comp.isEmpty())
            continue;
        const QRegularExpression re(QStringLiteral("\\b%1\\b").arg(QRegularExpression::escape(comp)));
        auto cit = re.globalMatch(text);
        while (cit.hasNext()) {
            const QRegularExpressionMatch m = cit.next();
            setFormat(m.capturedStart(), m.capturedLength(), m_componentFormat);
        }
    }
}

QVector<ReactionEditorWidget::Preset> ReactionEditorWidget::Presets()
{
    // Grouped into the established host-guest stoichiometries, their host-dimerisation variants (a
    // preceding 2 A <=> A2), and a few three-component systems. This replaces the convenience the
    // removed MaxA/MaxB grid gave. A header has an empty reactions string. Claude Generated.
    return {
        { tr("— Established models —"), QString() },
        { tr("1:1"), QStringLiteral("A + B <=> AB") },
        { tr("2:1 / 1:1"), QStringLiteral("A + B <=> AB\n2 A + B <=> A2B") },
        { tr("1:1 / 1:2"), QStringLiteral("A + B <=> AB\nA + 2 B <=> AB2") },
        { tr("2:1 / 1:1 / 1:2"), QStringLiteral("A + B <=> AB\n2 A + B <=> A2B\nA + 2 B <=> AB2") },
        { tr("— + host dimerisation —"), QString() },
        { tr("1:1, A2"), QStringLiteral("2 A <=> A2\nA + B <=> AB") },
        { tr("2:1 / 1:1, A2"), QStringLiteral("2 A <=> A2\nA + B <=> AB\n2 A + B <=> A2B") },
        { tr("1:1 / 1:2, A2"), QStringLiteral("2 A <=> A2\nA + B <=> AB\nA + 2 B <=> AB2") },
        { tr("2:1 / 1:1 / 1:2, A2"), QStringLiteral("2 A <=> A2\nA + B <=> AB\n2 A + B <=> A2B\nA + 2 B <=> AB2") },
        { tr("— Three components —"), QString() },
        { tr("Competitive (A+B, A+C)"), QStringLiteral("A + B <=> AB\nA + C <=> AC") },
        { tr("Ternary (A+B+C <=> ABC)"), QStringLiteral("A + B <=> AB\nA + B + C <=> ABC") },
    };
}

ReactionEditorWidget::ReactionEditorWidget(const QString& initial, QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    // Left: one-click presets (shared with the "Add model" menu via Presets()).
    QListWidget* presets = new QListWidget;
    presets->setMaximumWidth(210);
    presets->setToolTip(tr("Click a preset to load its reaction system into the editor."));
    for (const Preset& p : Presets()) {
        QListWidgetItem* item = new QListWidgetItem(p.name, presets);
        if (p.isHeader()) { // section header: bold, not selectable
            item->setFlags(Qt::NoItemFlags);
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
        } else {
            item->setData(Qt::UserRole, p.reactions);
            item->setToolTip(QString(p.reactions).replace('\n', QStringLiteral(" ; ")));
        }
    }
    outer->addWidget(presets);

    // Right: hint, editor and live preview.
    QVBoxLayout* right = new QVBoxLayout;
    right->setContentsMargins(0, 0, 0, 0);

    QLabel* hint = new QLabel(tr("One reaction per line, e.g. <tt>A + B &lt;=&gt; AB</tt> or "
                                 "<tt>2 A &lt;=&gt; A2</tt>. The left side lists the free components; "
                                 "arrows <tt>&lt;=&gt; = &lt;-&gt; -&gt;</tt> are accepted. Component "
                                 "order follows first appearance and maps to the data columns."));
    hint->setWordWrap(true);
    right->addWidget(hint);

    m_editor = new QTextEdit;
    m_editor->setAcceptRichText(false);
    m_editor->setPlainText(initial);
    QFont mono = m_editor->font();
    mono.setStyleHint(QFont::Monospace);
    mono.setFamily(QStringLiteral("monospace"));
    m_editor->setFont(mono);
    m_editor->setMinimumHeight(90);
    right->addWidget(m_editor);

    m_highlighter = new ReactionHighlighter(m_editor->document());

    m_preview = new QLabel;
    m_preview->setWordWrap(true);
    m_preview->setTextFormat(Qt::RichText);
    right->addWidget(m_preview);

    outer->addLayout(right, 1);

    connect(m_editor, &QTextEdit::textChanged, this, [this]() {
        updatePreview();
        emit changed(reactionText());
    });
    // A preset click loads its full reaction system (replaces the current text); headers carry no data.
    connect(presets, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        const QString reactions = item->data(Qt::UserRole).toString();
        if (!reactions.isEmpty())
            m_editor->setPlainText(reactions);
    });

    updatePreview();
}

QString ReactionEditorWidget::reactionText() const
{
    return m_editor->toPlainText();
}

void ReactionEditorWidget::updatePreview()
{
    const ReactionSystem system = ReactionParser::Parse(reactionText());
    m_highlighter->setComponents(system.components);

    if (reactionText().trimmed().isEmpty()) {
        m_preview->setText(tr("<i>No reactions — the model stays undefined. Enter at least one, e.g. 'A + B &lt;=&gt; AB'.</i>"));
        return;
    }

    QString html;
    html += tr("<b>Components (%1):</b> %2<br>")
                .arg(system.components.size())
                .arg(system.components.isEmpty() ? tr("none") : system.components.join(", "));

    QStringList speciesLabels;
    for (const ReactionSpecies& s : system.species)
        speciesLabels << s.label;
    html += tr("<b>Species (%1):</b> %2")
                .arg(speciesLabels.size())
                .arg(speciesLabels.isEmpty() ? tr("none") : speciesLabels.join(", "));

    QStringList errors;
    for (const ReactionDiagnostic& d : system.diagnostics) {
        if (!d.ok)
            errors << tr("line %1: %2").arg(d.line).arg(d.message);
    }
    if (!errors.isEmpty())
        html += QStringLiteral("<br><span style='color:#c62828'>%1</span>").arg(errors.join("<br>"));

    m_preview->setText(html);
}
