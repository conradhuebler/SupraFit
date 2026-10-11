/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtWidgets/QScrollArea>

#include "genericwidgetdialog.h"

GenericWidgetDialog::GenericWidgetDialog(const QString& title, QWidget* widget, QWidget* parent)
    : QDialog(parent)
    , m_widget(widget)
{
    setModal(true);
    // A fixed 800x600 was too small for content-rich widgets — the scripted-model definition alone has
    // ten fields plus a preset column and a guidance panel — so every open began with a manual resize.
    // Size relative to the available screen instead, capped so it stays a dialog. Claude Generated.
    QScreen* screen = QGuiApplication::primaryScreen();
    const QRect available = screen ? screen->availableGeometry() : QRect(0, 0, 1280, 900);
    resize(qMin(1500, static_cast<int>(available.width() * 0.85)),
        qMin(950, static_cast<int>(available.height() * 0.85)));
    setWindowTitle(title.toUtf8());
    setUi();
}

void GenericWidgetDialog::setUi()
{
    QGridLayout* mainlayout = new QGridLayout;
    setLayout(mainlayout);

    // The hosted widget can be far taller than the dialog — the scripted-model definition alone has
    // ten fields in a flow layout — so it goes into a scroll area instead of overflowing the dialog
    // and pushing the buttons off screen. setWidgetResizable lets it use the full width and reflow.
    // Claude Generated.
    QScrollArea* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(m_widget);
    mainlayout->addWidget(scroll, 0, 1, 1, 3);

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainlayout->addWidget(m_buttonbox, 1, 1, 1, 3);
}
