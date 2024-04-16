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

#include "src/capabilities/datagenerator.h"

#include "src/core/filehandler.h"
#include "src/core/jsonhandler.h"
#include "src/core/models/dataclass.h"

#include "src/global.h"

#include "src/ui/guitools/waiter.h"

#include "src/ui/dialogs/generatedatadialog.h"
#include "src/ui/dialogs/spectraimport.h"
#include "src/ui/dialogs/thermogram.h"

#include <src/ui/widgets/DropTable.h>

#include <QDebug>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QTimer>

#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableView>

#include "importdata.h"

ImportData::ImportData(const QString& file, QWidget* parent)
    : QDialog(parent)
    , m_filename(file)
    , m_projectfile(QString("Project - %1").arg(QDateTime::currentDateTime().toString()))
{
    m_generator = new DataGenerator(this);
    setUi();
    LoadFile();
}

ImportData::ImportData(QWidget* parent)
    : QDialog(parent)
    , m_projectfile(QString("Project - %1").arg(QDateTime::currentDateTime().toString()))
{
    m_generator = new DataGenerator(this);
    setUi();
    DataTable* model = new DataTable(0, 0, this);

    m_table->setModel(model);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        auto list = m_table->selectionModel()->selectedColumns();
        m_independent_rows->setMaximum(m_table->model()->columnCount() - list.size());
    });
}

ImportData::ImportData(QWeakPointer<DataClass> data)
{
    // That is all not nice, I know

    if (data.toStrongRef()->DataType() == DataClassPrivate::Table) {
        setUi();
        m_table->setModel(data.toStrongRef()->IndependentModel());
    } else if (data.toStrongRef()->DataType() == DataClassPrivate::Thermogram) {
        setUi();
        DataTable* model = new DataTable(0, 0, this);
        m_table->setModel(model);
        connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            auto list = m_table->selectionModel()->selectedColumns();
            m_independent_rows->setMaximum(m_table->model()->columnCount() - list.size());
        });
        m_raw = data.toStrongRef()->ExportData()["raw"].toObject();
        m_systemparameter = data.toStrongRef().data()->getSystemObject();
        QTimer::singleShot(0, this, SLOT(ImportThermogram()));

    } else {
        setUi();
        DataTable* model = new DataTable(0, 0, this);
        m_table->setModel(model);
        connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            auto list = m_table->selectionModel()->selectedColumns();
            m_independent_rows->setMaximum(m_table->model()->columnCount() - list.size());
        });
    }
}

ImportData::~ImportData()
{
    if (m_storeddata)
        delete m_storeddata;
}

void ImportData::setUi()
{
    QGridLayout* layout = new QGridLayout;

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_independent_rows = new QSpinBox;
    m_independent_rows->setMinimum(1);
    m_independent_rows->setValue(1);
    m_independent_rows->setPrefix("# = ");
    connect(m_independent_rows, &QSpinBox::valueChanged, this, &ImportData::ReshapeTable);

    m_simulation = new QCheckBox(tr("Simulate Data with # Data Points"));
    QWidget* simulationWidget = SimulationWidget();
    simulationWidget->setHidden(true);

    connect(m_simulation, &QCheckBox::stateChanged, m_dependent_rows, [this, simulationWidget](int state) {
        simulationWidget->setHidden(!state);
        ReshapeTable();
        if (m_table) {
            if (state)
                m_independent_rows->setValue(m_table->model()->columnCount());
            else
                NoChanged();
        }
    });

    m_line = new QLineEdit;
    m_line->setText(m_projectfile);
    connect(m_line, &QLineEdit::textEdited, this, [this](const QString& text) {
        m_title = text;
    });
    m_select = new QPushButton("Select file");
    connect(m_select, SIGNAL(clicked()), this, SLOT(SelectFile()));
    m_file = new QPushButton("Load");
    m_export = new QPushButton("Export table");
    connect(m_export, SIGNAL(clicked()), this, SLOT(ExportFile()));
    connect(m_file, SIGNAL(clicked()), this, SLOT(LoadFile()));

    m_thermogram = new QPushButton(tr("Import as Thermogram"));
    m_thermogram->setStyleSheet("background-color: #77d740;");
    connect(m_thermogram, &QPushButton::clicked, this, [this]() {
        ImportThermogram(m_filename);
    });

    m_spectra = new QPushButton(tr("Import as Spectras"));
    m_spectra->setStyleSheet("background-color: #77d740;");
    connect(m_spectra, &QPushButton::clicked, this, [this]() {
        ImportSpectra(m_filename);
    });

    m_table = new DropTable;

    layout->addWidget(m_select, 0, 0);
    layout->addWidget(m_line, 0, 1, 1, 2);
    layout->addWidget(m_export, 0, 3);
    layout->addWidget(new QLabel(tr("# Independent Variable(s):")), 1, 0);
    layout->addWidget(m_independent_rows, 1, 1);
    layout->addWidget(simulationWidget, 2, 0, 1, 4);
    layout->addWidget(m_simulation, 1, 3, Qt::AlignRight);

    layout->addWidget(m_table, 6, 0, 1, 4);

    layout->addWidget(m_thermogram, 7, 0);
    layout->addWidget(m_spectra, 7, 1);
    layout->addWidget(m_buttonbox, 7, 2, 1, 2);

    connect(m_table, &DropTable::Edited, this, &ImportData::NoChanged);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionDoubleClicked, this, [this](int i) {
        if (i >= m_equations.size() || i == -1)
            return;
        m_equation->setText(m_equations[i]);
        m_currentEquationIndex = i;
    });

    setLayout(layout);
    setWindowTitle(tr("Import Table"));
    resize(800, 600);
}

