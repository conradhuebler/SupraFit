/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/dialogs/regressionanalysisdialog.h"
#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/widgets/signalelement.h"
#include "src/ui/widgets/systemparameterwidget.h"

#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTextEdit>

#include <QtCharts/QXYSeries>

#include "datawidget.h"

DataWidget::DataWidget()
    : m_system_parameter_loaded(false)
{
    m_widget = new QWidget;
    layout = new QGridLayout;
    m_switch = new QPushButton(tr("1<=>2"));
    m_switch->setToolTip(tr("<html>Swap the independent variables, 1	&harr;2</html>"));
    m_switch->setStyleSheet("background-color: #77d740;");
    //m_switch->setMaximumSize(100, 30);
    connect(m_switch, SIGNAL(clicked()), this, SLOT(switchHG()));

    m_linear = new QPushButton(tr("Regression"));
    m_linear->setToolTip(tr("Perform advanced linear regression"));
    m_linear->setStyleSheet("background-color: #77d740;");
    connect(m_linear, &QPushButton::clicked, this, &DataWidget::LinearAnalysis);

    m_hide_points = new QPushButton(tr("Hide Datapoints"));
    m_hide_points->setToolTip(tr("Hide Data points, but keep series visible. Useful to just simulate curves without being disturbed by Data Points."));
    m_hide_points->setStyleSheet("background-color: #77d740;");

    m_plot_x = new QCheckBox(tr("Plot X Values"));
    m_plot_x->setToolTip(tr("Plot first column as x variable."));

    m_name = new QLineEdit();
    connect(m_name, SIGNAL(textEdited(QString)), this, SLOT(SetProjectName()));
    m_concentrations = new QTableView;
    m_signals = new QTableView;
    m_signals->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_signals->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_signals, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));
    connect(m_concentrations->verticalScrollBar(), SIGNAL(valueChanged(int)), m_signals->verticalScrollBar(), SLOT(setValue(int)));
    QHBoxLayout* hlayout = new QHBoxLayout;

    hlayout->addWidget(new QLabel(tr("<html><h3>Project Name</h3></html>")), 0, Qt::AlignLeft);
    hlayout->addWidget(m_name);
    hlayout->addWidget(m_plot_x, 0, Qt::AlignRight);
    hlayout->addWidget(m_hide_points);
    hlayout->addWidget(m_switch, 0, Qt::AlignRight);
    hlayout->addWidget(m_linear, 0, Qt::AlignRight);

    m_datapoints = new QLabel;
    m_substances = new QLabel;
    m_const_subs = new QLabel;
    m_signals_count = new QLabel;
    m_tables = new QWidget; //(tr("Data Tables"));
    QHBoxLayout* group_layout = new QHBoxLayout;
    group_layout->addWidget(m_concentrations);
    group_layout->addWidget(m_signals);
    m_tables->setLayout(group_layout);

    m_text_edit = new QTextEdit;
    m_text_edit->setPlaceholderText(tr("Some information and description to that data set are welcome."));

    layout->addLayout(hlayout, 0, 0, 1, 4);
    layout->addWidget(m_text_edit, 1, 0, 1, 4);

    layout->addWidget(m_datapoints, 2, 0);
    layout->addWidget(m_substances, 2, 1);
    layout->addWidget(m_const_subs, 2, 2);
    layout->addWidget(m_signals_count, 2, 3);

    m_widget->setLayout(layout);
    QScrollArea* area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setWidget(m_widget);
    m_splitter = new QSplitter;
    m_splitter->setOrientation(Qt::Vertical);
    m_splitter->addWidget(area);

    hlayout = new QHBoxLayout;
    hlayout->addWidget(m_splitter);
    setLayout(hlayout);
}

DataWidget::~DataWidget()
{
    QSettings settings;
    settings.beginGroup("overview");
    settings.setValue("splitterSizes", m_splitter->saveState());
}

