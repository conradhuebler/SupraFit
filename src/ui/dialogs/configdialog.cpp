/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/widgets/buttons/scientificbox.h"
#include "src/ui/widgets/optimizerwidget.h"

#include <QtCharts/QChart>

#include <QtCore/QThread>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

#include "src/ui/settingsregistry.h"

#include "configdialog.h"

OptimizerDialog::OptimizerDialog(QJsonObject config, QWidget* parent)
    : QDialog(parent)
    , m_opt_config(config)
{
    setUi();
}

OptimizerDialog::~OptimizerDialog()
{
}

void OptimizerDialog::setUi()
{
    QVBoxLayout* mainlayout = new QVBoxLayout;

    setLayout(mainlayout);

    m_mainwidget = new QTabWidget;
    mainlayout->addWidget(m_mainwidget);

    createOptimTab();

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainlayout->addWidget(m_buttons);
}

void OptimizerDialog::createOptimTab()
{
    QVBoxLayout* layout = new QVBoxLayout;
    m_opt_widget = new OptimizerWidget(m_opt_config, this);
    m_opt_widget->setLayout(layout);

    m_mainwidget->addTab(m_opt_widget, tr("Optimizer Settings"));
}

ConfigDialog::ConfigDialog(QJsonObject config, QWidget* parent)
    : m_opt_config(config)
    , QDialog(parent)
{
    m_dirlevel = qApp->instance()->property("dirlevel").toInt();
    setUi();
    setWindowTitle("Configure");
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::setUi()
{
    QVBoxLayout* mainlayout = new QVBoxLayout;
    setLayout(mainlayout);

    m_mainwidget = new QTabWidget;
    mainlayout->addWidget(m_mainwidget);

    // One tab per distinct group, in registry declaration order; widgets are generated from the
    // registry so a new setting only needs a single table entry (see SupraFitSettings::registry).
    QStringList groups;
    for (const SupraFitSettings::SettingDef& d : SupraFitSettings::registry()) {
        if (d.inDialog && !d.group.isEmpty() && !groups.contains(d.group))
            groups << d.group;
    }
    for (const QString& group : groups)
        m_mainwidget->addTab(buildTab(group), group);

    createOptimTab();

    // Wire dependsOn enable-chains once every widget exists (controller may live in another tab).
    for (const SupraFitSettings::SettingDef& d : SupraFitSettings::registry()) {
        if (d.dependsOn.isEmpty())
            continue;
        QWidget* dependent = m_widgets.value(d.key, nullptr);
        QCheckBox* controller = qobject_cast<QCheckBox*>(m_widgets.value(d.dependsOn, nullptr));
        if (!dependent || !controller)
            continue;
        dependent->setEnabled(controller->isChecked());
        connect(controller, &QCheckBox::toggled, dependent, &QWidget::setEnabled);
    }

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainlayout->addWidget(m_buttons);
    setMinimumSize(600, 400);
}

QWidget* ConfigDialog::buildTab(const QString& group)
{
    QWidget* tab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;
    tab->setLayout(layout);
    QObject* app = qApp->instance();

    for (const SupraFitSettings::SettingDef& d : SupraFitSettings::registry()) {
        if (d.group != group || !d.inDialog)
            continue;

        if (!d.note.isEmpty())
            layout->addWidget(new QLabel(d.note));

        if (d.kind == SupraFitSettings::Kind::Custom) {
            if (d.key == QLatin1String("dirlevel"))
                buildDirectorySection(layout); // also emits the working-dir row
            continue;
        }

        const QVariant value = app->property(qPrintable(d.key));

        if (d.kind == SupraFitSettings::Kind::Bool) {
            QCheckBox* box = new QCheckBox(d.label);
            box->setChecked(value.toBool());
            if (!d.tooltip.isEmpty())
                box->setToolTip(d.tooltip);
            layout->addWidget(box);
            m_widgets.insert(d.key, box);
        } else if (d.kind == SupraFitSettings::Kind::Int) {
            QSpinBox* box = new QSpinBox;
            box->setRange(static_cast<int>(d.min), static_cast<int>(d.max));
            box->setValue(value.toInt());
            if (!d.suffix.isEmpty())
                box->setSuffix(d.suffix);
            if (!d.tooltip.isEmpty())
                box->setToolTip(d.tooltip);
            QHBoxLayout* h = new QHBoxLayout;
            h->addWidget(new QLabel(d.label));
            h->addWidget(box);
            layout->addLayout(h);
            m_widgets.insert(d.key, box);
        } else if (d.kind == SupraFitSettings::Kind::Double) {
            QDoubleSpinBox* box = new QDoubleSpinBox;
            box->setRange(d.min, d.max);
            if (d.step > 0)
                box->setSingleStep(d.step);
            if (d.decimals >= 0)
                box->setDecimals(d.decimals);
            box->setValue(value.toDouble());
            if (!d.suffix.isEmpty())
                box->setSuffix(d.suffix);
            if (!d.tooltip.isEmpty())
                box->setToolTip(d.tooltip);
            QHBoxLayout* h = new QHBoxLayout;
            h->addWidget(new QLabel(d.label));
            h->addWidget(box);
            layout->addLayout(h);
            m_widgets.insert(d.key, box);
        } else if (d.kind == SupraFitSettings::Kind::String) {
            QLineEdit* edit = new QLineEdit(value.toString());
            edit->setClearButtonEnabled(true);
            if (!d.tooltip.isEmpty())
                edit->setToolTip(d.tooltip);
            QHBoxLayout* h = new QHBoxLayout;
            h->addWidget(new QLabel(d.label));
            h->addWidget(edit);
            layout->addLayout(h);
            m_widgets.insert(d.key, edit);
        }
    }
    layout->addStretch();
    return tab;
}

void ConfigDialog::buildDirectorySection(QVBoxLayout* layout)
{
    QObject* app = qApp->instance();

    m_current_dir = new QRadioButton(tr("Current Directory, where Application was started"));
    m_current_dir->setChecked(m_dirlevel == 0);
    layout->addWidget(m_current_dir);

    m_last_dir = new QRadioButton(tr("Last Directory"));
    m_last_dir->setChecked(m_dirlevel == 1);
    layout->addWidget(m_last_dir);

    m_working_dir = new QRadioButton(tr("Set a working Directory"));
    m_working_dir->setChecked(m_dirlevel == 2);
    layout->addWidget(m_working_dir);

    m_working = new QLineEdit;
    m_working->setClearButtonEnabled(true);
    m_working->setText(app->property("workingdir").toString());
    m_select_working = new QPushButton(tr("Select"), this);
    connect(m_select_working, &QPushButton::clicked, this, &ConfigDialog::SelectWorkingDir);

    QHBoxLayout* h = new QHBoxLayout;
    h->addWidget(new QLabel(tr("Working Directory:")));
    h->addWidget(m_working);
    h->addWidget(m_select_working);
    layout->addLayout(h);
}

void ConfigDialog::createOptimTab()
{
    QVBoxLayout* layout = new QVBoxLayout;
    m_opt_widget = new OptimizerWidget(m_opt_config, this);
    m_opt_widget->setLayout(layout);

    m_mainwidget->addTab(m_opt_widget, tr("Optimizer Settings"));
}

void ConfigDialog::SelectWorkingDir()
{
    QString filename = QFileDialog::getExistingDirectory(this, "Choose Working Directory", qApp->instance()->property("workingdir").toString());
    if (filename.isEmpty())
        return;

    m_working->setText(filename);
}

void ConfigDialog::accept()
{
    if (m_current_dir->isChecked())
        m_dirlevel = 0;
    else if (m_last_dir->isChecked())
        m_dirlevel = 1;
    else if (m_working_dir->isChecked())
        m_dirlevel = 2;

    QObject* app = qApp->instance();

    // Custom (hand-built) settings.
    app->setProperty("dirlevel", m_dirlevel);
    app->setProperty("workingdir", m_working->text());

    // Generated settings: read each widget back into its qApp property by kind.
    for (const SupraFitSettings::SettingDef& d : SupraFitSettings::registry()) {
        QWidget* w = m_widgets.value(d.key, nullptr);
        if (!w)
            continue;
        if (QCheckBox* box = qobject_cast<QCheckBox*>(w))
            app->setProperty(qPrintable(d.key), box->isChecked());
        else if (QSpinBox* box = qobject_cast<QSpinBox*>(w))
            app->setProperty(qPrintable(d.key), box->value());
        else if (QDoubleSpinBox* box = qobject_cast<QDoubleSpinBox*>(w))
            app->setProperty(qPrintable(d.key), box->value());
        else if (QLineEdit* edit = qobject_cast<QLineEdit*>(w))
            app->setProperty(qPrintable(d.key), edit->text());
    }

    QDialog::accept();
}

#include "configdialog.moc"
