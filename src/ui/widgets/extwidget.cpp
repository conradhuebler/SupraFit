/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include "src/core/AbstractModel.h"

#include "src/core/toolset.h"

#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QWeakPointer>

#include <QtGui/QClipboard>
#include <QtWidgets/QApplication>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>

#include "extwidget.h"

extWidget::extWidget(QWeakPointer<AbstractModel> model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    QGridLayout* layout = new QGridLayout;

    m_std = new ClickLabel;
    connect(m_std, &ClickLabel::MouseClicked, this, [this]() {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(tr("%1").arg(Print::printDouble(m_model.data()->StdDeviation())));
        m_std->Clicked();
    });

    m_sse = new ClickLabel;
    connect(m_sse, &ClickLabel::MouseClicked, this, [this]() {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(tr("%1").arg(Print::printDouble(m_model.data()->SumofSquares())));
        m_sse->Clicked();
    });

    m_sey = new ClickLabel;
    connect(m_sey, &ClickLabel::MouseClicked, this, [this]() {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(tr("%1").arg(Print::printDouble(m_model.data()->SEy())));
        m_sey->Clicked();
    });

    layout->addWidget(m_sse, 0, 0);
    layout->addWidget(m_std, 0, 1);
    layout->addWidget(m_sey, 0, 2);

    m_ideal = new DnDLabel(tr("<html><i>DnD</i> Ideal Model</html>"));
    m_mc_std = new DnDLabel(tr("<html><i>DnD</i> MC from Model &sigma;</html>"));
    m_mc_sey = new DnDLabel(tr("<html><i>DnD</i> MC from Model SE<sub>y</sub></html>"));
    m_mc_user = new DnDLabel(tr("<html><i>DnD</i> MC from User &sigma;</html>"));
    m_variance = new QDoubleSpinBox;
    m_variance->setMinimum(0);
    m_variance->setMaximum(1e6);
    m_variance->setDecimals(5);
    m_variance->setValue(1e-3);

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(m_ideal);
    hlayout->addWidget(m_mc_std);
    hlayout->addWidget(m_mc_sey);
    hlayout->addWidget(m_mc_user);
    hlayout->addWidget(m_variance);

    layout->addLayout(hlayout, 1, 0, 1, 3);

    setLayout(layout);

    connect(m_model.data(), &AbstractModel::Recalculated, this, &extWidget::Update);
    connect(m_variance, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &extWidget::Update);
}

void extWidget::Update()
{
    m_sse->setText(tr("<h4><i>CnP</i> Sum of Squares: %1</h4>").arg(Print::printDouble(m_model.data()->SumofSquares())));
    m_std->setText(tr("<h4><i>CnP</i> Standard Deviation: %1</h4>").arg(Print::printDouble(m_model.data()->StdDeviation())));
    m_sey->setText(tr("<h4><i>CnP</i> SE<sub>y</sub>: %1</h4>").arg(Print::printDouble(m_model.data()->SEy())));

    QJsonObject data = m_model.data()->ExportData();
    data["dependent"] = m_model.data()->ModelTable()->ExportTable(true);
    data["uuid"] = QString();
    QJsonObject top;
    top["data"] = data;
    QJsonDocument document(top);
    m_ideal->setContent(document.toBinaryData());

    qint64 seed = QDateTime::currentMSecsSinceEpoch();
    std::mt19937 rng(seed);

    DataTable* table = m_model.data()->ModelTable()->PrepareMC(QVector<double>() << m_model.data()->StdDeviation(), rng);
    data["dependent"] = table->ExportTable(true);
    top["data"] = data;
    document = QJsonDocument(top);
    m_mc_std->setContent(document.toBinaryData());
    delete table;

    table = m_model.data()->ModelTable()->PrepareMC(QVector<double>() << m_model.data()->SEy(), rng);
    data["dependent"] = table->ExportTable(true);
    top["data"] = data;
    document = QJsonDocument(top);
    m_mc_sey->setContent(document.toBinaryData());
    delete table;

    table = m_model.data()->ModelTable()->PrepareMC(QVector<double>() << m_variance->value(), rng);
    data["dependent"] = table->ExportTable(true);
    top["data"] = data;
    document = QJsonDocument(top);
    m_mc_user->setContent(document.toBinaryData());
    delete table;
}
