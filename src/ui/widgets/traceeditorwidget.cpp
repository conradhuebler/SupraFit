/*
 * TraceEditorWidget - chart, range selection and processing steps of a set of traces
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

// Claude Generated: WP2 of roadmap/input_data_editing.md

#include <charts.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include <QtCore/QEvent>
#include <QtCore/QJsonArray>
#include <QtCore/QLocale>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>

#include <QtGui/QMouseEvent>
#include <QtGui/QPalette>
#include <QtGui/QWheelEvent>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

#include <QtWidgets/QApplication>

#include "src/ui/guitools/appearance.h"
#include "src/ui/guitools/statusline.h"

#include "traceeditorwidget.h"

namespace {
const double kLimit = 1e12; ///< bounds of the free number fields

/// decimals for an axis of this span: wavelengths in nm need 2, ppm or small spans 4
int decimalsFor(double span)
{
    return (std::isfinite(span) && span >= 100) ? 2 : 4;
}

/// "a b; c d" from excluded ranges
QString rangesToText(const QJsonArray& ranges)
{
    QStringList parts;
    for (const QJsonValue& v : ranges) {
        const QJsonArray pair = v.toArray();
        if (pair.size() == 2)
            parts << QString("%1 %2").arg(pair[0].toDouble()).arg(pair[1].toDouble());
    }
    return parts.join("; ");
}

/// parses "a b; c d" (also "a-b" is not accepted: minus belongs to the numbers); false on a malformed entry
bool textToRanges(const QString& text, QJsonArray* ranges)
{
    QJsonArray result;
    const QStringList parts = text.split(';', Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        const QStringList numbers = part.trimmed().split(QRegularExpression("[\\s]+"), Qt::SkipEmptyParts);
        if (numbers.isEmpty())
            continue;
        if (numbers.size() != 2)
            return false;
        bool ok1 = false, ok2 = false;
        const double a = QLocale::c().toDouble(numbers[0], &ok1);
        const double b = QLocale::c().toDouble(numbers[1], &ok2);
        if (!ok1 || !ok2)
            return false;
        result.append(QJsonArray{ a, b });
    }
    *ranges = result;
    return true;
}
} // namespace

TraceEditorWidget::TraceEditorWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void TraceEditorWidget::setupUi()
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);

    // range row
    QHBoxLayout* range = new QHBoxLayout;
    m_start = new QDoubleSpinBox;
    m_end = new QDoubleSpinBox;
    for (QDoubleSpinBox* box : { m_start, m_end }) {
        box->setKeyboardTracking(false);
        box->setMinimumWidth(90);
        box->setRange(-kLimit, kLimit);
        box->setDecimals(2);
        connect(box, &QDoubleSpinBox::valueChanged, this, [this]() {
            rangeMoved();
            if (!m_updating)
                emit rangeChanged(rangeStart(), rangeEnd());
        });
    }
    m_start->setToolTip(tr("Lower end of the selected x range; a drag with the right mouse button in the chart selects it, too."));
    m_end->setToolTip(tr("Upper end of the selected x range."));
    m_zoom = new QPushButton(tr("Zoom to range"));
    m_zoom->setFlat(true);
    m_fit_y = new QPushButton(tr("Fit y to the visible range"));
    m_fit_y->setObjectName("traceFitY");
    m_fit_y->setFlat(true);
    m_fit_y->setToolTip(tr("Scales the y axis to the lowest and highest point of the drawn traces between the left and right end of the x axis."));
    m_processed = new QCheckBox(tr("Show processed"));
    m_processed->setToolTip(tr("Show the traces after the processing steps instead of the raw data."));
    range->addWidget(new QLabel(tr("Start")));
    range->addWidget(m_start);
    range->addWidget(new QLabel(tr("End")));
    range->addWidget(m_end);
    range->addWidget(m_zoom);
    range->addWidget(m_fit_y);
    range->addWidget(m_processed);
    range->addStretch();
    layout->addLayout(range);

    // stacking: view only (Claude Generated, Viewer 2)
    QHBoxLayout* stack = new QHBoxLayout;
    m_stack_y = new QDoubleSpinBox;
    m_stack_y->setObjectName("traceStackOffset");
    m_stack_y->setRange(0, 1000);
    m_stack_y->setDecimals(0);
    m_stack_y->setSingleStep(10);
    m_stack_y->setSuffix(tr(" %"));
    m_stack_y->setKeyboardTracking(false);
    m_stack_y->setToolTip(tr("Draws the traces one above the other: each one higher by this share of the tallest trace between Start and End. Only the view changes, not the data."));
    m_stack_x = new QDoubleSpinBox;
    m_stack_x->setObjectName("traceStackXOffset");
    m_stack_x->setRange(-50, 50);
    m_stack_x->setDecimals(1);
    m_stack_x->setSingleStep(0.5);
    m_stack_x->setSuffix(tr(" %"));
    m_stack_x->setKeyboardTracking(false);
    m_stack_x->setToolTip(tr("Moves each trace of the stack further to the right on the screen by this share of the range Start to End. Only the view changes, not the data."));
    m_stack_reverse = new QCheckBox(tr("Reverse order"));
    m_stack_reverse->setObjectName("traceStackReverse");
    m_stack_reverse->setToolTip(tr("Off: the first trace of the list at the bottom of the stack. On: the first trace on top."));
    QLabel* stack_label = new QLabel(tr("Stack offset"));
    stack_label->setToolTip(m_stack_y->toolTip());
    QLabel* x_label = new QLabel(tr("x offset"));
    x_label->setToolTip(m_stack_x->toolTip());
    stack->addWidget(stack_label);
    stack->addWidget(m_stack_y);
    stack->addWidget(x_label);
    stack->addWidget(m_stack_x);
    stack->addWidget(m_stack_reverse);
    stack->addStretch();
    layout->addLayout(stack);

    // row of the host's controls
    m_tool_row = new QWidget;
    m_tools = new QHBoxLayout;
    m_tools->setContentsMargins(0, 0, 0, 0);
    m_tools->addStretch();
    m_tool_row->setLayout(m_tools);
    m_tool_row->setHidden(true);
    layout->addWidget(m_tool_row);

    m_chart = new ChartView;
    m_chart->setAutoScaleStrategy(AutoScaleStrategy::QtNiceNumbers);
    m_chart->setVerticalLineEnabled(true);
    m_chart->setSelectStrategy(SelectStrategy::Horizontal);
    m_chart->privateView()->setVerticalLinePrec(0);
    m_chart->privateView()->setVerticalLinesPrec(-1);
    m_chart->privateView()->installEventFilter(this);
    m_chart->privateView()->viewport()->installEventFilter(this); // wheel and middle button (Viewer 2)
    // a zoom by the mouse is the user's view from now on
    connect(m_chart, &ChartView::zoomChanged, this, [this]() {
        captureView();
        refreshMarkers();
    });

    // processing steps
    QWidget* panel = new QWidget;
    QVBoxLayout* panel_layout = new QVBoxLayout;
    panel_layout->setContentsMargins(0, 0, 0, 0);
    panel_layout->addWidget(new QLabel(tr("Processing steps")));
    m_list = new QListWidget;
    m_list->setToolTip(tr("Applied from top to bottom to every trace; the raw data stay unchanged. Untick a step to switch it off."));
    panel_layout->addWidget(m_list, 1);

    m_add = new QToolButton;
    m_add->setText(tr("Add"));
    m_add->setPopupMode(QToolButton::InstantPopup);
    QMenu* menu = new QMenu(m_add);
    const QList<QPair<QString, QString>> types = {
        { "Range", tr("Range") },
        { "Smooth", tr("Smoothing (Savitzky-Golay)") },
        { "Baseline", tr("Baseline") },
        { "Normalise", tr("Normalisation") },
        { "ShiftAxis", tr("Shift of the x axis") },
        { "Blank", tr("Blank region") },
        { "PickPeaks", tr("Peak picking") }
    };
    for (const auto& type : types) {
        const QString key = type.first;
        menu->addAction(type.second, this, [this, key]() { addOperation(key); });
    }
    m_add->setMenu(menu);
    m_remove = new QPushButton(tr("Remove"));
    m_up = new QPushButton(tr("Up"));
    m_down = new QPushButton(tr("Down"));
    QHBoxLayout* buttons = new QHBoxLayout;
    buttons->addWidget(m_add);
    buttons->addWidget(m_remove);
    buttons->addWidget(m_up);
    buttons->addWidget(m_down);
    panel_layout->addLayout(buttons);

    m_parameters = new QGroupBox(tr("Parameters"));
    m_form = new QFormLayout;
    m_parameters->setLayout(m_form);
    panel_layout->addWidget(m_parameters);

    m_error_label = new QLabel;
    m_error_label->setWordWrap(true);
    m_error_label->setHidden(true);
    panel_layout->addWidget(m_error_label);
    panel->setLayout(panel_layout);
    panel->setMinimumWidth(240);

    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(m_chart);
    splitter->addWidget(panel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setChildrenCollapsible(false);
    layout->addWidget(splitter, 1);

    // one-line hint on the mouse and keys of the chart (Claude Generated, UX package 1)
    m_hint = new StatusLine;
    m_hint->setObjectName("traceEditorHint");
    layout->addWidget(m_hint);
    setLayout(layout);

    connect(m_zoom, &QPushButton::clicked, this, [this]() {
        m_chart->setXMin(rangeStart());
        m_chart->setXMax(rangeEnd());
        captureView();
        refreshMarkers();
    });
    connect(m_fit_y, &QPushButton::clicked, this, &TraceEditorWidget::fitYToVisible);
    for (QDoubleSpinBox* box : { m_stack_y, m_stack_x })
        connect(box, &QDoubleSpinBox::valueChanged, this, [this]() {
            saveStackSettings();
            applyStack();
        });
    connect(m_stack_reverse, &QCheckBox::toggled, this, [this]() {
        saveStackSettings();
        applyStack();
    });
    connect(m_processed, &QCheckBox::toggled, this, &TraceEditorWidget::updateChart);
    connect(m_chart, &ChartView::addRect, this, [this](const QPointF& a, const QPointF& b) { selectionToRange(a.x(), b.x()); });
    connect(m_list, &QListWidget::currentRowChanged, this, [this]() {
        updateForm();
        const int row = m_list->currentRow();
        m_remove->setEnabled(row >= 0);
        m_up->setEnabled(row > 0);
        m_down->setEnabled(row >= 0 && row + 1 < m_list->count());
    });
    connect(m_list, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        if (m_updating)
            return;
        setOperationEnabled(m_list->row(item), item->checkState() == Qt::Checked);
    });
    connect(m_remove, &QPushButton::clicked, this, [this]() { removeOperation(m_list->currentRow()); });
    connect(m_up, &QPushButton::clicked, this, [this]() { moveOperation(m_list->currentRow(), m_list->currentRow() - 1); });
    connect(m_down, &QPushButton::clicked, this, [this]() { moveOperation(m_list->currentRow(), m_list->currentRow() + 1); });

    updateList();
    updateForm();
    updateHint();
}

void TraceEditorWidget::setDoubleClickHint(const QString& text)
{
    m_double_click_hint = text;
    updateHint();
}

QString TraceEditorWidget::hintText() const
{
    QStringList parts = { tr("Left-drag: zoom"), tr("Wheel: y zoom at the pointer"), tr("Middle click: whole spectrum"), tr("Right-drag: select the range") };
    if (!m_double_click_hint.isEmpty())
        parts << m_double_click_hint;
    parts << tr("ESC: cancel a drag");
    return parts.join(QStringLiteral("  ·  "));
}

bool TraceEditorWidget::hintsEnabled()
{
    const QVariant value = qApp->property("thermogram_guidelines");
    return !value.isValid() || value.toBool();
}

void TraceEditorWidget::updateHint()
{
    m_hint->setMessage(hintText(), StatusLine::Info);
    m_hint->setVisible(hintsEnabled());
}

void TraceEditorWidget::showEvent(QShowEvent* event)
{
    updateHint(); // the setting may have changed since the widget was built
    QWidget::showEvent(event);
}

/*! \brief New traces. The same traces again (same ids, e.g. other FID processing or another referencing
 * offset) keep the range and the user's zoom; if their whole axis moved by the same amount (referencing),
 * range and zoom move along, so the same signals stay in view. Other traces: whole range, zoom reset. */