void DataWidget::setData(QWeakPointer<DataClass> dataclass, QWeakPointer<ChartWrapper> wrapper)
{
    m_data = dataclass;
    m_wrapper = wrapper;

    dialog = new RegressionAnalysisDialog(m_data, m_wrapper, this);
    m_concentrations->setModel(m_data.toStrongRef().data()->IndependentModel());

    if (!m_data.toStrongRef().data()->isSimulation())
        m_signals->setModel(m_data.toStrongRef().data()->DependentModel());
    else
        m_linear->hide();

    connect(m_data.toStrongRef().data()->DependentModel(), SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(HidePoint()));
    m_concentrations->resizeColumnsToContents();
    m_signals->resizeColumnsToContents();

    m_plot_x->setVisible(m_data.toStrongRef().data()->IndependentModel()->columnCount() == 1);
    m_plot_x->setChecked(m_data.toStrongRef().data()->PlotMode());
    connect(m_plot_x, &QCheckBox::stateChanged, this, [this](int state) {
        m_data.toStrongRef().data()->setPlotMode(state);
        m_wrapper.toStrongRef().data()->UpdateModel();
        dialog->UpdatePlots();
    });

    qApp->instance()->setProperty("projectname", m_data.toStrongRef().data()->ProjectTitle());
    m_name->setText(qApp->instance()->property("projectname").toString());
    m_substances->setText(tr("<html><h4>Independent Variables: %1</h4><html>").arg(m_data.toStrongRef().data()->IndependentModel()->columnCount()));
    m_datapoints->setText(tr("<html><h4>Data Points: %1</h4><html>").arg(m_data.toStrongRef().data()->DependentModel()->rowCount()));
    m_signals_count->setText(tr("<html><h4>Series Count: %1</h4><html>").arg(m_data.toStrongRef().data()->SeriesCount()));

    // why was this here?
    // dialog = new RegressionAnalysisDialog(m_data, m_wrapper, this);
    dialog->UpdatePlots();

    QVBoxLayout* vlayout = new QVBoxLayout;
    for (int i = 0; i < m_wrapper.toStrongRef().data()->SeriesSize(); ++i) {
        QPointer<SignalElement> el = new SignalElement(m_data, m_wrapper, i, this);
        connect(m_wrapper.toStrongRef().data()->Series(i), &QtCharts::QAbstractSeries::visibleChanged, dialog, &RegressionAnalysisDialog::UpdatePlots);
        connect(m_hide_points, &QPushButton::clicked, el, &SignalElement::HideSeries);
        vlayout->addWidget(el);
        if (m_data.toStrongRef().data()->Type() == DataClassPrivate::DataType::Simulation)
            el->HideSeries();
        m_signal_elements << el;
    }

    layout->addLayout(vlayout, 3, 0, 1, 4);

    QHBoxLayout* scaling_layout = new QHBoxLayout;
    scaling_layout->addWidget(new QLabel(tr("Scaling factors for input data:")));
    for (int i = 0; i < m_data.toStrongRef().data()->getScaling().size(); ++i) {
        QDoubleSpinBox* spin_box = new QDoubleSpinBox;
        spin_box->setMaximum(1e8);
        spin_box->setMinimum(-1e8);
        spin_box->setValue(m_data.toStrongRef().data()->getScaling()[i]);
        spin_box->setSingleStep(1e-2);
        spin_box->setDecimals(7);
        connect(spin_box, SIGNAL(valueChanged(double)), this, SLOT(setScaling()));
        m_scaling_boxes << spin_box;
        QHBoxLayout* lay = new QHBoxLayout;
        lay->addWidget(new QLabel(tr("%1. substance").arg(i + 1)));
        lay->addWidget(spin_box);
        scaling_layout->addLayout(lay);
    }

    layout->addLayout(scaling_layout, 4, 0, 1, 4);

    if (m_data.toStrongRef().data()->IndependentVariableSize() == 1)
        m_switch->hide();
    m_splitter->addWidget(m_tables);

    QSettings settings;
    settings.beginGroup("overview");
    m_splitter->restoreState(settings.value("splitterSizes").toByteArray());
    m_switch->setVisible(m_data.toStrongRef().data()->IndependentVariableSize() == 2);
    connect(m_data.toStrongRef().data(), &DataClass::ProjectTitleChanged, m_name, [this](const QString& str) {
        if (str == m_name->text())
            return;
        m_name->setText(str);
    });
    m_text_edit->append(m_data.toStrongRef().data()->Content());
    connect(m_text_edit, &QTextEdit::textChanged, m_data.toStrongRef().data(), [this]() {
        m_data.toStrongRef().data()->setContent(m_text_edit->toPlainText());
    });

    if (m_data.toStrongRef().data()->Type() == DataClassPrivate::DataType::Simulation) {
        m_hide_points->hide();
    }

    connect(m_wrapper.toStrongRef().data(), &ChartWrapper::ModelTransformed, m_wrapper.toStrongRef().data(), [this]() {
        if (m_plot_x->isVisible())
            m_plot_x->hide();
    });
}

void DataWidget::switchHG()
{
    m_data.toStrongRef().data()->SwitchConentrations();
    m_wrapper.toStrongRef().data()->UpdateModel();
    emit recalculate();
}

void DataWidget::SetProjectName()
{
    qApp->instance()->setProperty("projectname", m_name->text());
    m_data.toStrongRef().data()->setProjectTitle(m_name->text());
    emit NameChanged();
}

void DataWidget::setEditable(bool editable)
{
    m_data.toStrongRef().data()->DependentModel()->setEditable(editable);
}

void DataWidget::setScaling()
{
    QList<qreal> scaling;
    for (int i = 0; i < m_scaling_boxes.size(); ++i)
        scaling << m_scaling_boxes[i]->value();
    m_data.toStrongRef().data()->setScaling(scaling);
    m_wrapper.toStrongRef().data()->UpdateModel();
    emit recalculate();
}

void DataWidget::ShowContextMenu(const QPoint& pos)
{
    Q_UNUSED(pos)
    QModelIndex index = m_signals->currentIndex();
    int row = index.row();
    m_data.toStrongRef().data()->DependentModel()->CheckRow(row);
    HidePoint();
    emit recalculate();
}

void DataWidget::HidePoint()
{
    m_wrapper.toStrongRef().data()->stopAnimiation();
    m_wrapper.toStrongRef().data()->UpdateModel();
    m_wrapper.toStrongRef().data()->restartAnimation();
    dialog->UpdatePlots();
}


void DataWidget::LinearAnalysis()
{
    dialog->show();
}

#include "datawidget.moc"