QWidget* ImportData::SimulationWidget()
{
    QHBoxLayout* layout = new QHBoxLayout;

    m_equation = new QLineEdit;
    connect(m_equation, &QLineEdit::textEdited, this, [this]() {
        if (m_currentEquationIndex < m_equations.size() && m_currentEquationIndex != -1)
            m_equations[m_currentEquationIndex] = m_equation->text();
        Evaluate();
    });

    m_datapoints = new QSpinBox;
    m_datapoints->setMinimum(1);
    m_datapoints->setValue(20);
    m_datapoints->setPrefix("# = ");
    connect(m_datapoints, &QSpinBox::valueChanged, this, &ImportData::ReshapeTable);

    m_dependent_rows = new QSpinBox;
    m_dependent_rows->setMinimum(1);
    m_dependent_rows->setValue(1);
    m_dependent_rows->setPrefix("# = ");

    m_store_generater = new QPushButton(tr("Store Recipe"));
    connect(m_store_generater, &QPushButton::clicked, this, [this]() {
        const QString filename = QFileDialog::getSaveFileName(this, tr("Store simulation recipe"), getDir(), "*.json");
        if (filename.isEmpty() || filename.isNull())
            return;
        JsonHandler::WriteJsonFile(Generator(), filename);
        setLastDir(filename);
    });

    m_load_generator = new QPushButton(tr("Load Recipe"));
    connect(m_load_generator, &QPushButton::clicked, this, [this]() {
        const QString filename = QFileDialog::getOpenFileName(this, tr("Load simulation recipe"), getDir(), "*.json");
        if (filename.isEmpty() || filename.isNull())
            return;
        QJsonObject data;
        JsonHandler::ReadJsonFile(data, filename);
        if (data.keys().contains("main", Qt::CaseInsensitive)) {
            QJsonObject tmp;
            for (auto key : m_project.keys())
                if (key.compare("main", Qt::CaseInsensitive) == 0) {
                    tmp = m_project[key].toObject();
                    data = tmp;
                    break;
                }
        }
        m_independent_rows->setValue(data["independent"].toInt());
        m_datapoints->setValue(data["datapoints"].toInt());
        m_dependent_rows->setValue(data["dependent"].toInt());
        m_equations = data["equations"].toString().split("|");

        Evaluate(data);
        setLastDir(filename);
    });

    layout->addWidget(new QLabel(tr("Datapoints")));
    layout->addWidget(m_datapoints);
    layout->addWidget(new QLabel(tr("Number Series || Signals")));
    layout->addWidget(m_dependent_rows);
    layout->addWidget(m_store_generater);
    layout->addWidget(m_load_generator);

    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->addLayout(layout);

    layout = new QHBoxLayout;
    layout->addWidget(new QLabel(tr("Equation:")));
    layout->addWidget(m_equation);
    vlayout->addLayout(layout);

    QGroupBox* widget = new QGroupBox;
    widget->setTitle(tr("Data generation details"));
    widget->setLayout(vlayout);
    return widget;
}