void TraceEditorWidget::setTraces(const TraceSet& traces)
{
    QStringList old_ids = m_traces.ids(), new_ids = traces.ids();
    std::sort(old_ids.begin(), old_ids.end());
    std::sort(new_ids.begin(), new_ids.end());
    const bool same = !new_ids.isEmpty() && old_ids == new_ids;
    double old_lo = 0, old_hi = 0;
    const bool had_range = m_traces.commonRange(&old_lo, &old_hi);
    const double old_start = rangeStart(), old_end = rangeEnd();

    m_traces = traces;
    double lo = 0, hi = 0;
    const bool has_range = m_traces.commonRange(&lo, &hi);
    if (!has_range) {
        lo = -kLimit;
        hi = kLimit;
    }
    double start = lo, end = hi;
    if (same && had_range && has_range) {
        // a shift of the whole axis: both ends moved by the same amount
        const double d_lo = lo - old_lo, d_hi = hi - old_hi;
        const double tolerance = 1e-9 * std::max(1.0, std::abs(hi - lo));
        const double delta = std::abs(d_lo - d_hi) < tolerance ? 0.5 * (d_lo + d_hi) : 0;
        // a range at an end of the data stays at that end
        start = old_start <= old_lo ? lo : std::clamp(old_start + delta, lo, hi);
        end = old_end >= old_hi ? hi : std::clamp(old_end + delta, lo, hi);
        m_view_x0 += delta;
        m_view_x1 += delta;
    } else
        m_user_view = false;
    m_updating = true;
    for (QDoubleSpinBox* box : { m_start, m_end }) {
        box->setDecimals(decimalsFor(hi - lo));
        box->setRange(lo, hi);
    }
    m_start->setValue(start);
    m_end->setValue(end);
    m_updating = false;
    updateChart();
}

