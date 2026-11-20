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

#include "outdatedbanner.h"

#include "src/ui/guitools/statusline.h"

#include <QtCore/QEvent>
#include <QtGui/QPalette>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

OutdatedBanner::OutdatedBanner(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("outdatedBanner"));
    m_label = new QLabel(tr("This model was fitted to earlier data."));
    m_label->setWordWrap(true);
    m_fit = new QPushButton(tr("Fit again"));
    m_fit->setObjectName(QStringLiteral("outdatedFitAgain"));
    m_fit->setToolTip(tr("Fit the model to the current data, as the Fit button does"));
    m_current = new QPushButton(tr("Mark as current"));
    m_current->setObjectName(QStringLiteral("outdatedMarkCurrent"));
    m_current->setToolTip(tr("Keep the parameters as a fit of the current data without fitting again. Only do this if the fit still holds for the changed data."));
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->addWidget(m_label, 1);
    layout->addWidget(m_fit);
    layout->addWidget(m_current);
    connect(m_fit, &QPushButton::clicked, this, &OutdatedBanner::fitAgain);
    connect(m_current, &QPushButton::clicked, this, &OutdatedBanner::markCurrent);
    setFrameShape(QFrame::StyledPanel);
    setAutoFillBackground(true);
    updateColors();
    hide();
}

void OutdatedBanner::setOutdated(bool outdated)
{
    setVisible(outdated);
}

void OutdatedBanner::setFitAvailable(bool available)
{
    m_fit->setVisible(available);
}

void OutdatedBanner::changeEvent(QEvent* event)
{
    if (!m_updating && (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange))
        updateColors();
    QFrame::changeEvent(event);
}

/*! \brief The window colour with a fifth of the warning amber, the text in the window text colour (both schemes). */
void OutdatedBanner::updateColors()
{
    const QPalette base = QPalette();
    const QColor window = base.color(QPalette::Window), amber = StatusLine::color(StatusLine::Warning);
    const qreal f = 0.2;
    const QColor tint = QColor::fromRgbF(window.redF() * (1 - f) + amber.redF() * f, window.greenF() * (1 - f) + amber.greenF() * f,
        window.blueF() * (1 - f) + amber.blueF() * f);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, tint);
    palette.setColor(QPalette::WindowText, base.color(QPalette::WindowText));
    // setting the palette sends a palette change event of its own: not handled again
    m_updating = true;
    setPalette(palette);
    m_updating = false;
    m_label->setForegroundRole(QPalette::WindowText);
}
