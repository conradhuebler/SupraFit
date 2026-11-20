/*
 * SpectraWidget - spectra import: spectra list, x values, chart with processing, concentrations
 * Copyright (C) 2020 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <charts.h>

#include <cmath>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include <src/core/spectrahandler.h>

#include <src/ui/widgets/DropTable.h>
#include <src/ui/widgets/concentrationeditor.h>
#include <src/ui/widgets/traceeditorwidget.h>

#include "src/ui/guitools/appearance.h"
#include "src/ui/guitools/dialogmemory.h"
#include "src/ui/guitools/guitools.h"
#include "src/ui/guitools/statusline.h"
#include "src/ui/guitools/waiter.h"

#include "src/global.h"

#include "spectrawidget.h"

SpectraWidget::SpectraWidget(QWidget* parent)
    : QWidget(parent)
{
    m_handler = new SpectraHandler;
    setUI();
}

void SpectraWidget::setUI()
{
    QGridLayout* layout = new QGridLayout;

    m_main_splitter = new QSplitter(Qt::Horizontal);
    m_main_splitter->setObjectName("spectraMainSplitter");
    m_list_splitter = new QSplitter(Qt::Vertical);
    m_list_splitter->setObjectName("spectraListSplitter");

    m_files = new QListWidget;
    m_files->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction* action = new QAction(tr("Remove spectrum"));
    action->setIcon(Icon("list-remove"));
    connect(action, &QAction::triggered, m_files, [this]() {
        QListWidgetItem* item = m_files->currentItem();
        if (!item)
            return;
        // the handler drops the raw spectrum; the files are not read again
        m_handler->removeSpectrum(item->data(Qt::UserRole).toString());
        UpdateSpectra();
        UpdateVerticaLines();
        UpdateData();
        emit changed();
    });
    m_files->addAction(action);

    m_add_xvalue = new QLineEdit;
    m_add_xvalue->setPlaceholderText(tr("Enter x value!"));
    m_add_xvalue->setToolTip(tr("An x value (e.g. a wavelength) for the table; Enter or \"Add value\" adds it, a double-click in the chart, too."));

    m_accept_x = new QPushButton(tr("Add value"));
    m_accept_x->setFlat(true);

    m_xvalues = new QListWidget;

    m_cluster = new QCheckBox;
    m_cluster->setText(tr("Cluster Results"));
    m_cluster->setChecked(true);
    m_cluster->setToolTip(tr("VarCovar selects three times the number of x values and groups them by x into that number of clusters, one x per cluster."));

    m_averaged = new QCheckBox;
    m_averaged->setText(tr("Cluster Average"));
    m_averaged->setChecked(false);
    m_averaged->setToolTip(tr("Each cluster gives its mean x instead of its x of largest variance."));

    m_values = new QSpinBox;
    m_values->setMinimum(1);
    m_values->setMaximum(100);
    m_values->setValue(5);
    m_values->setToolTip(tr("Number of x values VarCovar selects."));
    m_varcovar = new QPushButton(tr("VarCovar"));
    m_varcovar->setToolTip(tr("Selects x values in the selected range from the variance-covariance matrix of the spectra: the x of largest variance first, then the least correlated ones. Replaces the list of x values."));

    QGridLayout* xlayout = new QGridLayout;
    xlayout->addWidget(m_add_xvalue, 0, 0);
    xlayout->addWidget(m_accept_x, 0, 1);
    xlayout->addWidget(m_xvalues, 1, 0, 1, 2);
    //xlayout->addWidget(m_values, 2, 0);
    //xlayout->addWidget(m_varcovar, 2, 1);
    QWidget* xwidget = new QWidget;
    xwidget->setLayout(xlayout);
    m_xvalues->setContextMenuPolicy(Qt::ActionsContextMenu);

    action = new QAction(tr("Remove wavelength"));
    action->setIcon(Icon("list-remove"));
    connect(action, &QAction::triggered, m_xvalues, [this]() {
        auto avl = m_xvalues->currentRow();
        if (avl < m_xvalues->count() && avl >= 0) {
            delete m_xvalues->takeItem(avl);
            UpdateXValues();
            UpdateVerticaLines();
            UpdateData();
            emit changed();
        }
    });
    m_xvalues->addAction(action);

    action = new QAction(tr("Clear List"));
    action->setIcon(Icon("list-remove"));
    connect(action, &QAction::triggered, m_xvalues, [this]() {
        m_xvalues->clear();
        m_handler->setXValues(QVector<double>());
        UpdateVerticaLines();
        UpdateData();
        emit changed();
    });
    m_xvalues->addAction(action);

    // chart, range (Start/End, right-button drag) and processing steps come from the trace editor;
    // the x-value selection stays here
    m_editor = new TraceEditorWidget;
    m_editor->setDoubleClickHint(tr("Double-click: add an x value"));
    m_editor->setSettingsKey("spectraimport"); // stack offsets of the spectra dialog (Claude Generated, Viewer 2)
    QLabel* values_label = new QLabel(tr("# X Values"));
    values_label->setToolTip(m_values->toolTip());
    m_editor->addToolWidget(values_label);
    m_editor->addToolWidget(m_values);
    m_editor->addToolWidget(m_cluster);
    m_editor->addToolWidget(m_averaged);
    m_editor->addToolWidget(m_varcovar);
    QWidget* chart_viewport = m_editor;

    m_datatable = new DropTable;

    QWidget* table_viewport = new QWidget;
    QGridLayout* table_layout = new QGridLayout;

    m_export_table = new QPushButton(tr("Export table to file"));
    table_layout->addWidget(m_export_table, 0, 4);
    table_layout->addWidget(m_datatable, 1, 0, 1, 5);

    table_viewport->setLayout(table_layout);

    m_views = new QTabWidget;
    m_views->addTab(chart_viewport, tr("Spectra View"));
    m_views->addTab(table_viewport, tr("Data View"));

    // concentrations: paste, load from a file, rows checked against the spectra (Claude Generated, UX package 2a)
    m_indep = new ConcentrationEditor(tr("Concentrations"));
    m_indep->setObjectName("spectraConcentrationEditor");
    m_indep->table()->setObjectName("spectraConcentrations");
    m_indep->setMaximumWidth(300);
    QWidget* indep = m_indep;

    m_list_splitter->addWidget(m_files);
    m_list_splitter->addWidget(xwidget);
    m_main_splitter->addWidget(m_list_splitter);
    m_files->setMaximumWidth(200);
    m_main_splitter->addWidget(m_views);
    m_main_splitter->addWidget(indep);

    layout->addWidget(m_main_splitter, 0, 0);

    setLayout(layout);

    connect(m_editor->chart(), &ChartView::pointDoubleClicked, this, &SpectraWidget::PointDoubleClicked);
    connect(m_editor, &TraceEditorWidget::rangeChanged, this, [this](double start, double end) {
        m_handler->setXRange(start, end);
        m_project = m_handler->getSpectraData();
        emit changed();
    });
    connect(m_editor, &TraceEditorWidget::operationsChanged, this, [this]() {
        m_handler->setOperations(m_editor->operations());
        UpdateData();
        emit changed();
    });
    connect(m_handler, &SpectraHandler::Updated, this, &SpectraWidget::UpdateData);
    // pasted concentrations go into the table at once (Claude Generated, UX package 1)
    connect(m_indep, &ConcentrationEditor::Edited, this, [this]() {
        UpdateData();
        emit changed();
    });
    for (QCheckBox* box : { m_cluster, m_averaged })
        connect(box, &QCheckBox::toggled, this, &SpectraWidget::changed);
    connect(m_values, &QSpinBox::valueChanged, this, &SpectraWidget::changed);

    connect(m_accept_x, &QPushButton::clicked, this, [this]() { addXValueText(m_add_xvalue->text()); });
    connect(m_add_xvalue, &QLineEdit::returnPressed, this, [this]() { addXValueText(m_add_xvalue->text()); });

    connect(m_varcovar, &QPushButton::clicked, this, [this]() {
        Waiter wait;
        m_handler->setXRange(m_editor->rangeStart(), m_editor->rangeEnd());
        m_xvalues->clear();
        m_handler->VarCovarSelect(m_values->value(), m_cluster->isChecked(), m_averaged->isChecked());
        UpdateVerticaLines();
        for (auto d : m_handler->XValues())
            m_xvalues->addItem(QString::number(d));
        UpdateData();
        emit changed();
    });

    connect(m_export_table, &QPushButton::clicked, this, &SpectraWidget::SaveToFile);
}

void SpectraWidget::setFile(const QString& file)
{
    m_handler->setSpectrafromFile(file);
    UpdateSpectra();
    emit changed();
}

void SpectraWidget::addFile(const QString& file)
{
    m_handler->addSpectrum(file);
    UpdateSpectra();
    emit changed();
}

int SpectraWidget::addFiles(const QStringList& files, QList<QPair<QString, QString>>* failed)
{
    int read = 0;
    for (const QString& file : files) {
        const QFileInfo info(file);
        QString reason;
        if (!info.isFile() || !info.isReadable())
            reason = tr("cannot be read");
        else {
            m_handler->addSpectrum(file);
            const QStringList order = m_handler->getOrder();
            // the readers give an empty spectrum for a file without numbers: drop it again
            if (!order.isEmpty()) {
                const XYTrace trace = m_handler->Spectra().trace(order.last());
                if (trace.size() < 2)
                    reason = tr("no spectrum found");
                else
                    trace.isValid(&reason);
                if (!reason.isEmpty())
                    m_handler->removeSpectrum(order.last());
            }
        }
        if (reason.isEmpty())
            ++read;
        else if (failed)
            failed->append(qMakePair(file, reason));
    }
    UpdateSpectra();
    UpdateData();
    emit changed();
    return read;
}

void SpectraWidget::rememberIn(DialogMemory* memory)
{
    if (!memory)
        return;
    memory->restore(m_values, "xValueCount");
    memory->restore(m_cluster, "clusterResults");
    memory->restore(m_averaged, "clusterAverage");
    connect(m_values, &QSpinBox::valueChanged, this, [memory](int value) { memory->setValue("xValueCount", value); });
    connect(m_cluster, &QCheckBox::toggled, this, [memory](bool on) { memory->setValue("clusterResults", on); });
    connect(m_averaged, &QCheckBox::toggled, this, [memory](bool on) { memory->setValue("clusterAverage", on); });
}

void SpectraWidget::setDirectory(const QString& directry, const QString& type)
{
    m_handler->addDirectory(directry, type);
    UpdateSpectra();
    emit changed();
}

void SpectraWidget::clear()
{
    m_handler->clearFiles();
    m_editor->setTraces(TraceSet());
    m_files->clear();
}

/*! \brief Shows the spectra of the handler: chart (trace editor, range = the handler's XStart/XEnd)
 * and the list of spectra in the colours of the chart. */
