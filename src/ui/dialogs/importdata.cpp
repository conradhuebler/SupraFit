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

#include "src/core/dataclass.h"
#include "src/core/filehandler.h"
#include "src/core/jsonhandler.h"
#include "src/global.h"

#include "src/ui/dialogs/thermogram.h"

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

void TableView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_C && event->modifiers() & Qt::ControlModifier) {
        QApplication::clipboard()->setText(this->currentIndex().data().toString());
    } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_V) {
        QString paste = QApplication::clipboard()->text();
        FileHandler* handler = new FileHandler(this);
        handler->setFileContent(paste);
        DataTable* model = handler->getData();
        if (model->isValid()) {
            setModel(model);
            emit Edited();
        }
        delete handler;
    } else {

        QTableView::keyPressEvent(event);
    }
}

ImportData::ImportData(const QString& file, QWidget* parent)
    : QDialog(parent)
    , m_filename(file)
    , m_projectfile(QString("Project - %1").arg(QDateTime::currentDateTime().toString()))
{
    setUi();
    LoadFile();
}

ImportData::ImportData(QWidget* parent)
    : QDialog(parent)
    , m_projectfile(QString("Project - %1").arg(QDateTime::currentDateTime().toString()))
{
    setUi();
    DataTable* model = new DataTable(0, 0, this);
    m_table->setModel(model);
}

ImportData::ImportData(QWeakPointer<DataClass> data)
{
    // That is all not nice, I know

    if (data.data()->DataType() == DataClassPrivate::Table) {
        setUi();
        m_table->setModel(data.data()->IndependentModel());
        m_sec_table->setModel(data.data()->DependentModel());
    } else if (data.data()->DataType() == DataClassPrivate::Thermogram) {
        setUi();
        DataTable* model = new DataTable(0, 0, this);
        m_table->setModel(model);
        m_raw = data.data()->ExportData()["raw"].toObject();
        m_systemparameter = data.data()->getSystemObject();
        QTimer::singleShot(0, this, SLOT(ImportThermogram()));

    } else {
        setUi();
        DataTable* model = new DataTable(0, 0, this);
        m_table->setModel(model);
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

    m_simulation = new QCheckBox(tr("Simulate Data with # Serie(s)"));
    m_dependent_rows = new QSpinBox;
    m_dependent_rows->setMinimum(1);
    m_dependent_rows->setValue(1);
    m_dependent_rows->setEnabled(false);
    m_dependent_rows->setPrefix("# = ");

    connect(m_simulation, &QCheckBox::stateChanged, m_dependent_rows, [this](int state) {
        m_dependent_rows->setEnabled(state);

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

    m_thermogram = new QPushButton(tr("Import Thermogram"));
    connect(m_thermogram, &QPushButton::clicked, this, [this]() {
        ImportThermogram(m_filename);
    });

    m_table = new TableView;

    layout->addWidget(m_select, 0, 0);
    layout->addWidget(m_line, 0, 1, 1, 2);
    layout->addWidget(m_export, 0, 3);
    layout->addWidget(new QLabel(tr("# Independent Variable(s):")), 1, 0);
    layout->addWidget(m_independent_rows, 1, 1);
    layout->addWidget(m_simulation, 1, 2, Qt::AlignRight);
    layout->addWidget(m_dependent_rows, 1, 3);
    layout->addWidget(m_table, 3, 0, 1, 4);
    layout->addWidget(m_thermogram, 4, 0);
    layout->addWidget(m_buttonbox, 4, 1, 1, 3);

    connect(m_table, &TableView::Edited, this, &ImportData::NoChanged);

    setLayout(layout);
    setWindowTitle(tr("Import Table"));
    resize(800, 600);
}

void ImportData::NoChanged()
{
    if (!m_table)
        return;
    m_independent_rows->setMinimum(1);
    m_independent_rows->setMaximum(m_table->model()->columnCount());

    if (m_table->model()->columnCount() > 2)
        m_independent_rows->setValue(2);
    else
        m_independent_rows->setValue(1);
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
    QFileInfo info(m_filename);
    QPointer<DataTable> model;
    if (info.suffix() == "itc" || info.suffix() == "ITC") {

        ImportThermogram(m_filename);
    } else if (info.suffix() == "json" || info.suffix() == "JSON" || info.suffix() == "suprafit" || info.suffix() == "SUPRAFIT" || info.suffix() == "jdat" || info.suffix() == "JDAT") {
        m_projectfile = m_filename;
        JsonHandler::ReadJsonFile(m_project, m_filename);
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

        if (m_table->model()->columnCount() > 2)
            m_independent_rows->setValue(2);
        else
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

void ImportData::WriteData(const DataTable* model, int independent)
{
    independent = m_independent_rows->value();
    m_storeddata = new DataClass;
    DataTable* indep = model->BlockColumns(0, independent);
    DataTable* dep = model->BlockColumns(independent, model->columnCount() - independent);

    if (model->columnCount() - independent == 0) {
        DataTable* model = new DataTable(m_dependent_rows->value(), indep->rowCount(), this);
        m_storeddata->setDependentTable(model);
        m_storeddata->setType(DataClassPrivate::DataType::Simulation);
        m_projectfile = QString("Simulation - %1").arg(QDateTime::currentDateTime().toString());
    } else {
        m_storeddata->setDataType(m_type);
        m_storeddata->setDependentTable(dep);
    }

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
    QDialog::accept();
}

bool ImportData::ImportThermogram(const QString& filename)
{
    Thermogram* thermogram = new Thermogram;
    if (!m_filename.isEmpty())
        thermogram->setExperimentFile(filename);
    thermogram->show();

    if (thermogram->exec() == QDialog::Accepted) {
        if (thermogram->ParameterUsed())
            m_systemparameter = thermogram->SystemParamter();
        FileHandler* handler = new FileHandler(this);
        handler->setFileContent(thermogram->Content());
        DataTable* model = handler->getData();
        m_table->setModel(model);
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
#include "importdata.moc"