void ImportData::NoChanged()
{
    if (!m_table)
        return;
    m_independent_rows->setMinimum(1);
    m_independent_rows->setMaximum(m_table->model()->columnCount());

    if (m_table->model()->columnCount() > 2) {
        m_independent_rows->setValue(qApp->instance()->property("lastSize").toInt());
    } else
        m_independent_rows->setValue(1);
}

void ImportData::ReshapeTable()
{
    if (!m_simulation->isChecked())
        return;

    if (m_equations.size() < m_independent_rows->value()) {
        while (m_equations.size() < m_independent_rows->value())
            m_equations << "X";
    } else {
        while (m_equations.size() > m_independent_rows->value())
            m_equations.removeLast();
    }
    Evaluate(Generator());
}

QJsonObject ImportData::Generator() const
{
    QJsonObject data;
    data["independent"] = m_independent_rows->value();
    data["datapoints"] = m_datapoints->value();
    data["dependent"] = m_dependent_rows->value();
    data["equations"] = m_equations.join("|");
    return data;
}

void ImportData::Evaluate()
{
    if (!m_simulation->isChecked())
        return;

    Evaluate(Generator());
}

void ImportData::Evaluate(const QJsonObject& data)
{
    m_generator->setJson(data);
    m_generator->Evaluate();
    m_generated_table = m_generator->Table();
    m_table->setModel(m_generated_table);
    // m_table->selectAll();
    //  m_data = data;
    setGeneratedData(data);
}

void ImportData::SelectFile()
{
    m_filename = QFileDialog::getOpenFileName(this, "Select file", getDir());
    setLastDir(m_filename);
    m_projectfile = QString();
    LoadFile();
}

void ImportData::LoadFile()
{
    Waiter wait;
    QFileInfo info(m_filename);
    QPointer<DataTable> model;
    if (info.suffix() == "itc" || info.suffix() == "ITC") {

        ImportThermogram(m_filename);
    } else if (info.suffix() == "json" || info.suffix() == "JSON" || info.suffix() == "suprafit" || info.suffix() == "SUPRAFIT" || info.suffix() == "jdat" || info.suffix() == "JDAT") {
        m_projectfile = m_filename;
        JsonHandler::ReadJsonFile(m_project, m_filename);
        if ((m_project.keys().contains("datapoints") && m_project.keys().contains("equations")) || m_project.keys().contains("main", Qt::CaseInsensitive)) {
#pragma message("move the simulation import to the filehandler soon!")
            m_simulation->setChecked(true);
            if (m_project.keys().contains("main", Qt::CaseInsensitive)) {
                QJsonObject tmp;
                for (auto key : m_project.keys())
                    if (key.compare("main", Qt::CaseInsensitive) == 0) {
                        tmp = m_project[key].toObject();
                        m_project = tmp;
                        break;
                    }
            }
            m_independent_rows->setValue(m_project["independent"].toInt());
            m_datapoints->setValue(m_project["datapoints"].toInt());
            m_dependent_rows->setValue(m_project["dependent"].toInt());
            m_equations = m_project["equations"].toString().split("|");

            Evaluate(m_project);
            QTimer::singleShot(0, this, &ImportData::accept);
        } else
            QDialog::accept();
    } else {
        m_projectfile = m_filename;
        m_line->setText(info.baseName());
        m_title = info.baseName();
        FileHandler* filehandler = new FileHandler(m_filename, this);
        filehandler->LoadFile();
        if (filehandler->FileSupported()) {
            model = filehandler->getData();
            if (!model) {
                delete filehandler;
                return;
            }
            if (model->columnCount() == 0 && model->rowCount() == 0 && qApp->instance()->property("auto_thermo_dialog").toBool()) {
                if (!ImportThermogram(m_filename))
                    return;

            } else {
                if (filehandler->Type() == FileHandler::FileType::dH)
                    m_systemparameter = filehandler->SystemParameter();
                model->setEditable(true);
                m_table->setModel(model);
                connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
                    auto list = m_table->selectionModel()->selectedColumns();
                    m_independent_rows->setMaximum(m_table->model()->columnCount() - list.size());
                });
                // m_table->selectAll();
                if (model->columnCount() == 2 && model->rowCount() > 100) {
                    if (qApp->instance()->property("auto_thermo_dialog").toBool()) {
                        if (!ImportThermogram(m_filename))
                            return;
                        else
                            QMessageBox::warning(this, QString("Whow!"), QString("This rather long xy file should probably be treated as thermogram. Just push the Import Thermogram on left.\nBut please be aware that, the automatic peak picking will probably fail to import the data correctly.\nYou need the time between each inject and the starting time for the first injection."));
                    }
                }
                if (filehandler->Type() == FileHandler::FileType::dH)
                    QTimer::singleShot(10, this, &QDialog::accept);
                //NoChanged();
            }
        } else {
            QMessageBox::warning(this, QString("File not supported!"), QString("Sorry, but I don't know this format. Try a simple table."));
            return;
        }
        delete filehandler;
    }
    if (model) {
        m_independent_rows->setMinimum(1);
        m_independent_rows->setMaximum(m_table->model()->columnCount());

        if (m_table->model()->columnCount() > 2) {
            m_independent_rows->setValue(qApp->instance()->property("lastSize").toInt());
        } else
            m_independent_rows->setValue(1);
    }
}