void TraceEditorWidget::setOperations(const TraceOpList& operations)
{
    m_operations = operations;
    m_edit_error.clear();
    updateList();
    m_list->setCurrentRow(m_operations.isEmpty() ? -1 : 0);
    updateForm();
    updateChart();
}

void TraceEditorWidget::setRange(double start, double end)
{
    m_updating = true;
    m_start->setValue(std::min(start, end));
    m_end->setValue(std::max(start, end));
    m_updating = false;
    rangeMoved();
}

double TraceEditorWidget::rangeStart() const
{
    return m_start->value();
}

double TraceEditorWidget::rangeEnd() const
{
    return m_end->value();
}

void TraceEditorWidget::setShowProcessed(bool processed)
{
    m_processed->setChecked(processed); // toggled() redraws
}

bool TraceEditorWidget::showProcessed() const
{
    return m_processed->isChecked();
}

void TraceEditorWidget::addToolWidget(QWidget* widget)
{
    m_tools->insertWidget(m_tools->count() - 1, widget); // before the stretch
    m_tool_row->setHidden(false);
}

/*! \brief The selection of a right-button drag; the two x values come in screen order, which is
 * descending on a reversed axis. */
void TraceEditorWidget::selectionToRange(double a, double b)
{
    m_start->blockSignals(true);
    m_end->blockSignals(true);
    m_start->setValue(std::min(a, b));
    m_end->setValue(std::max(a, b));
    m_start->blockSignals(false);
    m_end->blockSignals(false);
    rangeMoved();
    emit rangeChanged(rangeStart(), rangeEnd());
}

int TraceEditorWidget::addOperation(const QString& type)
{
    std::unique_ptr<TraceOp> op;
    if (type == "Range")
        op = std::make_unique<RangeOp>(rangeStart(), rangeEnd());
    else if (type == "Smooth")
        op = std::make_unique<SmoothOp>();
    else if (type == "Normalise")
        op = std::make_unique<NormaliseOp>();
    else if (type == "Baseline")
        op = std::make_unique<BaselineOp>();
    else if (type == "PickPeaks")
        op = std::make_unique<PickPeaksOp>();
    else if (type == "ShiftAxis")
        op = std::make_unique<ShiftAxisOp>();
    else if (type == "Blank") {
        auto blank = std::make_unique<BlankOp>();
        double lo = 0, hi = 0;
        // the selected range, unless it is the whole range (that would blank everything)
        if (m_traces.commonRange(&lo, &hi) && (rangeStart() > lo || rangeEnd() < hi))
            blank->ranges << qMakePair(rangeStart(), rangeEnd());
        op = std::move(blank);
    }
    if (!op) {
        setError(tr("Unknown processing step: %1").arg(type));
        return -1;
    }
    m_operations.append(std::move(op));
    m_edit_error.clear();
    m_updating = true;
    m_processed->setChecked(true); // show what the new step does
    m_updating = false;
    listChanged(m_operations.size() - 1);
    return m_operations.size() - 1;
}

bool TraceEditorWidget::removeOperation(int index)
{
    if (!m_operations.remove(index))
        return false;
    m_edit_error.clear();
    listChanged(std::min(index, m_operations.size() - 1));
    return true;
}

bool TraceEditorWidget::moveOperation(int from, int to)
{
    if (!m_operations.move(from, to))
        return false;
    m_edit_error.clear();
    listChanged(to);
    return true;
}

bool TraceEditorWidget::setOperationEnabled(int index, bool enabled)
{
    if (index < 0 || index >= m_operations.size())
        return false;
    m_operations.at(index)->enabled = enabled;
    m_edit_error.clear();
    if (QListWidgetItem* item = m_list->item(index)) {
        m_updating = true;
        item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
        m_updating = false;
    }
    updateChart();
    emit operationsChanged();
    return true;
}

