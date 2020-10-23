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

#include <QtWidgets/QComboBox>
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

SpectraImport::SpectraImport(const QString& directory)
{
    QStringList path = directory.split("|||");
    setUI();
    if (path.size() == 2) {
        m_path->setText(path[0]);
        m_spectrawidget->setDirectory(path[0], path[1]);
    } else {
        m_path->setText(directory);
        m_spectrawidget->setDirectory(directory, "csv");
    }
}

void SpectraImport::setUI()
{
    QGridLayout* layout = new QGridLayout;

    m_directory = new QPushButton(tr("Open Directory"));
    connect(m_directory, &QPushButton::clicked, this, &SpectraImport::setDirectory);
    m_path = new QLineEdit;

    m_spectrawidget = new SpectraWidget;

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &SpectraImport::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_file_type = new QComboBox;
    m_file_type->addItem("csv");
    m_file_type->addItem("absorb");
    m_file_type->addItem("xy");
    m_file_type->setCurrentText(qApp->instance()->property("LastSpectraType").toString());

    layout->addWidget(m_file_type, 0, 0);
    layout->addWidget(m_directory, 0, 1);
    layout->addWidget(m_path, 0, 2);
    layout->addWidget(m_spectrawidget, 1, 0, 1, 3);

    layout->addWidget(m_buttonbox, 2, 2);

    setLayout(layout);
    setWindowTitle(tr("Spectra Analysis and Import"));
    setMinimumSize(1280, 800);
}

void SpectraImport::setDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), getDir());

    QString dir = QString("%1|||%2").arg(directory).arg(m_file_type->currentText());
    setLastDir(dir);
    m_path->setText(directory);
    m_spectrawidget->setDirectory(directory, m_file_type->currentText());
    qApp->instance()->setProperty("LastSpectraType", m_file_type->currentText());
}

void SpectraImport::setData(const QJsonObject& data)
{
    m_spectrawidget->setData(data);
}

void SpectraImport::accept()
{
    m_spectrawidget->UpdateData();
    QDialog::accept();
}
