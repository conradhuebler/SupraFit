/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global_config.h"

#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFontDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

#include <QtCharts/QChartView>

#include "chartconfig.h"

ChartConfigDialog::ChartConfigDialog(QWidget* widget)
    : QDialog(widget)
{
    setModal(false);
    QGridLayout* layout = new QGridLayout;

    m_title = new QLineEdit;
    connect(m_title, &QLineEdit::textChanged, this, &ChartConfigDialog::Changed);
    m_title->setMinimumWidth(400);
    m_titlefont = new QPushButton(tr("Font"));
    connect(m_titlefont, &QPushButton::clicked, this, &ChartConfigDialog::setTitleFont);
    m_titlefont->setMaximumSize(60, 30);
    m_titlefont->setStyleSheet("background-color: #F3ECE0;");
    m_x_axis = new QLineEdit;
    connect(m_x_axis, SIGNAL(textChanged(QString)), this, SLOT(Changed()));

    m_y_axis = new QLineEdit;
    connect(m_y_axis, SIGNAL(textChanged(QString)), this, SLOT(Changed()));

    m_x_min = new QDoubleSpinBox;
    connect(m_x_min, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));
    m_x_min->setMinimum(-1 * 1e14);
    m_x_min->setMaximum(1e17);
    m_x_min->setDecimals(5);

    m_x_max = new QDoubleSpinBox;
    connect(m_x_max, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));
    m_x_max->setMinimum(-1 * 1e14);
    m_x_max->setMaximum(1e17);
    m_x_max->setDecimals(5);

    m_x_step = new QSpinBox;
    connect(m_x_step, SIGNAL(valueChanged(int)), this, SLOT(Changed()));

    m_y_min = new QDoubleSpinBox;
    m_y_min->setMinimum(-1 * 1e14);
    m_y_min->setMaximum(1e17);
    m_y_min->setSingleStep(1);
    m_y_min->setDecimals(5);
    connect(m_y_min, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));

    m_y_max = new QDoubleSpinBox;
    m_y_max->setMinimum(-1 * 1e14);
    m_y_max->setMaximum(1 * 1e17);
    m_y_max->setDecimals(5);
    connect(m_y_max, SIGNAL(valueChanged(qreal)), this, SLOT(Changed()));

    m_y_step = new QSpinBox;
    connect(m_y_step, SIGNAL(valueChanged(int)), this, SLOT(Changed()));

    m_scaleaxis = new QPushButton(tr("Reset Scaling"));
    connect(m_scaleaxis, SIGNAL(clicked()), this, SIGNAL(ScaleAxis()));
    m_scaleaxis->setMaximumSize(100, 30);
    m_scaleaxis->setStyleSheet("background-color: #F3ECE0;");

    m_resetFontConfig = new QPushButton(tr("Reset Font Config"));
    m_resetFontConfig->setStyleSheet("background-color: #F3ECE0;");
    connect(m_resetFontConfig, &QPushButton::clicked, m_resetFontConfig, [this]() {
        QMessageBox::StandardButton replay;
        QString message;
#ifdef noto_font
        message = "Fonts will be set Google Noto Font!";
#else
        message = "Fonts will be set to your systems standard font configuration!";
#endif
        replay = QMessageBox::information(this, tr("Reset Font Config."), tr("Do you really want to reset the current font config?\n%1").arg(message), QMessageBox::Yes | QMessageBox::No);
        if (replay == QMessageBox::Yes) {
            emit this->ResetFontConfig();
        }
    });

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &ChartConfigDialog::Changed);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    m_legend = new QCheckBox(tr("Show Legend"));
    connect(m_legend, &QCheckBox::stateChanged, this, &ChartConfigDialog::Changed);

    m_lock_scaling = new QCheckBox(tr("Lock Scaling"));
    connect(m_lock_scaling, &QCheckBox::stateChanged, this, &ChartConfigDialog::Changed);

    m_annotation = new QCheckBox(tr("In Chart Annotation"));
    m_annotation->setChecked(true);
    connect(m_annotation, &QCheckBox::stateChanged, this, &ChartConfigDialog::Changed);

    m_theme = new QComboBox;
    m_theme->addItem("Light", QtCharts::QChart::ChartThemeLight);
    m_theme->addItem("Blue Cerulean", QtCharts::QChart::ChartThemeBlueCerulean);
    m_theme->addItem("Dark", QtCharts::QChart::ChartThemeDark);
    m_theme->addItem("Brown Sand", QtCharts::QChart::ChartThemeBrownSand);
    m_theme->addItem("Blue NCS", QtCharts::QChart::ChartThemeBlueNcs);
    m_theme->addItem("High Contrast", QtCharts::QChart::ChartThemeHighContrast);
    m_theme->addItem("Blue Icy", QtCharts::QChart::ChartThemeBlueIcy);
    m_theme->addItem("Qt", QtCharts::QChart::ChartThemeQt);
    m_theme->addItem("Black 'n' White", 8);
    connect(m_theme, qOverload<int>(&QComboBox::currentIndexChanged), this, &ChartConfigDialog::Changed);

    layout->addWidget(new QLabel(tr("<h4>Chart Title</h4>")), 0, 0);
    layout->addWidget(m_title, 0, 1);
    layout->addWidget(m_titlefont, 0, 2, Qt::AlignRight);
    layout->addWidget(m_theme, 0, 3, Qt::AlignRight);

    QGroupBox* box = new QGroupBox(tr("X Axis"));
    QGridLayout* grid = new QGridLayout;
    grid->addWidget(new QLabel(tr("<h5>Label:</h5>")), 0, 0);
    grid->addWidget(m_x_axis, 0, 1, 1, 2);

    grid->addWidget(new QLabel(tr("<div align='center'><h5>Axis Range</h5></div>")), 1, 0, 1, 3);
    grid->addWidget(m_x_min, 2, 0);
    connect(m_x_min, qOverload<double>(&QDoubleSpinBox::valueChanged), m_lock_scaling, [this]() {
        this->m_lock_scaling->setChecked(true);
    });
    grid->addWidget(m_x_step, 2, 1);
    connect(m_x_step, qOverload<int>(&QSpinBox::valueChanged), m_lock_scaling, [this]() {
        this->m_lock_scaling->setChecked(true);
    });
    grid->addWidget(m_x_max, 2, 2);
    connect(m_x_max, qOverload<double>(&QDoubleSpinBox::valueChanged), m_lock_scaling, [this]() {
        this->m_lock_scaling->setChecked(true);
    });

    box->setLayout(grid);

    layout->addWidget(box, 2, 0, 1, 4);

    box = new QGroupBox(tr("Y Axix"));
    grid = new QGridLayout;
    grid->addWidget(new QLabel(tr("<h5>Label:</h5>")), 0, 0);
    grid->addWidget(m_y_axis, 0, 1, 1, 2);

    grid->addWidget(new QLabel(tr("<div align='center'><h5>Axis Range</h5></div>")), 1, 0, 1, 3);
    grid->addWidget(m_y_min, 2, 0);
    connect(m_y_min, qOverload<double>(&QDoubleSpinBox::valueChanged), m_lock_scaling, [this]() {
        this->m_lock_scaling->setChecked(true);
    });
    grid->addWidget(m_y_step, 2, 1);
    connect(m_y_step, qOverload<int>(&QSpinBox::valueChanged), m_lock_scaling, [this]() {
        this->m_lock_scaling->setChecked(true);
    });
    grid->addWidget(m_y_max, 2, 2);
    connect(m_y_max, qOverload<double>(&QDoubleSpinBox::valueChanged), m_lock_scaling, [this]() {
        this->m_lock_scaling->setChecked(true);
    });
    box->setLayout(grid);

    layout->addWidget(box, 3, 0, 1, 4);

    layout->addWidget(m_scaleaxis, 5, 1);
    layout->addWidget(m_lock_scaling, 5, 2);

    m_labels = new QPushButton(tr("Label Font"));
    connect(m_labels, &QPushButton::clicked, this, &ChartConfigDialog::setLabelFont);
    m_labels->setMaximumSize(90, 30);
    m_labels->setStyleSheet("background-color: #F3ECE0;");

    m_keys = new QPushButton(tr("Legend Font"));
    connect(m_keys, &QPushButton::clicked, this, &ChartConfigDialog::setKeysFont);
    m_keys->setMaximumSize(90, 30);
    m_keys->setStyleSheet("background-color: #F3ECE0;");

    m_ticks = new QPushButton(tr("Ticks Font"));
    connect(m_ticks, &QPushButton::clicked, this, &ChartConfigDialog::setTicksFont);
    m_ticks->setMaximumSize(90, 30);
    m_ticks->setStyleSheet("background-color: #F3ECE0;");

    m_alignment = new QPushButton(tr("Align Legend"));
    m_alignment->setMaximumSize(130, 30);
    m_alignment->setStyleSheet("background-color: #F3ECE0;");

    m_show_axis = new QCheckBox(tr("Show Axis"));
    m_show_axis->setChecked(true);

    QMenu* align = new QMenu(this);
    QAction* left = new QAction(tr("Left"), this);
    left->setData(Qt::AlignLeft);
    align->addAction(left);

    QAction* right = new QAction(tr("Right"), this);
    right->setData(Qt::AlignRight);
    align->addAction(right);

    QAction* top = new QAction(tr("Top"), this);
    top->setData(Qt::AlignTop);
    align->addAction(top);

    QAction* bottom = new QAction(tr("Bottom"), this);
    bottom->setData(Qt::AlignBottom);
    align->addAction(bottom);
    m_alignment->setMenu(align);

    connect(align, &QMenu::triggered, [this](QAction* action) {
        m_chartconfig.align = Qt::Alignment(action->data().toInt());
        emit ConfigChanged(Config());
    });

    QHBoxLayout* actions = new QHBoxLayout;

    actions->addWidget(m_labels);
    actions->addWidget(m_ticks);
    actions->addStretch(300);
    actions->addWidget(m_keys);
    actions->addWidget(m_alignment);
    actions->addWidget(m_legend);
    actions->addWidget(m_annotation);
    actions->addWidget(m_show_axis);
    layout->addLayout(actions, 6, 0, 1, 3);

    m_x_size = new QSpinBox;
    m_x_size->setRange(0, 1e6);
    m_x_size->setValue(qApp->instance()->property("xSize").toInt());

    m_y_size = new QSpinBox;
    m_y_size->setRange(0, 1e6);
    m_y_size->setValue(qApp->instance()->property("ySize").toInt());

    m_scaling = new QSpinBox;
    m_scaling->setRange(0, 1e6);
    m_scaling->setValue(qApp->instance()->property("chartScaling").toInt());

    m_markerSize = new QDoubleSpinBox;
    m_markerSize->setRange(0, 30);
    m_markerSize->setValue(qApp->instance()->property("markerSize").toDouble());

    m_lineWidth = new QDoubleSpinBox;
    m_lineWidth->setRange(0, 30);
    m_lineWidth->setValue(qApp->instance()->property("lineWidth").toDouble());

    actions = new QHBoxLayout;
    actions->addWidget(new QLabel(tr("X Size:")));
    actions->addWidget(m_x_size);

    actions->addWidget(new QLabel(tr("Y Size:")));
    actions->addWidget(m_y_size);

    actions->addWidget(new QLabel(tr("Scaling:")));
    actions->addWidget(m_scaling);

    actions->addWidget(new QLabel(tr("Marker Size:")));
    actions->addWidget(m_markerSize);

    actions->addWidget(new QLabel(tr("Line Width:")));
    actions->addWidget(m_lineWidth);

    layout->addLayout(actions, 7, 0, 1, 3);

    layout->addWidget(m_resetFontConfig, 10, 0);

    layout->addWidget(m_buttons, 10, 1, 1, 2);
    setLayout(layout);
    setWindowTitle("Configure charts ...");
}

