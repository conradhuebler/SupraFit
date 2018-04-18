/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QWeakPointer>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSplitter>

#include "src/global.h"

#include "src/core/models.h"

#include "resultsdialog.h"

ResultsDialog::ResultsDialog(QSharedPointer<AbstractModel> model, QWidget* parent)
    : QDialog(parent)
    , m_model(model)
{
    m_layout = new QGridLayout;
    setModal(false);
    m_mainwidget = new QSplitter(Qt::Horizontal);
    m_layout->addWidget(m_mainwidget, 0, 0);
    m_results = new QListWidget;
    m_tabs = new QTabWidget;

    m_mainwidget->addWidget(m_results);
    m_mainwidget->addWidget(m_tabs);

    setWindowTitle("Collected Results for " + m_model.data()->Name());
    setLayout(m_layout);
    resize(1024, 800);
}

void ResultsDialog::ShowResult(SupraFit::Statistic type)
{
}

void ResultsDialog::UpdateList()
{
}