bool TraceEditorWidget::setOperationParameters(int index, const QJsonObject& parameters)
{
    if (index < 0 || index >= m_operations.size())
        return false;
    TraceOp* current = m_operations.at(index);
    QJsonObject json = current->toJson();
    for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it) {
        if (it.key() == "Type")
            continue;
        if (it.value().isNull())
            json.remove(it.key());
        else
            json[it.key()] = it.value();
    }
    QString error;
    std::unique_ptr<TraceOp> op = TraceOp::fromJson(json, &error);
    if (!op) {
        m_edit_error = tr("Step %1 not changed: %2").arg(index + 1).arg(error);
        updateError();
        return false;
    }
    op->enabled = current->enabled;
    // replace in place: append the new step and move it to the position of the old one
    m_operations.remove(index);
    m_operations.append(std::move(op));
    m_operations.move(m_operations.size() - 1, index);
    m_edit_error.clear();
    if (QListWidgetItem* item = m_list->item(index)) {
        m_updating = true;
        item->setText(operationLabel(m_operations.at(index)));
        m_updating = false;
    }
    updateChart();
    emit operationsChanged();
    return true;
}

void TraceEditorWidget::listChanged(int select)
{
    updateList();
    m_list->setCurrentRow(select);
    updateForm();
    updateChart();
    emit operationsChanged();
}

QString TraceEditorWidget::operationLabel(const TraceOp* op)
{
    if (!op)
        return QString();
    if (auto r = dynamic_cast<const RangeOp*>(op))
        return tr("Range %1 to %2").arg(std::min(r->xStart, r->xEnd)).arg(std::max(r->xStart, r->xEnd));
    if (auto s = dynamic_cast<const SmoothOp*>(op))
        return tr("Smoothing: window %1, order %2").arg(s->window).arg(s->order);
    if (auto n = dynamic_cast<const NormaliseOp*>(op)) {
        QString mode = n->mode == TraceOps::NormaliseMode::Max ? tr("largest magnitude") : n->mode == TraceOps::NormaliseMode::Area ? tr("area")
                                                                                                                                   : tr("minimum to maximum");
        return n->useReference ? tr("Normalisation: %1 in %2 to %3").arg(mode).arg(n->refStart).arg(n->refEnd) : tr("Normalisation: %1").arg(mode);
    }
    if (auto b = dynamic_cast<const BaselineOp*>(op))
        return b->excluded.isEmpty() ? tr("Baseline: degree %1").arg(b->degree) : tr("Baseline: degree %1, %2 excluded").arg(b->degree).arg(b->excluded.size());
    if (auto a = dynamic_cast<const ShiftAxisOp*>(op))
        return tr("Shift of the x axis by %1").arg(a->offset);
    if (auto b = dynamic_cast<const BlankOp*>(op)) {
        if (b->ranges.size() == 1)
            return tr("Blank region %1 to %2").arg(std::min(b->ranges[0].first, b->ranges[0].second)).arg(std::max(b->ranges[0].first, b->ranges[0].second));
        return tr("Blank regions: %1").arg(b->ranges.size());
    }
    if (auto p = dynamic_cast<const PickPeaksOp*>(op))
        return p->relative ? tr("Peak picking: above %1 of the span").arg(p->threshold) : tr("Peak picking: above %1").arg(p->threshold);
    return op->type();
}

void TraceEditorWidget::updateList()
{
    m_updating = true;
    m_list->clear();
    for (int i = 0; i < m_operations.size(); ++i) {
        QListWidgetItem* item = new QListWidgetItem(operationLabel(m_operations.at(i)));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_operations.at(i)->enabled ? Qt::Checked : Qt::Unchecked);
        m_list->addItem(item);
    }
    m_updating = false;
    const int row = m_list->currentRow();
    m_remove->setEnabled(row >= 0);
    m_up->setEnabled(row > 0);
    m_down->setEnabled(row >= 0 && row + 1 < m_list->count());
}

/*! \brief Builds the parameter fields of the selected step. A field writes its value through
 * setOperationParameters when the edit is done (Enter, focus out, arrows); the form is not rebuilt
 * then, so the focus stays. */
