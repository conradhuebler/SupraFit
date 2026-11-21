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

#pragma once

#include <QtGui/QColor>
#include <QtWidgets/QLabel>

/*! \brief A single line of text that says how things stand in a dialog (Claude Generated).
 *
 * Info is drawn in the window text colour, Warning and Error in an amber and a red that are readable on
 * the window colour of the light and the dark scheme (Appearance::IsDark). A text longer than the line
 * is elided at the end; the full text is the tooltip. The colour follows a change of the palette. */
class StatusLine : public QLabel {
    Q_OBJECT
public:
    enum Level {
        Info = 0,
        Warning = 1,
        Error = 2
    };

    explicit StatusLine(QWidget* parent = nullptr);

    /*! \brief Shows text (first line only, the rest is in the tooltip) with the colour of level. */
    void setMessage(const QString& text, Level level = Info);
    void clearMessage();

    inline QString message() const { return m_text; }
    inline Level level() const { return m_level; }

    /*! \brief Text colour of a level on the current application palette. */
    static QColor color(Level level);

    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void updateText();
    void updateColor();

    QString m_text;
    Level m_level = Info;
};