void ImportData::ExportFile()
{
    QString filename = QFileDialog::getSaveFileName(this, "Select file", getDir());
    if (filename.isEmpty())
        return;

    setLastDir(filename);
    DataTable* model = qobject_cast<DataTable*>(m_table->model());

    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite))
        return;

    QTextStream stream(&file);
    stream << model->ExportAsString();
}

void ImportData::LoadTable(DataTable* model, int independent)
{
    m_table->setModel(model);
    m_independent_rows->setValue(independent);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        auto list = m_table->selectionModel()->selectedColumns();
        m_independent_rows->setMaximum(m_table->model()->columnCount() - list.size());
    });
}

void ImportData::setSpectraData(const QJsonObject& json)
{
    m_raw = json;
    m_type = DataClassPrivate::Spectrum;
}

void ImportData::setGeneratedData(const QJsonObject& json)
{
    m_raw = json;
    m_type = DataClassPrivate::Simulation;
}

void ImportData::WriteData(const DataTable* model, int independent)
{
    QVector<int> columns;
    for (const QModelIndex& i : m_table->selectionModel()->selectedColumns()) {
        columns << i.column();
    }

    QPointer<DataTable> tmp;
    for (int i = 0; i < model->columnCount(); ++i) {
        if (columns.contains(i))
            continue;
        DataTable* t = model->BlockColumns(i, 1);
        if (!tmp) {
            tmp = t;
            continue;
        }
        tmp->appendColumns(t);
        delete t;
    }
    independent = m_independent_rows->value();
    m_storeddata = new DataClass;
    // DataTable* indep = model->BlockColumns(0, independent);
    // DataTable* dep = model->BlockColumns(independent, model->columnCount() - independent);
    DataTable* indep = tmp->BlockColumns(0, independent);
    DataTable* dep = tmp->BlockColumns(independent, tmp->columnCount() - independent);
    QStringList header_indep, header_dep;
    QStringList header = tmp->header();

#pragma message("will this still be true after restructureing")
    if (tmp->columnCount() - independent == 0) {
        DataTable* model = new DataTable(indep->rowCount(), m_dependent_rows->value(), this);
        //header_indep = QStringList(header.begin(), header.begin() + independent );
        m_storeddata->setDependentTable(model);
        m_storeddata->setType(DataClassPrivate::DataType::Simulation);
        m_projectfile = QString("Simulation - %1").arg(QDateTime::currentDateTime().toString());
    } else {
        m_storeddata->setDataType(m_type);
        m_storeddata->setDependentTable(dep);
        //header_dep = QStringList(header.begin() + independent + 1, header.end() );
    }
    delete tmp;
#pragma message("have a look at here, while restructureing stuff")
    m_storeddata->setIndependentTable(indep);
    m_storeddata->setRawData(m_raw);
    m_storeddata->setProjectTitle(m_title);
}

