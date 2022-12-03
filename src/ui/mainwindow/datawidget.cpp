/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/ui/guitools/guitools.h"

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
    m_layout = new QGridLayout;

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
    m_concentrations->setContextMenuPolicy(Qt::ActionsContextMenu);

    QHBoxLayout* hlayout = new QHBoxLayout;

    hlayout->addWidget(new QLabel(tr("<html><h3>Project Name</h3></html>")), 0, Qt::AlignLeft);
    hlayout->addWidget(m_name);
    hlayout->addWidget(m_plot_x, 0, Qt::AlignRight);
    hlayout->addWidget(m_hide_points);
    hlayout->addWidget(m_linear, 0, Qt::AlignRight);

    m_datapoints = new QLabel;
    m_substances = new QLabel;
    m_const_subs = new QLabel;
    m_signals_count = new QLabel;

    m_x_model = new QLineEdit;
    m_y_model = new QLineEdit;

    m_x_string = new QLabel;
    m_x_string->setFixedWidth(200);

    m_y_string = new QLabel;
    m_y_string->setFixedWidth(200);

    m_x_raw = new QCheckBox(tr("Show Raw Data"));
    m_y_raw = new QCheckBox(tr("Show Raw Data"));

    m_tables = new QWidget; //(tr("Data Tables"));

    m_range = new QLabel(tr("All data are included!"));
    m_shift_begin = new QPushButton(tr("Shift begin data"));
    m_shift_begin->setDisabled(true);
    // m_shift_begin->setFlat(true);

    m_tables_layout = new QGridLayout;

    m_tables_layout->addWidget(m_range, 0, 0, 1, 4);
    m_tables_layout->addWidget(m_shift_begin, 0, 4, 1, 2);

    m_tables_layout->addWidget(m_x_raw, 1, 0);
    m_tables_layout->addWidget(m_x_string, 1, 1);
    m_tables_layout->addWidget(m_x_model, 1, 2);

    m_tables_layout->addWidget(m_y_raw, 1, 3);
    m_tables_layout->addWidget(m_y_string, 1, 4);
    m_tables_layout->addWidget(m_y_model, 1, 5);

    m_tables_layout->addWidget(m_concentrations, 2, 0, 1, 3);
    m_tables_layout->addWidget(m_signals, 2, 3, 1, 3);
    m_tables->setLayout(m_tables_layout);

    m_text_edit = new QTextEdit;
    m_text_edit->setPlaceholderText(tr("Some information and description to that data set are welcome."));

    m_layout->addLayout(hlayout, 0, 0, 1, 4);

    m_layout->addWidget(m_datapoints, 1, 0);
    m_layout->addWidget(m_substances, 1, 1);
    m_layout->addWidget(m_const_subs, 1, 2);
    m_layout->addWidget(m_signals_count, 1, 3);

    m_series_scroll_area = new QScrollArea;
    m_series_scroll_area->setWidgetResizable(true);
    m_series_scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_widget->setLayout(m_layout);
    QScrollArea* area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setWidget(m_widget);
    m_splitter = new QSplitter;
    m_splitter->setOrientation(Qt::Vertical);
    m_splitter->addWidget(area);

    QTabWidget* tabWidget = new QTabWidget;
    tabWidget->addTab(m_tables, tr("Project Data"));
    tabWidget->addTab(m_text_edit, tr("Project Description"));

    m_layout->addWidget(tabWidget, 2, 0, 1, 4);

    setLayout(m_layout);
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

    m_shift_begin->setDisabled(false);

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
        // dialog->UpdatePlots();
    });

    qApp->instance()->setProperty("projectname", m_data.toStrongRef().data()->ProjectTitle());
    m_name->setText(qApp->instance()->property("projectname").toString());
    m_substances->setText(tr("<html><h4>Independent Variables: %1</h4><html>").arg(m_data.toStrongRef().data()->IndependentModel()->columnCount()));
    m_datapoints->setText(tr("<html><h4>Data Points: %1</h4><html>").arg(m_data.toStrongRef().data()->DependentModel()->rowCount()));
    m_signals_count->setText(tr("<html><h4>Series Count: %1</h4><html>").arg(m_data.toStrongRef().data()->SeriesCount()));

    // why was this here?
    // dialog = new RegressionAnalysisDialog(m_data, m_wrapper, this);
    // dialog->UpdatePlots();

    QVBoxLayout* vlayout = new QVBoxLayout;
    for (int i = 0; i < m_wrapper.toStrongRef().data()->SeriesSize(); ++i) {
        QPointer<SignalElement> el = new SignalElement(m_data, m_wrapper, i, this);
        connect(m_wrapper.toStrongRef().data()->Series(i), &QAbstractSeries::visibleChanged, dialog, &RegressionAnalysisDialog::UpdatePlots);
        connect(m_hide_points, &QPushButton::clicked, el, &SignalElement::HideSeries);
        vlayout->addWidget(el);
        if (m_data.toStrongRef().data()->Type() == DataClassPrivate::DataType::Simulation)
            el->HideSeries();
        m_signal_elements << el;
    }
    QWidget* scrollHolder = new QWidget;
    scrollHolder->setLayout(vlayout);

    m_series_scroll_area->setWidget(scrollHolder);

    //m_layout->addWidget(m_series_scroll_area, 4, 0, 1, 4);
    m_layout->addWidget(m_series_scroll_area, 3, 0, 1, 4);

    QSettings settings;
    settings.beginGroup("overview");
    m_splitter->restoreState(settings.value("splitterSizes").toByteArray());
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

    connect(m_x_raw, &QCheckBox::stateChanged, this, [this]() {
        if (m_x_raw->isChecked())
            m_concentrations->setModel(m_data.toStrongRef().data()->IndependentRawModel());
        else
            m_concentrations->setModel(m_data.toStrongRef().data()->IndependentModel());
    });

    connect(m_y_raw, &QCheckBox::stateChanged, this, [this]() {
        if (m_y_raw->isChecked())
            m_signals->setModel(m_data.toStrongRef().data()->DependentRawModel());
        else
            m_signals->setModel(m_data.toStrongRef().data()->DependentModel());
    });

    connect(m_concentrations->horizontalHeader(), &QHeaderView::sectionDoubleClicked, this, [this](int i) {
        m_index_x = i;
        QString eq = m_data.toStrongRef().data()->IndependentRawModel()->header()[i];
        m_x_model->setText(eq);
        m_x_string->setText(QString("Working on column %1").arg(i + 1));
    });

    connect(m_signals->horizontalHeader(), &QHeaderView::sectionDoubleClicked, this, [this](int i) {
        m_index_y = i;
        QString eq = m_data.toStrongRef().data()->DependentRawModel()->header()[i];
        m_y_model->setText(eq);
        m_y_string->setText(QString("Working on column %1").arg(i + 1));
    });

    connect(m_x_model, &QLineEdit::textEdited, this, [this](const QString& string) {
        if (m_index_x == -1)
            return;
        QStringList header = m_data.toStrongRef().data()->IndependentRawModel()->header();
        header[m_index_x] = string;
        m_data.toStrongRef().data()->IndependentRawModel()->setHeader(header);
        m_data.toStrongRef().data()->ApplyCalculationModel();
    });

    connect(m_y_model, &QLineEdit::textEdited, this, [this](const QString& string) {
        if (m_index_y == -1)
            return;
        QStringList header = m_data.toStrongRef().data()->DependentRawModel()->header();
        header[m_index_y] = string;
        m_data.toStrongRef().data()->DependentRawModel()->setHeader(header);
        m_data.toStrongRef().data()->ApplyCalculationModel();
    });

    QAction* beginDataAction = new QAction(tr("Begin Data"));
    beginDataAction->setToolTip(tr("Set the first row to be included in data."));
    beginDataAction->setIcon(Icon("go-top"));
    m_concentrations->addAction(beginDataAction);

    connect(beginDataAction, &QAction::triggered, beginDataAction, [this]() {
        QModelIndex index = m_concentrations->currentIndex();
        int i = index.row();
        if (i >= m_data.toStrongRef().data()->DataPoints())
            return;
        m_data.toStrongRef().data()->setDataBegin(i);
        UpdateRanges();
    });

    QAction* endDataAction = new QAction(tr("End Data"));
    endDataAction->setToolTip(tr("Set the last row to be included in data."));
    endDataAction->setIcon(Icon("go-down"));
    m_concentrations->addAction(endDataAction);

    connect(endDataAction, &QAction::triggered, endDataAction, [this]() {
        QModelIndex index = m_concentrations->currentIndex();
        int i = index.row() + 1;
        // qDebug() << i;
        if (i <= 0)
            return;
        m_data.toStrongRef().data()->setDataEnd(i);
        UpdateRanges();
    });

    connect(m_shift_begin, &QPushButton::clicked, this, [this]() {
        int begin = m_data.toStrongRef().data()->DataBegin();
        double x0 = m_data.toStrongRef().data()->IndependentRawModel()->data(begin);
        QStringList header = m_data.toStrongRef().data()->IndependentRawModel()->header();
        if (header.size() != 1)
            return;
        header[0] = QString("X1 - %1").arg(x0);
        m_data.toStrongRef().data()->IndependentRawModel()->setHeader(header);
        m_data.toStrongRef().data()->ApplyCalculationModel();
    });
    connect(m_data.toStrongRef().data(), &DataClass::DataRangedChanged, this, &DataWidget::UpdateRanges);
    UpdateRanges();
}

void DataWidget::UpdateRanges()
{
    if (!m_data)
        return;
    int begin = m_data.toStrongRef().data()->DataBegin();
    int end = m_data.toStrongRef().data()->DataEnd() - 1;
    qDebug() << begin << end;
    double x0 = m_data.toStrongRef().data()->IndependentModel()->data(begin);
    double x1 = m_data.toStrongRef().data()->IndependentModel()->data(end);
    double y0 = m_data.toStrongRef().data()->DependentModel()->data(begin);
    double y1 = m_data.toStrongRef().data()->DependentModel()->data(end);

    m_range->setText(QString("Data begin with index %1 (X1 = %2, Y1 = %3) and end with index %4 (X1 = %5, Y1 = %6)").arg(begin).arg(x0).arg(y0).arg(end).arg(x1).arg(y1));

    m_wrapper.toStrongRef().data()->stopAnimiation();
    m_wrapper.toStrongRef().data()->UpdateModel();
    m_wrapper.toStrongRef().data()->restartAnimation();
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
    dialog->UpdatePlots();
}

#include "datawidget.moc"
