/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/AbstractModel.h"

#include "src/ui/instance.h"

#include "src/core/toolset.h"

#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QWeakPointer>

#include <QtGui/QClipboard>
#include <QtGui/QDrag>
#include <QtGui/QMouseEvent>

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>

#include "exportsimulationwidget.h"

void DnDLabel::mousePressEvent(QMouseEvent* event)
{
    UpdateContent();
    ModelMime* mimeData = new ModelMime;
    mimeData->setData("application/x-suprafitmodel", m_content);
    mimeData->setText("SupraFitSimulation");

    QDrag* drag = new QDrag(this);
    drag->setPixmap(QPixmap(":/misc/suprafit.png"));
    drag->setMimeData(mimeData);
    drag->setHotSpot(event->pos());
    drag->exec();
}

void DnDLabel::UpdateContent()
{
    QJsonObject top;

    QJsonObject data = m_model.toStrongRef().data()->ExportData();
    data["DataType"] = DataClassPrivate::DataType::Table;
    data["title"] = QString("Simulated Data - %1").arg(QDateTime::currentDateTime().toString());

    if (m_type == 0) // ideal model
    {
        data["dependent"] = m_model.toStrongRef().data()->ModelTable()->ExportTable(true);
        data["content"] = QString("Simulated data from a %1").arg(m_model.toStrongRef().data()->Name());
    } else if (m_type == 1) // SEy from model
    {
        qint64 seed = QDateTime::currentMSecsSinceEpoch();
        std::mt19937 rng(seed);

        DataTable* table = m_model.toStrongRef().data()->ModelTable()->PrepareMC(QVector<double>() << m_model.toStrongRef().data()->SEy(), rng);
        data["dependent"] = table->ExportTable(true);
        data["content"] = QString("Simulated data from a %1 with standard normal distributed random error with %2 %3 N(0,%4=SEy=%5)").arg(m_model.toStrongRef().data()->Name()).arg(Unicode_epsilion).arg(Unicode_Math_Element).arg(Unicode_sigma).arg(m_model.toStrongRef().data()->SEy());
        top["data"] = data;
        delete table;

    } else if (m_type == 2) // sigma from model
    {
        qint64 seed = QDateTime::currentMSecsSinceEpoch();
        std::mt19937 rng(seed);

        DataTable* table = m_model.toStrongRef().data()->ModelTable()->PrepareMC(QVector<double>() << m_model.toStrongRef().data()->StdDeviation(), rng);
        data["dependent"] = table->ExportTable(true);
        data["content"] = QString("Simulated data from a %1 with standard normal distributed random error with %2 %3 N(0,%4=%5<sub>fit</sub>=%6)").arg(m_model.toStrongRef().data()->Name()).arg(Unicode_epsilion).arg(Unicode_Math_Element).arg(Unicode_sigma).arg(Unicode_sigma).arg(m_model.toStrongRef().data()->StdDeviation());
        top["data"] = data;
        delete table;
    } else if (m_type == 3) // user defined sigma
    {
        qint64 seed = QDateTime::currentMSecsSinceEpoch();
        std::mt19937 rng(seed);

        DataTable* table = m_model.toStrongRef().data()->ModelTable()->PrepareMC(QVector<double>() << m_stdev, rng);
        data["dependent"] = table->ExportTable(true);
        data["content"] = QString("Simulated data from a %1 with standard normal distributed random error with %2 %3 N(0,%4=%5)").arg(m_model.toStrongRef().data()->Name()).arg(Unicode_epsilion).arg(Unicode_Math_Element).arg(Unicode_sigma).arg(m_stdev);
        top["data"] = data;
        delete table;
    } else if (m_type == 4) // Bootstrapping
    {
        QVector<qreal> vector = m_model.toStrongRef().data()->ErrorVector();
        auto Uni = std::uniform_int_distribution<int>(0, vector.size() - 1);

        qint64 seed = QDateTime::currentMSecsSinceEpoch();
        std::mt19937 rng(seed);

        DataTable* table = m_model.toStrongRef().data()->ModelTable()->PrepareBootStrap(Uni, rng, vector);
        data["dependent"] = table->ExportTable(true);
        data["content"] = QString("Simulated data from a %1 using Bootstrapping").arg(m_model.toStrongRef().data()->Name());
        top["data"] = data;
        delete table;
    }
    data["uuid"] = QString();

    top["data"] = data;
    QJsonDocument document(top);
    m_content = document.toJson();
}

