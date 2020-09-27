/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include <src/ui/widgets/spectrawidget.h>

#include "src/global.h"

#include "spectraimport.h"

SpectraImport::SpectraImport()
{
    setUI();
}

void SpectraImport::setUI()
{
    QGridLayout* layout = new QGridLayout;

    m_directory = new QPushButton(tr("Open Directory"));
    connect(m_directory, &QPushButton::clicked, this, &SpectraImport::setDirectory);
    m_path = new QLineEdit;

    m_spectrawidget = new SpectraWidget;

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(m_directory, 0, 0);
    layout->addWidget(m_path, 0, 1);
    layout->addWidget(m_spectrawidget, 1, 0, 1, 2);

    layout->addWidget(m_buttonbox, 2, 1);

    setLayout(layout);
    setWindowTitle(tr("Spectra Analysis and Import"));
    setMinimumSize(1280, 800);
}

void SpectraImport::setDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), getDir());
    //const QString directory = QFileDialog::getOpenFileName(this, tr("Open Directory"), getDir());

    setLastDir(directory);
    m_path->setText(directory);
    m_spectrawidget->setDirectory(directory);
    //m_spectrawidget->addFile(directory);
}