void SpectraWidget::UpdateSpectra()
{
    m_files->clear();
    m_editor->setTraces(m_handler->Spectra());
    m_editor->setRange(m_handler->XStart(), m_handler->XEnd());
    const QList<QColor>& colors = m_editor->traceColors();
    const QStringList order = m_handler->getOrder();
    for (int i = 0; i < order.size(); ++i) {
        const QString& string = order[i];
        QListWidgetItem* item = new QListWidgetItem;
        item->setText(m_handler->Name(string));
        item->setData(Qt::UserRole, string);
        item->setData(Qt::UserRole + 1, m_handler->Path(string));
        if (i < colors.size()) {
            item->setData(Qt::BackgroundRole, colors[i]);
            item->setData(Qt::ForegroundRole, Appearance::ContrastText(colors[i]));
        }
        m_files->addItem(item);
    }
    m_indep->setSpectraCount(order.size());
}

void SpectraWidget::PointDoubleClicked(const QPointF& point)
{
    m_xvalues->addItem(QString::number(point.x()));
    m_handler->addXValue(point.x());
    UpdateData();
    UpdateVerticaLines();
    emit changed();
}

bool SpectraWidget::addXValueText(const QString& text)
{
    QString t = text.trimmed();
    t.replace(',', '.');
    bool ok = false;
    const double x = QLocale::c().toDouble(t, &ok);
    if (!ok || !std::isfinite(x)) {
        emit message(t.isEmpty() ? tr("Type an x value first.") : tr("'%1' is not a number: no x value added.").arg(text.trimmed()), StatusLine::Error);
        return false;
    }
    m_xvalues->addItem(QString::number(x));
    m_handler->addXValue(x);
    UpdateData();
    UpdateVerticaLines();
    m_add_xvalue->clear();
    emit changed();
    return true;
}