void TraceEditorWidget::updateForm()
{
    while (m_form->rowCount() > 0)
        m_form->removeRow(0);
    const int index = m_list->currentRow();
    if (index < 0 || index >= m_operations.size()) {
        m_parameters->setTitle(tr("Parameters"));
        m_form->addRow(new QLabel(tr("Select a step to edit it, or add one.")));
        return;
    }
    const TraceOp* op = m_operations.at(index);
    const QJsonObject json = op->toJson();
    m_parameters->setTitle(tr("Parameters of step %1").arg(index + 1));
    const int decimals = m_start->decimals();

    auto number = [this, index](const QString& key, double value, int decimals, double minimum) {
        QDoubleSpinBox* box = new QDoubleSpinBox;
        box->setKeyboardTracking(false);
        box->setDecimals(decimals);
        box->setRange(minimum, kLimit);
        box->setValue(value);
        connect(box, &QDoubleSpinBox::valueChanged, this, [this, index, key](double v) { setOperationParameters(index, QJsonObject{ { key, v } }); });
        return box;
    };
    auto integer = [this, index](const QString& key, int value, int minimum, int maximum) {
        QSpinBox* box = new QSpinBox;
        box->setKeyboardTracking(false);
        box->setRange(minimum, maximum);
        box->setValue(value);
        connect(box, &QSpinBox::valueChanged, this, [this, index, key](int v) { setOperationParameters(index, QJsonObject{ { key, v } }); });
        return box;
    };
    // rebuilding the form from a slot of one of its fields would delete the sender
    auto rebuild = [this]() { QMetaObject::invokeMethod(this, [this]() { updateForm(); }, Qt::QueuedConnection); };

    const QString type = op->type();
    if (type == "Range") {
        m_form->addRow(tr("From"), number("XStart", json["XStart"].toDouble(), decimals, -kLimit));
        m_form->addRow(tr("To"), number("XEnd", json["XEnd"].toDouble(), decimals, -kLimit));
        QPushButton* take = new QPushButton(tr("Use selected range"));
        connect(take, &QPushButton::clicked, this, [this, index, rebuild]() {
            setOperationParameters(index, QJsonObject{ { "XStart", rangeStart() }, { "XEnd", rangeEnd() } });
            rebuild();
        });
        m_form->addRow(take);
    } else if (type == "Smooth") {
        QSpinBox* window = integer("Window", json["Window"].toInt(), 1, 999);
        window->setToolTip(tr("Number of points of the moving polynomial fit; odd, larger than the order."));
        m_form->addRow(tr("Window (points)"), window);
        m_form->addRow(tr("Polynomial order"), integer("Order", json["Order"].toInt(), 0, 50));
    } else if (type == "Normalise") {
        QComboBox* mode = new QComboBox;
        mode->addItem(tr("Largest magnitude = 1"), "Max");
        mode->addItem(tr("Area = 1"), "Area");
        mode->addItem(tr("Minimum to maximum = 0 to 1"), "MinMax");
        mode->setCurrentIndex(mode->findData(json["Mode"].toString()));
        connect(mode, &QComboBox::currentIndexChanged, this, [this, index, mode]() { setOperationParameters(index, QJsonObject{ { "Mode", mode->currentData().toString() } }); });
        m_form->addRow(tr("Scale"), mode);
        const bool reference = json.contains("RefStart");
        QCheckBox* window = new QCheckBox(tr("Statistic from a reference window"));
        window->setChecked(reference);
        connect(window, &QCheckBox::toggled, this, [this, index, rebuild](bool on) {
            if (on)
                setOperationParameters(index, QJsonObject{ { "RefStart", rangeStart() }, { "RefEnd", rangeEnd() } });
            else
                setOperationParameters(index, QJsonObject{ { "RefStart", QJsonValue::Null }, { "RefEnd", QJsonValue::Null } });
            rebuild();
        });
        m_form->addRow(window);
        if (reference) {
            m_form->addRow(tr("Reference from"), number("RefStart", json["RefStart"].toDouble(), decimals, -kLimit));
            m_form->addRow(tr("Reference to"), number("RefEnd", json["RefEnd"].toDouble(), decimals, -kLimit));
        }
    } else if (type == "Baseline") {
        m_form->addRow(tr("Polynomial degree"), integer("Degree", json["Degree"].toInt(), 0, 10));
        QLineEdit* excluded = new QLineEdit(rangesToText(json["Excluded"].toArray()));
        excluded->setPlaceholderText(tr("e.g. 440 520; 600 620"));
        excluded->setToolTip(tr("x ranges left out of the baseline fit (the signals): two numbers per range, ranges separated by ';'."));
        connect(excluded, &QLineEdit::editingFinished, this, [this, index, excluded]() {
            QJsonArray ranges;
            if (!textToRanges(excluded->text(), &ranges)) {
                m_edit_error = tr("Step %1 not changed: excluded ranges need two numbers each, separated by ';'.").arg(index + 1);
                updateError();
                return;
            }
            setOperationParameters(index, QJsonObject{ { "Excluded", ranges } });
        });
        m_form->addRow(tr("Excluded ranges"), excluded);
        QPushButton* take = new QPushButton(tr("Exclude selected range"));
        connect(take, &QPushButton::clicked, this, [this, index, rebuild]() {
            const auto* base = dynamic_cast<const BaselineOp*>(m_operations.at(index));
            if (!base)
                return;
            QJsonArray ranges = base->toJson()["Excluded"].toArray();
            ranges.append(QJsonArray{ rangeStart(), rangeEnd() });
            setOperationParameters(index, QJsonObject{ { "Excluded", ranges } });
            rebuild();
        });
        m_form->addRow(take);
    } else if (type == "Blank") {
        // Claude Generated (Viewer 2)
        QLineEdit* blanked = new QLineEdit(rangesToText(json["Ranges"].toArray()));
        blanked->setObjectName("traceBlankRanges");
        blanked->setPlaceholderText(tr("e.g. 7.20 7.32; -0.05 0.05"));
        blanked->setToolTip(tr("x ranges replaced by a straight line between their neighbouring points (e.g. a standard or solvent signal): two numbers per range, ranges separated by ';'. A Normalisation that uses this signal must come before this step."));
        connect(blanked, &QLineEdit::editingFinished, this, [this, index, blanked]() {
            QJsonArray ranges;
            if (!textToRanges(blanked->text(), &ranges)) {
                m_edit_error = tr("Step %1 not changed: blanked ranges need two numbers each, separated by ';'.").arg(index + 1);
                updateError();
                return;
            }
            setOperationParameters(index, QJsonObject{ { "Ranges", ranges } });
        });
        m_form->addRow(tr("Ranges"), blanked);
        QPushButton* take = new QPushButton(tr("Blank the selected range"));
        take->setObjectName("traceBlankSelected");
        take->setToolTip(blanked->toolTip());
        connect(take, &QPushButton::clicked, this, [this, index, rebuild]() {
            const auto* blank = dynamic_cast<const BlankOp*>(m_operations.at(index));
            if (!blank)
                return;
            QJsonArray ranges = blank->toJson()["Ranges"].toArray();
            ranges.append(QJsonArray{ rangeStart(), rangeEnd() });
            setOperationParameters(index, QJsonObject{ { "Ranges", ranges } });
            rebuild();
        });
        m_form->addRow(take);
    } else if (type == "ShiftAxis") {
        QDoubleSpinBox* offset = number("Offset", json["Offset"].toDouble(), std::max(decimals, 4), -kLimit);
        offset->setToolTip(tr("Added to every x value, e.g. known minus observed position of a reference signal."));
        m_form->addRow(tr("Offset"), offset);
    } else if (type == "PickPeaks") {
        const bool relative = json["ThresholdMode"].toString() == "Relative";
        QDoubleSpinBox* threshold = number("Threshold", json["Threshold"].toDouble(), 4, -kLimit);
        threshold->setToolTip(tr("Relative: fraction of the y span above the minimum (0 to 1); absolute: y value."));
        m_form->addRow(tr("Threshold"), threshold);
        QCheckBox* rel = new QCheckBox(tr("Relative to the y span"));
        rel->setChecked(relative);
        connect(rel, &QCheckBox::toggled, this, [this, index](bool on) { setOperationParameters(index, QJsonObject{ { "ThresholdMode", on ? "Relative" : "Absolute" } }); });
        m_form->addRow(rel);
        m_form->addRow(tr("Minimal distance"), number("MinDistance", json["MinDistance"].toDouble(), decimals, 0));
        QCheckBox* integrate = new QCheckBox(tr("Integrate peaks"));
        integrate->setChecked(json["Integrate"].toBool());
        connect(integrate, &QCheckBox::toggled, this, [this, index](bool on) { setOperationParameters(index, QJsonObject{ { "Integrate", on } }); });
        m_form->addRow(integrate);
    }
}