ChartConfigDialog::~ChartConfigDialog()
{
}

void ChartConfigDialog::setConfig(const ChartConfig& chartconfig)
{
    const QSignalBlocker blocker(this);
    m_x_min->setValue(chartconfig.x_min);
    m_x_max->setValue(chartconfig.x_max);
    m_x_step->setValue(chartconfig.x_step);
    m_y_min->setValue(chartconfig.y_min);
    m_y_max->setValue(chartconfig.y_max);
    m_y_step->setValue(chartconfig.y_step);

    m_x_axis->setText(chartconfig.x_axis);
    m_y_axis->setText(chartconfig.y_axis);
    m_legend->setChecked(chartconfig.m_legend);
    m_lock_scaling->setChecked(chartconfig.m_lock_scaling);

    m_theme->setCurrentIndex(chartconfig.Theme);

    m_title->setText(chartconfig.title);

    m_chartconfig = chartconfig;
}

void ChartConfigDialog::Changed()
{
    m_chartconfig.title = m_title->text();

    m_chartconfig.x_axis = m_x_axis->text();
    m_chartconfig.y_axis = m_y_axis->text();
    m_chartconfig.x_min = m_x_min->value();
    m_chartconfig.x_max = m_x_max->value();
    m_chartconfig.x_step = m_x_step->value();

    m_chartconfig.y_min = m_y_min->value();
    m_chartconfig.y_max = m_y_max->value();
    m_chartconfig.y_step = m_y_step->value();
    m_chartconfig.m_legend = m_legend->isChecked();
    m_chartconfig.m_lock_scaling = m_lock_scaling->isChecked();
    m_chartconfig.m_annotation = m_annotation->isChecked();

    m_chartconfig.x_size = m_x_size->value();
    m_chartconfig.y_size = m_y_size->value();
    m_chartconfig.scaling = m_scaling->value();
    m_chartconfig.lineWidth = m_lineWidth->value();
    m_chartconfig.markerSize = m_markerSize->value();

    m_chartconfig.showAxis = m_show_axis->isChecked();

    m_chartconfig.Theme = m_theme->currentIndex();

    emit ConfigChanged(Config());
}

void ChartConfigDialog::setKeysFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_chartconfig.m_keys, this);
    if (ok) {
        m_chartconfig.m_keys = font;
        emit ConfigChanged(Config());
    }
}

void ChartConfigDialog::setLabelFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_chartconfig.m_label, this);
    if (ok) {
        m_chartconfig.m_label = font;
        emit ConfigChanged(Config());
    }
}

void ChartConfigDialog::setTicksFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_chartconfig.m_ticks, this);
    if (ok) {
        m_chartconfig.m_ticks = font;
        emit ConfigChanged(Config());
    }
}

void ChartConfigDialog::setTitleFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_chartconfig.m_title, this);
    if (ok) {
        m_chartconfig.m_title = font;
        emit ConfigChanged(Config());
    }
}
#include "chartconfig.moc"
