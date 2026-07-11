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

#include <QtWidgets/QLabel>
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

ReactionEditorWidget::ReactionEditorWidget(const QString& initial, QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    QLabel* hint = new QLabel(tr("One reaction per line, e.g. <tt>A + B &lt;=&gt; AB</tt> or "
                                 "<tt>2 A &lt;=&gt; A2</tt>. The left side lists the free components; "
                                 "arrows <tt>&lt;=&gt; = &lt;-&gt; -&gt;</tt> are accepted. Component "
                                 "order follows first appearance and maps to the data columns."));
    hint->setWordWrap(true);
    outer->addWidget(hint);

    m_editor = new QTextEdit;
    m_editor->setAcceptRichText(false);
    m_editor->setPlainText(initial);
    QFont mono = m_editor->font();
    mono.setStyleHint(QFont::Monospace);
    mono.setFamily(QStringLiteral("monospace"));
    m_editor->setFont(mono);
    m_editor->setMinimumHeight(90);
    outer->addWidget(m_editor);

    m_highlighter = new ReactionHighlighter(m_editor->document());

    m_preview = new QLabel;
    m_preview->setWordWrap(true);
    m_preview->setTextFormat(Qt::RichText);
    outer->addWidget(m_preview);

    connect(m_editor, &QTextEdit::textChanged, this, [this]() {
        updatePreview();
        emit changed(reactionText());
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
        m_preview->setText(tr("<i>No reactions — the model falls back to the MaxA/MaxB grid.</i>"));
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