void TraceEditorWidget::setError(const QString& error)
{
    m_edit_error = error;
    updateError();
}

void TraceEditorWidget::updateError()
{
    const QString text = !m_edit_error.isEmpty() ? m_edit_error : m_error;
    m_error_label->setText(text);
    m_error_label->setHidden(text.isEmpty());
    // a colour with a meaning: red, readable on the window colour of either scheme
    QPalette palette = m_error_label->palette();
    palette.setColor(QPalette::WindowText, Appearance::IsDark() ? QColor(255, 120, 110) : QColor(180, 30, 20));
    m_error_label->setPalette(palette);
}

/*! \brief Draws every trace (raw, or processed if "Show processed" is ticked), the peak markers of a
 * PickPeaks step on the processed traces and the limits of the selected range. The processing runs
 * for every trace in either view, so an error of a step is reported also while the raw data are shown.
 * The user's zoom is kept (setTraces decides whether the traces are the same); the stack offsets are
 * applied to the points of the series. */
void TraceEditorWidget::updateChart()
{
    m_chart->clearChart();
    m_colors.clear();
    m_peaks.clear();
    m_shown.clear();
    m_lines.clear();
    m_peak_series.clear();
    m_error.clear();
    const bool processed = m_processed->isChecked();

    for (int i = 0; i < m_traces.size(); ++i) {
        const XYTrace& raw = m_traces.at(i);
        XYTrace shown = raw;
        QVector<TracePeak> peaks;
        if (!m_operations.isEmpty()) {
            QString error;
            XYTrace result = m_operations.apply(raw, &error, &peaks);
            if (!error.isEmpty() && m_error.isEmpty())
                m_error = tr("%1: %2").arg(raw.name().isEmpty() ? tr("Trace %1").arg(i + 1) : raw.name(), error);
            if (processed)
                shown = result;
        }
        m_peaks << peaks;
        m_shown << shown;
    }
    updateStackReference();

    for (int i = 0; i < m_shown.size(); ++i) {
        LineSeries* series = new LineSeries;
        series->append(shiftedPoints(i));
        m_chart->addSeries(series, false);
        // the chart gives the theme colour to the pen; LineSeries::color() only knows a colour set by setColor
        const QColor color = series->pen().color();
        series->setColor(color);
        series->setLineWidth(i == m_highlight ? 2 : 1);
        m_colors << color;
        m_lines << series;
    }
    // markers after all lines, so the lines keep the colours of the raw view
    for (int i = 0; i < m_peaks.size(); ++i) {
        if (!processed || m_peaks[i].isEmpty()) {
            m_peak_series << nullptr;
            continue;
        }
        ScatterSeries* markers = new ScatterSeries;
        const QPointF shift = stackOffset(i);
        for (const TracePeak& peak : m_peaks[i])
            markers->append(peak.x + shift.x(), peak.height + shift.y());
        markers->setMarkerSize(8);
        m_chart->addSeries(markers, false);
        markers->setColor(m_colors[i]);
        m_peak_series << markers;
    }
    const bool stacked = isStacked();
    for (int i = 0; i < m_lines.size(); ++i) {
        const bool visible = !(stacked && isExcluded(i));
        if (!visible) {
            m_lines[i]->setVisible(false);
            if (m_peak_series[i])
                m_peak_series[i]->setVisible(false);
        }
    }
    if (!m_traces.isEmpty()) {
        m_lower = new LineSeries;
        m_upper = new LineSeries;
        for (LineSeries* limit : { m_lower.data(), m_upper.data() }) {
            m_chart->addSeries(limit, false);
            QColor color = palette().color(QPalette::Text);
            color.setAlpha(150); // visible on light and dark chart backgrounds
            limit->setColor(color);
            limit->setDashDotLine(true);
        }
        updateRangeLines();
    }
    if (m_chart->axisY())
        m_chart->axisY()->setLabelsVisible(m_stack_y->value() == 0);
    applyAxisDirection();
    updateError();
    refreshMarkers();
    emit chartUpdated();
    applyView(); // the series of the host rescaled the axes
}

/*! \brief Moves the drawn series to the current stack offsets: the points are replaced, the series stay
 * (colours, the host's series and the processing are untouched). The host is told by chartUpdated() to
 * move its own series. Claude Generated (Viewer 2) */
void TraceEditorWidget::applyStack()
{
    if (m_lines.isEmpty())
        return;
    const bool stacked = isStacked();
    for (int i = 0; i < m_lines.size(); ++i) {
        if (!m_lines[i])
            continue;
        const bool visible = !(stacked && isExcluded(i));
        m_lines[i]->replace(shiftedPoints(i));
        m_lines[i]->setVisible(visible);
        if (i < m_peak_series.size() && m_peak_series[i]) {
            const QPointF shift = stackOffset(i);
            QList<QPointF> points;
            for (const TracePeak& peak : m_peaks[i])
                points << QPointF(peak.x + shift.x(), peak.height + shift.y());
            m_peak_series[i]->replace(points);
            m_peak_series[i]->setVisible(visible);
        }
    }
    updateRangeLines();
    if (m_chart->axisY())
        m_chart->axisY()->setLabelsVisible(m_stack_y->value() == 0);
    if (!m_user_view)
        m_chart->formatAxis(); // whole stack in view
    refreshMarkers();
    emit chartUpdated();
    applyView();
}

int TraceEditorWidget::stackLevel(int traceIndex) const
{
    if (traceIndex < 0 || traceIndex >= m_traces.size() || isExcluded(traceIndex))
        return -1;
    int level = 0, count = 0;
    for (int i = 0; i < m_traces.size(); ++i) {
        if (isExcluded(i))
            continue;
        if (i < traceIndex)
            ++level;
        ++count;
    }
    return m_stack_reverse->isChecked() ? count - 1 - level : level;
}

