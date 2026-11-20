/*
 * StatusLine - one-line message of a dialog (information, warning, error) in palette colours
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
 * Claude Generated (UX package 1 of the import dialogs).
 */

#include <algorithm>

#include <QtCore/QEvent>
#include <QtGui/QFontMetrics>
#include <QtGui/QPalette>

#include "src/ui/guitools/appearance.h"

#include "statusline.h"

StatusLine::StatusLine(QWidget* parent)
    : QLabel(parent)
{
    setWordWrap(false);
    setTextFormat(Qt::PlainText);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed); // never widens the dialog
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    updateColor();
}

QColor StatusLine::color(Level level)
{
    const bool dark = Appearance::IsDark();
    switch (level) {
    case Warning:
        return dark ? QColor(240, 190, 90) : QColor(150, 90, 0);
    case Error:
        return dark ? QColor(255, 120, 110) : QColor(180, 30, 20);
    case Info:
        break;
    }
    return QPalette().color(QPalette::WindowText);
}

void StatusLine::setMessage(const QString& text, Level level)
{
    m_text = text;
    m_level = level;
    setToolTip(text);
    updateColor();
    updateText();
}

void StatusLine::clearMessage()
{
    setMessage(QString(), Info);
}

QSize StatusLine::minimumSizeHint() const
{
    return QSize(0, fontMetrics().height() + 2 * margin());
}

void StatusLine::resizeEvent(QResizeEvent* event)
{
    QLabel::resizeEvent(event);
    updateText();
}

void StatusLine::changeEvent(QEvent* event)
{
    QLabel::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange)
        updateColor();
    else if (event->type() == QEvent::FontChange)
        updateText();
}

void StatusLine::updateText()
{
    const QString line = m_text.section('\n', 0, 0);
    const QString shown = fontMetrics().elidedText(line, Qt::ElideRight, std::max(0, contentsRect().width()));
    // the elided text differs only while the line is too narrow; an ellipsis also marks further lines
    QLabel::setText(line == m_text ? shown : shown + (shown.endsWith(QChar(0x2026)) ? QString() : QStringLiteral(" …")));
}

void StatusLine::updateColor()
{
    QPalette p = palette();
    // Info takes the text colour of the application palette (light or dark scheme)
    p.setColor(QPalette::WindowText, m_level == Info ? QPalette().color(QPalette::WindowText) : color(m_level));
    if (p != palette()) // setPalette sends PaletteChange, which comes back here with an equal palette
        setPalette(p);
}