int SpectraWidget::spectraCount() const
{
    return m_handler->getOrder().size();
}

int SpectraWidget::xValueCount() const
{
    return m_handler->XValues().size();
}

int SpectraWidget::concentrationRows() const
{
    return m_indep->rows();
}

int SpectraWidget::concentrationColumns() const
{
    return m_indep->columns();
}

bool SpectraWidget::hasTable() const
{
    return spectraCount() > 0 && xValueCount() > 0;
}

bool SpectraWidget::ready(QString* why) const
{
    const int spectra = spectraCount();
    const int x = xValueCount();
    const int rows = concentrationRows();
    QString reason;
    bool ok = false;
    if (spectra == 0)
        reason = tr("No spectra: load a file or open a directory.");
    else if (x == 0)
        reason = tr("No x values: double-click in the chart, type a value or use VarCovar.");
    else if (rows > 0 && rows != spectra)
        reason = tr("The concentrations have %1 rows for %2 spectra: missing cells would be 0.").arg(rows).arg(spectra);
    else {
        ok = true;
        reason = rows > 0 ? tr("OK hands over %1 spectra at %2 x values with %3 concentration column(s).").arg(spectra).arg(x).arg(concentrationColumns())
                          : tr("OK hands over %1 spectra at %2 x values, without concentrations.").arg(spectra).arg(x);
    }
    if (why)
        *why = reason;
    return ok;
}

