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

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QVector>
#include <QtGui/QColor>
#include <QtWidgets/QWidget>

#include "src/core/trace/traceops.h"
#include "src/core/trace/traceset.h"

class ChartView;
class LineSeries;

class QCheckBox;
class QDoubleSpinBox;
class QFormLayout;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QToolButton;
class StatusLine;

/*! \brief Shows a TraceSet (raw or processed by a TraceOpList), lets the user select an x range and
 * edit the processing steps (Claude Generated).
 *
 * The widget renders only: the science (the steps, their validation and the processing) lives in
 * TraceOpList/TraceOps of the core. It holds a copy of the traces to draw and the operation list it
 * edits; every change of the list emits operationsChanged(), the host reads operations() and stores
 * it where the data live (e.g. SpectraHandler::setOperations). Errors of the core validation are
 * shown in the widget (errorText()), the list is then left unchanged.
 *
 * The x range (spin boxes "Start"/"End", or a right-button drag in the chart) is a selection of the
 * host (rangeChanged()); it is not a processing step. Descending axes (NMR ppm) are drawn reversed.
 * The host can put own controls into the tool row (addToolWidget) and use chart() for its own
 * interaction (double-click, vertical lines).
 *
 * A one-line hint under the chart names the mouse actions of the chart and, set by the host
 * (setDoubleClickHint), what a double-click does; it is shown with the setting "thermogram_guidelines"
 * ("Show hints in the import dialogs"), like the guide line of the thermogram dialog. The chart has no
 * modes of its own (the wheel does nothing here), so the line does not change while working.
 */
class TraceEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit TraceEditorWidget(QWidget* parent = nullptr);

    /*! \brief Traces to show (copied for drawing); the range limits follow their common x range. */
    void setTraces(const TraceSet& traces);
    inline const TraceSet& traces() const { return m_traces; }

    /*! \brief Replaces the edited list; no operationsChanged(). */
    void setOperations(const TraceOpList& operations);
    inline const TraceOpList& operations() const { return m_operations; }

    /*! \brief Appends a step of this JSON "Type" (Range, Smooth, Normalise, Baseline, ShiftAxis, PickPeaks) with
     * default parameters (a Range takes the selected range); returns its index or -1. */
    int addOperation(const QString& type);
    bool removeOperation(int index);
    bool moveOperation(int from, int to);
    bool setOperationEnabled(int index, bool enabled);
    /*! \brief Sets parameters of step index: the keys of parameters replace those of the step's JSON
     * (a null value removes the key), the result is read by TraceOp::fromJson. On an invalid
     * parameter the step stays as it was, errorText() says why and false is returned. */
    bool setOperationParameters(int index, const QJsonObject& parameters);

    /*! \brief Message of the last failed edit or of the first trace whose processing fails; empty if none. */
    inline QString errorText() const { return m_edit_error.isEmpty() ? m_error : m_edit_error; }

    /*! \brief Sets the selected range (no rangeChanged()). */
    void setRange(double start, double end);
    double rangeStart() const;
    double rangeEnd() const;

    void setShowProcessed(bool processed);
    bool showProcessed() const;

    /*! \brief Peaks of the last PickPeaks step per trace (empty without such a step). */
    inline const QVector<QVector<TracePeak>>& peaks() const { return m_peaks; }
    /*! \brief Colour of each trace in the chart (order of traces()). */
    inline const QList<QColor>& traceColors() const { return m_colors; }

    inline ChartView* chart() const { return m_chart; }
    /*! \brief Adds a control of the host to the tool row above the chart. */
    void addToolWidget(QWidget* widget);

    /*! \brief What a double-click in the chart does for the host, for the hint line (empty: not mentioned). Claude Generated */
    void setDoubleClickHint(const QString& text);
    /*! \brief Text of the one-line hint under the chart (mouse and keys of the chart). Claude Generated */
    QString hintText() const;
    /*! \brief True if the import dialogs show hint lines: the setting "thermogram_guidelines" (on when not set). Claude Generated */
    static bool hintsEnabled();

    /*! \brief One-line description of a step for the list. */
    static QString operationLabel(const TraceOp* op);

    /*! \brief Draws the traces again (raw or processed) with peaks and range limits. */
    void updateChart();
    /*! \brief Places the vertical marker lines of the chart (x values of the host) again (call after adding them); the chart
     * computes their position only when they are added. */
    void refreshMarkers();

signals:
    void operationsChanged();
    void rangeChanged(double start, double end);
    /*! \brief The chart was drawn again (its series were replaced); a host adds its own series here. */
    void chartUpdated();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setupUi();
    void updateList();
    void updateForm();
    void updateError();
    void setError(const QString& error);
    void listChanged(int select);
    void selectionToRange(double a, double b);
    void updateRangeLines();
    void updateHint();

    TraceSet m_traces;
    TraceOpList m_operations;
    QVector<QVector<TracePeak>> m_peaks;
    QList<QColor> m_colors;
    QString m_error, m_edit_error;
    bool m_updating = false;
    double m_y_min = 0, m_y_max = 0; ///< y span of the drawn traces, height of the range limits

    ChartView* m_chart;
    QDoubleSpinBox *m_start, *m_end;
    QCheckBox* m_processed;
    QPushButton* m_zoom;
    QHBoxLayout* m_tools;
    QWidget* m_tool_row;
    QListWidget* m_list;
    QToolButton* m_add;
    QPushButton *m_remove, *m_up, *m_down;
    QGroupBox* m_parameters;
    QFormLayout* m_form;
    QLabel* m_error_label;
    StatusLine* m_hint; ///< one-line hint on the mouse and keys of the chart (setting "thermogram_guidelines")
    QString m_double_click_hint;
    QPointer<LineSeries> m_lower, m_upper; ///< dashed limits of the selected range
};