ExportSimulationWidget::ExportSimulationWidget(QWeakPointer<AbstractModel> model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    QGridLayout* layout = new QGridLayout;

    m_std = new ClickLabel;
    connect(m_std, &ClickLabel::MouseClicked, this, [this]() {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText((tr("%1").arg(Print::printDouble(m_model.toStrongRef().data()->StdDeviation()))).simplified());
        m_std->Clicked();
    });

    m_sse = new ClickLabel;
    connect(m_sse, &ClickLabel::MouseClicked, this, [this]() {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText((tr("%1").arg(Print::printDouble(m_model.toStrongRef().data()->SSE()))).simplified());
        m_sse->Clicked();
    });

    m_sey = new ClickLabel;
    connect(m_sey, &ClickLabel::MouseClicked, this, [this]() {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText((tr("%1").arg(Print::printDouble(m_model.toStrongRef().data()->SEy()))).simplified());
        m_sey->Clicked();
    });
    if (!m_model.toStrongRef().data()->isSimulation()) {
        layout->addWidget(m_sse, 0, 0);
        layout->addWidget(m_std, 0, 1);
        layout->addWidget(m_sey, 0, 2);
    }

    m_ideal = new DnDLabel(tr("<img src='%1' height='18'></img>&emsp;<b> &emsp;Ideal Model</b>").arg(QString(":/icons/edit-copy.png")), 0, m_model);
    m_mc_std = new DnDLabel(tr("<img src='%1' height='18'></img>&emsp;<b> &emsp;MC from Model &sigma;<sub>fit</sub></b>").arg(QString(":/icons/edit-copy.png")), 2, m_model);
    m_mc_std->setParent(this);
    m_mc_sey = new DnDLabel(tr("<img src='%1' height='18'></img>&emsp;<b> &emsp;MC from Model SE<sub>y</sub></b>").arg(QString(":/icons/edit-copy.png")), 1, m_model);
    m_mc_sey->setParent(this);
    m_bs = new DnDLabel(tr("<img src='%1' height='18'></img>&emsp;<b> &emsp;MC using BS</b>").arg(QString(":/icons/edit-copy.png")), 4, m_model);
    m_bs->setParent(this);
    m_mc_user = new DnDLabel(tr("<img src='%1' height='18'></img>&emsp;<b> &emsp;MC from User &sigma;</b>").arg(QString(":/icons/edit-copy.png")), 3, m_model);

    m_variance = new QDoubleSpinBox;
    m_variance->setMinimum(0);
    m_variance->setMaximum(1e6);
    m_variance->setDecimals(5);
    m_variance->setValue(1e-3);

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(m_ideal);
    hlayout->addWidget(m_mc_std);
    hlayout->addWidget(m_mc_sey);
    hlayout->addWidget(m_bs);

    m_mc_std->setHidden(m_model.toStrongRef().data()->isSimulation());
    m_mc_sey->setHidden(m_model.toStrongRef().data()->isSimulation());
    m_bs->setHidden(m_model.toStrongRef().data()->isSimulation());
    hlayout->addWidget(m_mc_user);
    hlayout->addWidget(m_variance);
    layout->addLayout(hlayout, 1, 0, 1, 3);

    connect(m_variance, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        m_mc_user->setStd(value);
    });

    setLayout(layout);

    connect(m_model.toStrongRef().data(), &AbstractModel::Recalculated, this, [this]() {
        m_sse->setText(tr("<img src='%1' height='18'></img>&emsp;<b> &emsp;SSE: %2</b>").arg(QString(":/icons/edit-copy.png")).arg(Print::printDouble(m_model.toStrongRef().data()->SSE())));
        m_std->setText(tr("<img src='%1' height='18'></img>&emsp;<b> &emsp;%2: %3</b>").arg(QString(":/icons/edit-copy.png")).arg(Unicode_sigma).arg(Print::printDouble(m_model.toStrongRef().data()->StdDeviation())));
        m_sey->setText(tr("<img src='%1' height='18'></img>&emsp;<b> &emsp;SE<sub>y</sub>: %2</b>").arg(QString(":/icons/edit-copy.png")).arg(Print::printDouble(m_model.toStrongRef().data()->SEy())));
    });
    connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, this, &ExportSimulationWidget::UpdateVisibility);

    UpdateVisibility();
}
void ExportSimulationWidget::UpdateVisibility()
{
    m_ideal->setVisible(qApp->instance()->property("advanced_ui").toBool());
    m_mc_std->setVisible(qApp->instance()->property("advanced_ui").toBool());
    m_mc_sey->setVisible(qApp->instance()->property("advanced_ui").toBool());
    m_mc_user->setVisible(qApp->instance()->property("advanced_ui").toBool());
    m_variance->setVisible(qApp->instance()->property("advanced_ui").toBool());
}