/*! \brief Table of the selected x values (processed spectra), joined with the dropped concentrations
 * if there are any; ProjectData() always holds the spectra block of the handler. */
void SpectraWidget::UpdateData()
{
    DataTable* data = qobject_cast<DataTable*>(m_handler->CompileSimpleTable());
    m_input_table = data->ExportTable(true);
    m_datatable->setModel(data);
    // before the early returns: the dialog hands this over also without a concentration table
    m_project = m_handler->getSpectraData();
    QPointer<DataTable> table = m_indep->dataTable();
    if (!table)
        return;
    if (!table->isValid() || table->columnCount() == 0)
        return;
    DataTable* result = new DataTable(table);
    result->appendColumns(data);
    m_datatable->setModel(result);
    m_input_table = result->ExportTable(true);
}

void SpectraWidget::setData(const QJsonObject& data)
{
    {
        const DataTable independent(data["independent"].toObject());
        const QSignalBlocker block(m_indep); // the spectra come next; UpdateData() below builds the table
        m_indep->setTable(&independent);
    }
    m_handler->LoadData(data["raw"].toObject());
    m_editor->setOperations(m_handler->Operations());
    m_editor->setShowProcessed(!m_handler->Operations().isEmpty());
    UpdateSpectra();
    UpdateVerticaLines();
    for (auto d : m_handler->XValues())
        m_xvalues->addItem(QString::number(d));
    UpdateData();
}

void SpectraWidget::UpdateXValues()
{
    QVector<double> list;
    for (int i = 0; i < m_xvalues->count(); ++i)
        list << m_xvalues->item(i)->data(Qt::DisplayRole).toDouble();
    m_handler->setXValues(list);
}

void SpectraWidget::UpdateVerticaLines()
{
    m_editor->chart()->privateView()->removeAllVerticalLines();
    for (auto d : m_handler->XValues())
        m_editor->chart()->privateView()->addVerticalLine(d);
    m_editor->refreshMarkers(); // places the lines on the current axes
}

void SpectraWidget::SaveToFile()
{
    const QString content = qobject_cast<DataTable*>(m_datatable->model())->ExportAsString();
    const QString filename = QFileDialog::getSaveFileName(this, "Export Table", getDir());

    if (filename.isEmpty())
        return;

    setLastDir(filename);

    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite))
        return;

    QTextStream stream(&file);
    stream << content;
}