void ImportData::accept()
{
    QPointer<DataTable> model = qobject_cast<DataTable*>(m_table->model());
    if (!model) {
        QDialog::reject();
        return;
    }

    if (model->rowCount() == 0 && model->columnCount() == 0) {
        delete model;
        QDialog::reject();
        return;
    }
    WriteData(model);
    QJsonObject object, data;
    data = m_storeddata->ExportData();
    data["system"] = m_systemparameter;
    object["data"] = data;
    m_project = object;
    delete model;
    qApp->instance()->setProperty("lastSize", m_independent_rows->value());
    QDialog::accept();
}

void ImportData::setData(const DataTable* model)
{
    WriteData(model, 2);
}

bool ImportData::ImportThermogram(const QString& filename)
{
    Thermogram* thermogram = new Thermogram;
    if (m_filename.isEmpty())
        thermogram->setExperimentFile(filename);
    thermogram->show();

    if (thermogram->exec() == QDialog::Accepted) {
        if (thermogram->ParameterUsed())
            m_systemparameter = thermogram->SystemParamter();
        FileHandler* handler = new FileHandler(this);
        handler->setFileContent(thermogram->Content());
        DataTable* model = handler->getData();
        m_table->setModel(model);
        connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            auto list = m_table->selectionModel()->selectedColumns();
            m_independent_rows->setMaximum(m_table->model()->columnCount() - list.size());
        });
        NoChanged();
        m_raw = thermogram->Raw();
        m_type = DataClassPrivate::Thermogram;
        m_title = thermogram->ProjectName();
        delete thermogram;
        model->setEditable(true);
        return true;
    } else {
        delete thermogram;
        QDialog::reject();
    }
    return false;
}

bool ImportData::ImportThermogram()
{
    Thermogram* thermogram = new Thermogram;
    thermogram->show();
    thermogram->setRootDir(m_root_dir);

    thermogram->setRaw(m_raw);
    thermogram->setSystemParameter(m_systemparameter);

    if (thermogram->exec() == QDialog::Accepted) {
        if (thermogram->ParameterUsed())
            m_systemparameter = thermogram->SystemParamter();
        FileHandler* handler = new FileHandler(this);
        handler->setFileContent(thermogram->Content());
        DataTable* model = handler->getData();
        m_table->setModel(model);
        connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            auto list = m_table->selectionModel()->selectedColumns();
            m_independent_rows->setMaximum(m_table->model()->columnCount() - list.size());
        });
        NoChanged();
        m_raw = thermogram->Raw();
        m_type = DataClassPrivate::Thermogram;
        m_title = thermogram->ProjectName();
        delete thermogram;
        model->setEditable(true);
        return true;
    } else {
        delete thermogram;
        QDialog::reject();
    }
    return false;
}

bool ImportData::ImportSpectra(const QString& filename)
{
    SpectraImport* import = new SpectraImport;
    import->setSpectraFile(filename);

    if (import->exec() == QDialog::Accepted) {
        QJsonObject table = import->InputTable();
        setSpectraData(import->ProjectData());
        //m_project = import->ProjectData();

        DataTable* model = new DataTable(table);
        m_table->setModel(model);
        connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            auto list = m_table->selectionModel()->selectedColumns();
            m_independent_rows->setMaximum(m_table->model()->columnCount() - list.size());
        });
        NoChanged();
        delete import;
        return true;
    }
    return false;
}

void ImportData::GenerateData()
{
    GenerateDataDialog* generate = new GenerateDataDialog;
    generate->show();

    if (generate->exec() == QDialog::Accepted) {
        DataTable* model = new DataTable(generate->Table());
        QJsonObject data = generate->Data();
        m_table->setModel(model);
        connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            auto list = m_table->selectionModel()->selectedColumns();
            m_independent_rows->setMaximum(m_table->model()->columnCount() - list.size());
        });
        m_independent_rows->setMaximum(data["independent"].toInt());
        m_independent_rows->setValue(data["independent"].toInt());
        m_simulation->setChecked(true);
    }
    delete generate;
}

#include "importdata.moc"
