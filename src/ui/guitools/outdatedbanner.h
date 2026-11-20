/*
 * OutdatedBanner - the note of a model pane that its fit belongs to earlier data (Edit Data)
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
 * Claude Generated (Edit Data, E1).
 */

#pragma once

#include <QtWidgets/QFrame>

class QLabel;
class QPushButton;

/*! \brief A line in the head of a model pane, shown while the model is marked outdated (`AbstractModel::isDataOutdated`):
 *  the text "This model was fitted to earlier data." and the buttons "Fit again" and "Mark as current" (Claude Generated).
 *
 *  The colours come from the palette: the window colour tinted with the warning amber of `StatusLine`, the text in the window
 *  text colour; they follow a change of the palette. The host decides visibility (`setOutdated`) and what the buttons do. */
class OutdatedBanner : public QFrame {
    Q_OBJECT
public:
    explicit OutdatedBanner(QWidget* parent = nullptr);

    /*! \brief Shows the banner for an outdated model, hides it otherwise. */
    void setOutdated(bool outdated);
    /*! \brief Hides "Fit again" for a pane that has no fit of its own. */
    void setFitAvailable(bool available);

    inline QPushButton* fitButton() const { return m_fit; }
    inline QPushButton* currentButton() const { return m_current; }
    inline QLabel* label() const { return m_label; }

signals:
    void fitAgain();
    void markCurrent();

protected:
    void changeEvent(QEvent* event) override;

private:
    void updateColors();

    QLabel* m_label;
    QPushButton *m_fit, *m_current;
    bool m_updating = false;
};