bool TraceEditorWidget::isExcluded(int traceIndex) const
{
    return traceIndex >= 0 && traceIndex < m_excluded.size() && m_excluded[traceIndex];
}

bool TraceEditorWidget::isStacked() const
{
    return m_stack_y->value() != 0 || m_stack_x->value() != 0;
}

QPointF TraceEditorWidget::stackOffset(int traceIndex) const
{
    const int level = stackLevel(traceIndex);
    if (level <= 0)
        return QPointF(0, 0);
    const double dy = m_stack_y->value() / 100.0 * m_stack_reference;
    // to the right on the screen: towards lower x on a reversed axis
    const double dx = m_stack_x->value() / 100.0 * std::abs(rangeEnd() - rangeStart()) * (isAxisReversed() ? -1.0 : 1.0);
    return QPointF(level * dx, level * dy);
}

/*! \brief Largest max - min of a drawn trace (not excluded) within Start..End; 1 if there is none (flat or
 * no point in the range), so the offset never collapses to 0. */
void TraceEditorWidget::updateStackReference()
{
    const double lo = rangeStart(), hi = rangeEnd();
    double reference = 0;
    for (int i = 0; i < m_shown.size(); ++i) {
        if (isExcluded(i))
            continue;
        const XYTrace& trace = m_shown[i];
        double ymin = std::numeric_limits<double>::infinity(), ymax = -ymin;
        for (Eigen::Index k = 0; k < trace.size(); ++k) {
            const double x = trace.x()(k);
            if (x < lo || x > hi)
                continue;
            ymin = std::min(ymin, trace.y()(k));
            ymax = std::max(ymax, trace.y()(k));
        }
        if (ymax > ymin)
            reference = std::max(reference, ymax - ymin);
    }
    m_stack_reference = reference > 0 && std::isfinite(reference) ? reference : 1;
}

QList<QPointF> TraceEditorWidget::shiftedPoints(int traceIndex) const
{
    const XYTrace& trace = m_shown[traceIndex];
    const QPointF shift = stackOffset(traceIndex);
    QList<QPointF> points;
    points.reserve(trace.size());
    for (Eigen::Index k = 0; k < trace.size(); ++k)
        points.append(QPointF(trace.x()(k) + shift.x(), trace.y()(k) + shift.y()));
    return points;
}

/*! \brief The selected range moved: the reference height of the stack follows (not while setTraces sets it). */
void TraceEditorWidget::rangeMoved()
{
    if (m_updating)
        return;
    const double reference = m_stack_reference;
    updateStackReference();
    const bool x_moves = m_stack_x->value() != 0; // the x offset is a share of the range
    if (isStacked() && (reference != m_stack_reference || x_moves))
        applyStack();
    else
        updateRangeLines();
}

void TraceEditorWidget::setExcludedTraces(const QVector<bool>& excluded)
{
    if (excluded == m_excluded)
        return;
    m_excluded = excluded;
    updateStackReference();
    if (isStacked())
        applyStack();
}

void TraceEditorWidget::setHighlightedTrace(int index)
{
    if (index == m_highlight)
        return;
    m_highlight = index;
    updateLineWidths();
}

void TraceEditorWidget::updateLineWidths()
{
    for (int i = 0; i < m_lines.size(); ++i)
        if (m_lines[i])
            m_lines[i]->setLineWidth(i == m_highlight ? 2 : 1);
}

double TraceEditorWidget::stackPercent() const
{
    return m_stack_y->value();
}

void TraceEditorWidget::setStackPercent(double percent)
{
    m_stack_y->setValue(percent); // valueChanged moves the series
}

double TraceEditorWidget::stackXPercent() const
{
    return m_stack_x->value();
}

void TraceEditorWidget::setStackXPercent(double percent)
{
    m_stack_x->setValue(percent);
}

bool TraceEditorWidget::stackReversed() const
{
    return m_stack_reverse->isChecked();
}

void TraceEditorWidget::setStackReversed(bool reversed)
{
    m_stack_reverse->setChecked(reversed);
}

void TraceEditorWidget::setSettingsKey(const QString& key)
{
    m_settings_key = key;
    if (key.isEmpty())
        return;
    QSettings settings;
    const QString group = QStringLiteral("traceeditor/%1/").arg(key);
    for (QWidget* box : std::initializer_list<QWidget*>{ m_stack_y, m_stack_x, m_stack_reverse })
        box->blockSignals(true);
    m_stack_y->setValue(settings.value(group + "stackY", 0.0).toDouble());
    m_stack_x->setValue(settings.value(group + "stackX", 0.0).toDouble());
    m_stack_reverse->setChecked(settings.value(group + "stackReverse", false).toBool());
    for (QWidget* box : std::initializer_list<QWidget*>{ m_stack_y, m_stack_x, m_stack_reverse })
        box->blockSignals(false);
    applyStack();
}

void TraceEditorWidget::saveStackSettings()
{
    if (m_settings_key.isEmpty())
        return;
    QSettings settings;
    const QString group = QStringLiteral("traceeditor/%1/").arg(m_settings_key);
    settings.setValue(group + "stackY", m_stack_y->value());
    settings.setValue(group + "stackX", m_stack_x->value());
    settings.setValue(group + "stackReverse", m_stack_reverse->isChecked());
}

/*! \brief x range of the whole view: the common range of the traces widened by the x stack offsets. */
void TraceEditorWidget::homeX(double* lo, double* hi) const
{
    double a = 0, b = 0;
    if (!m_traces.commonRange(&a, &b)) {
        *lo = *hi = 0;
        return;
    }
    double dmin = 0, dmax = 0;
    for (int i = 0; i < m_traces.size(); ++i) {
        const double dx = stackOffset(i).x();
        dmin = std::min(dmin, dx);
        dmax = std::max(dmax, dx);
    }
    *lo = a + dmin;
    *hi = b + dmax;
}

