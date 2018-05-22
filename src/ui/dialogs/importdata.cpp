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

#include "src/core/dataclass.h"
#include "src/core/filehandler.h"
#include "src/global.h"

#include "src/ui/dialogs/thermogram.h"

#include <QDebug>

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
        setModel(model);
        emit Edited();
        delete handler;
    } else {

        QTableView::keyPressEvent(event);
    }
}

ImportData::ImportData(const QString& file, QWidget* parent)
    : QDialog(parent)
    , m_filename(file)
{
    setUi();
    LoadFile();
}

ImportData::ImportData(QWidget* parent)
    : QDialog(parent)
{
    setUi();
    DataTable* model = new DataTable(0, 0, this);
    m_table->setModel(model);
}

ImportData::ImportData(QWeakPointer<DataClass> data)
    : m_single(false)
{
    // That is all not nice, I know

    if (data.data()->DataType() == DataClassPrivate::Table) {
        setUi(m_single);
        m_table->setModel(data.data()->IndependentModel());
        m_sec_table->setModel(data.data()->DependentModel());
    } else if (data.data()->DataType() == DataClassPrivate::Thermogram) {
        setUi(true);
        DataTable* model = new DataTable(0, 0, this);
        m_table->setModel(model);
        m_raw = data.data()->ExportData()["raw"].toObject();
        QTimer::singleShot(0, this, SLOT(ImportThermogram(QString)));

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

void ImportData::setUi(bool single)
{
    QGridLayout* layout = new QGridLayout;

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_switch_concentration = new QCheckBox;
    m_switch_concentration->setText("Switch Host/Guest");
    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_conc = new QSpinBox;
    connect(m_conc, SIGNAL(editingFinished()), this, SLOT(NoChanged()));
    m_line = new QLineEdit;
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

    if (single == false) {
        m_sec_table = new TableView;
    }
    if (single) {
        layout->addWidget(m_select, 0, 0);
        layout->addWidget(m_line, 0, 1);
        layout->addWidget(m_export, 0, 3);

        layout->addWidget(new QLabel(tr("No. of indepdent variables:")), 1, 0);
        layout->addWidget(m_conc, 1, 1);
        layout->addWidget(m_table, 3, 0, 1, 4);
    }
    if (!single) {
        layout->addWidget(m_table, 3, 0);
        m_table->setFixedWidth(200);
        layout->addWidget(m_sec_table, 3, 1, 1, 3);
        m_sec_table->setFixedWidth(600);
    }
    if (single) {
        layout->addWidget(m_thermogram, 4, 0);
    }
    layout->addWidget(m_buttonbox, 4, 1, 1, 4);
    connect(m_table, &TableView::Edited, this, &ImportData::NoChanged);

    setLayout(layout);
    setWindowTitle(tr("Import Table"));
    resize(800, 600);
}

void ImportData::NoChanged()
{
    m_conc->setMinimum(1);
    m_conc->setMaximum(m_table->model()->columnCount() - 1);
    if (m_table->model()->columnCount() > 2)
        m_conc->setValue(2);
    else
        m_conc->setValue(1);
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
    PeakPick::spectrum original;
    if (info.suffix() == "itc" || info.suffix() == "ITC") {

        ImportThermogram(m_filename);
    } else if (info.suffix() == "json" || info.suffix() == "JSON" || info.suffix() == "suprafit" || info.suffix() == "SUPRAFIT" || info.suffix() == "jdat" || info.suffix() == "JDAT") {
        m_projectfile = m_filename;
        QDialog::accept();
    } else {
        m_line->setText(m_filename);
        FileHandler* filehandler = new FileHandler(m_filename, this);

        if (filehandler->FileSupported()) {
            DataTable* model = filehandler->getData();
            model->setEditable(true);
            m_table->setModel(model);
            if (model->columnCount() == 2 && model->rowCount() > 100)
                QMessageBox::warning(this, QString("Whow!"), QString("This rather long xy file should probably be treated as thermogram. Just push the Import Thermogram on left.\nBut please be aware that, the automatic peak picking will probably fail to import the data correctly."));
        } else
            QMessageBox::warning(this, QString("File not supported!"), QString("Sorry, but I don't know this format. Try a simple table."));

        delete filehandler;
        NoChanged();
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
    independent = m_conc->value();
    m_storeddata = new DataClass;
    DataTable* concentration_block = model->BlockColumns(0, independent);
    DataTable* signals_block = model->BlockColumns(independent, model->columnCount() - independent);
    m_storeddata->setDependentTable(signals_block);
    m_storeddata->setIndependentTable(concentration_block);
    m_storeddata->setRawData(m_raw);
    m_storeddata->setDataType(m_type);
    m_storeddata->setProjectTitle(m_title);
}

void ImportData::accept()
{
    DataTable* model = qobject_cast<DataTable*>(m_table->model());
    WriteData(model);
    QDialog::accept();
}

void ImportData::ImportThermogram(const QString& filename)
{
    Thermogram* thermogram = new Thermogram;
    if (!m_filename.isEmpty())
        thermogram->setExperimentFile(filename);
    thermogram->show();

    if (thermogram->exec() == QDialog::Accepted) {
        FileHandler* handler = new FileHandler(this);
        handler->setFileContent(thermogram->Content());
        DataTable* model = handler->getData();
        m_table->setModel(model);
        NoChanged();
        m_raw = thermogram->Raw();
        m_type = DataClassPrivate::Thermogram;
        m_title = thermogram->ProjectName();
        delete thermogram;
        accept();
    } else
        delete thermogram;
}
void ImportData::ImportThermogram()
{
    Thermogram* thermogram = new Thermogram;

    QJsonObject experiment = m_raw["experiment"].toObject();

    if (m_raw.keys().contains("dilution")) {
        QJsonObject experiment = m_raw["dilution"].toObject();

        thermogram->setDilutionFile(experiment["file"].toString());
        thermogram->setDilutionFit(experiment["fit"].toObject());
    }

    thermogram->setExperimentFile(experiment["file"].toString());
    thermogram->setExperimentFit(experiment["fit"].toObject());

    thermogram->show();

    if (thermogram->exec() == QDialog::Accepted) {
        FileHandler* handler = new FileHandler(this);
        handler->setFileContent(thermogram->Content());
        DataTable* model = handler->getData();
        m_table->setModel(model);
        NoChanged();
        m_raw = thermogram->Raw();
        m_type = DataClassPrivate::Thermogram;
        m_title = thermogram->ProjectName();
        delete thermogram;
        accept();

    } else {
        delete thermogram;
        QDialog::reject();
    }
}
#include "importdata.moc"