/*! \brief Sets the axes to the user's zoom, or x to the whole range (y as scaled by the chart). */
void TraceEditorWidget::applyView()
{
    QValueAxis* x = m_chart->axisX();
    QValueAxis* y = m_chart->axisY();
    if (!x || !y || m_traces.isEmpty())
        return;
    if (m_user_view) {
        x->setRange(m_view_x0, m_view_x1);
        y->setRange(m_view_y0, m_view_y1);
    } else {
        double lo = 0, hi = 0;
        homeX(&lo, &hi);
        if (hi > lo)
            x->setRange(lo, hi);
    }
    m_chart->privateView()->updateZoom();
}

void TraceEditorWidget::captureView()
{
    QValueAxis* x = m_chart->axisX();
    QValueAxis* y = m_chart->axisY();
    if (!x || !y)
        return;
    m_view_x0 = x->min();
    m_view_x1 = x->max();
    m_view_y0 = y->min();
    m_view_y1 = y->max();
    m_user_view = true;
}

void TraceEditorWidget::resetView()
{
    m_user_view = false;
    m_chart->formatAxis();
    applyView();
    refreshMarkers();
}

void TraceEditorWidget::fitYToVisible()
{
    QValueAxis* x = m_chart->axisX();
    QValueAxis* y = m_chart->axisY();
    if (!x || !y)
        return;
    const double lo = x->min(), hi = x->max();
    double ymin = std::numeric_limits<double>::infinity(), ymax = -ymin;
    for (int i = 0; i < m_shown.size(); ++i) {
        if (!m_lines.value(i) || !m_lines[i]->isVisible())
            continue;
        const QPointF shift = stackOffset(i);
        const XYTrace& trace = m_shown[i];
        for (Eigen::Index k = 0; k < trace.size(); ++k) {
            const double px = trace.x()(k) + shift.x();
            if (px < lo || px > hi)
                continue;
            ymin = std::min(ymin, trace.y()(k) + shift.y());
            ymax = std::max(ymax, trace.y()(k) + shift.y());
        }
    }
    if (!std::isfinite(ymin) || !std::isfinite(ymax))
        return;
    double margin = 0.05 * (ymax - ymin);
    if (!(margin > 0))
        margin = std::max(1e-12, 0.05 * std::abs(ymax));
    y->setRange(ymin - margin, ymax + margin);
    captureView();
    refreshMarkers();
}

void TraceEditorWidget::scaleY(double value, double factor)
{
    QValueAxis* y = m_chart->axisY();
    if (!y || !(factor > 0) || !std::isfinite(value))
        return;
    const double min = y->min(), max = y->max();
    const double anchor = std::clamp(value, min, max); // outside the plot: scale about the nearer edge
    y->setRange(anchor - (anchor - min) * factor, anchor + (max - anchor) * factor);
    captureView();
    refreshMarkers();
}

bool TraceEditorWidget::isAxisReversed() const
{
    switch (m_axis_direction) {
    case AxisDirection::Reversed:
        return true;
    case AxisDirection::Normal:
        return false;
    case AxisDirection::Auto:
        break;
    }
    return !m_traces.isEmpty() && m_traces.at(0).orientation() == XYTrace::Orientation::Descending;
}

void TraceEditorWidget::setAxisDirection(AxisDirection direction)
{
    if (direction == m_axis_direction)
        return;
    m_axis_direction = direction;
    applyAxisDirection();
    if (m_stack_x->value() != 0)
        applyStack(); // "to the right" is the other x direction now
    refreshMarkers();
}

/*! \brief Sets the reverse flag of the x axis; series, range limits and zoom follow the axis. Claude Generated */
void TraceEditorWidget::applyAxisDirection()
{
    if (m_chart->axisX())
        m_chart->axisX()->setReverse(isAxisReversed());
}

bool TraceEditorWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_chart->privateView() && event->type() == QEvent::Resize)
        QMetaObject::invokeMethod(this, [this]() { refreshMarkers(); }, Qt::QueuedConnection); // after the chart laid out its plot area
    if (watched == m_chart->privateView()->viewport()) {
        // Claude Generated (Viewer 2): the wheel scales y at the pointer; the middle button shows everything
        if (event->type() == QEvent::Wheel) {
            auto* wheel = static_cast<QWheelEvent*>(event);
            const double steps = wheel->angleDelta().y() / 120.0;
            if (steps != 0 && !m_traces.isEmpty()) {
                QChartView* view = m_chart->privateView();
                const QPointF value = view->chart()->mapToValue(view->chart()->mapFromScene(view->mapToScene(wheel->position().toPoint())));
                scaleY(value.y(), std::pow(0.9, steps));
            }
            return true;
        }
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::MiddleButton) {
                if (event->type() == QEvent::MouseButtonRelease)
                    resetView();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void TraceEditorWidget::refreshMarkers()
{
    if (m_user_view) // a series added by the host rescaled the axes; without a zoom of the user they stay as scaled
        applyView();
    m_chart->privateView()->updateZoom(); // y span of the axes, the height of the lines
    m_chart->privateView()->setVerticalLinesPrec(-1); // places the lines again
}

void TraceEditorWidget::updateRangeLines()
{
    if (!m_lower || !m_upper)
        return;
    // the drawn extent (with stack offsets), so the limits span the whole stack
    double ymin = std::numeric_limits<double>::infinity(), ymax = -ymin;
    for (int i = 0; i < m_shown.size(); ++i) {
        if (!m_lines.value(i) || !m_lines[i]->isVisible() || m_shown[i].isEmpty())
            continue;
        const double dy = stackOffset(i).y();
        ymin = std::min(ymin, m_shown[i].y().minCoeff() + dy);
        ymax = std::max(ymax, m_shown[i].y().maxCoeff() + dy);
    }
    m_y_min = std::isfinite(ymin) ? ymin : 0;
    m_y_max = std::isfinite(ymax) ? ymax : 1;
    m_lower->replace(QList<QPointF>{ QPointF(rangeStart(), m_y_min), QPointF(rangeStart(), m_y_max) });
    m_upper->replace(QList<QPointF>{ QPointF(rangeEnd(), m_y_min), QPointF(rangeEnd(), m_y_max) });
}
