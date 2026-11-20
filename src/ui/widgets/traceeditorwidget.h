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
 * host (rangeChanged()); it is not a processing step. The screen direction of the x axis is set by
 * setAxisDirection(): Auto draws traces stored in descending order reversed (NMR ppm from the file),
 * Reversed/Normal fix the direction whatever the order of the data. Everything that maps between
 * pixels and x values (range, zoom, markers, lines of the host) goes through the chart axis, so it
 * works in either direction.
 * The host can put own controls into the tool row (addToolWidget) and use chart() for its own
 * interaction (double-click, vertical lines).
 *
 * A one-line hint under the chart names the mouse actions of the chart and, set by the host
 * (setDoubleClickHint), what a double-click does; it is shown with the setting "thermogram_guidelines"
 * ("Show hints in the import dialogs"), like the guide line of the thermogram dialog. The chart has no
 * modes of its own, so the line does not change while working.
 *
 * View (Claude Generated, Viewer 2): the zoom of the user (left drag, wheel, "Fit y to the visible
 * range", "Zoom to range") is kept over every redraw with the same traces (other processing steps,
 * stack offsets, axis direction, raw/processed, series added by the host followed by refreshMarkers());
 * new or other traces (other ids) go back to the whole range, as does the middle button. When the axis of
 * the same traces moved as a whole (a referencing offset), range and zoom move along by the same amount.
 * The wheel scales y about the value under the pointer (one notch: span x 0.9 in, / 0.9 out).
 *
 * Stacking (view only, the data are unchanged): trace k is drawn moved by stackOffset(k). Its level L is
 * its place among the traces that are not excluded (setExcludedTraces), counted in list order from the
 * bottom (first trace at the bottom), or from the top with "Reverse order". Level L is moved up by
 * L x "Stack offset" % of the reference height (the largest max - min of a shown trace, not excluded,
 * within the selected range Start..End; computed again only when range, traces, steps or exclusions
 * change, never by zooming) and to the right on the screen by L x "x offset" % of the span End - Start:
 * on a reversed axis "right" is towards lower x, so the x shift in data units is negative there. With a
 * stack offset excluded traces are not drawn and the y labels are hidden (the ticks then mean nothing).
 * The settings are remembered per host (setSettingsKey, QSettings "traceeditor/<key>/stackY|stackX|
 * stackReverse"), never in a project.
 */
class TraceEditorWidget : public QWidget {
    Q_OBJECT
public:
    /*! \brief Screen direction of the x axis (Claude Generated).
     * Auto: reversed if the traces are stored in descending x order; Reversed: high x on the left
     * (chemical shift as usual for 1H NMR); Normal: low x on the left. */
    enum class AxisDirection {
        Auto,
        Reversed,
        Normal
    };

    explicit TraceEditorWidget(QWidget* parent = nullptr);

    /*! \brief Sets the direction of the x axis and redraws the axis, the range limits and the marker lines. Claude Generated */
    void setAxisDirection(AxisDirection direction);
    inline AxisDirection axisDirection() const { return m_axis_direction; }
    /*! \brief True if the x axis runs from high (left) to low (right) for the current direction and traces. Claude Generated */
    bool isAxisReversed() const;

    /*! \brief Traces to show (copied for drawing); the range limits follow their common x range. */
    void setTraces(const TraceSet& traces);
    inline const TraceSet& traces() const { return m_traces; }

    /*! \brief Replaces the edited list; no operationsChanged(). */
    void setOperations(const TraceOpList& operations);
    inline const TraceOpList& operations() const { return m_operations; }

    /*! \brief Appends a step of this JSON "Type" (Range, Smooth, Normalise, Baseline, ShiftAxis, Blank, PickPeaks) with
     * default parameters (a Range takes the selected range, a Blank the selected range unless it is the whole
     * range); returns its index or -1. */
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
     * computes their position only when they are added. Restores the user's zoom, if there is one: a series added by
     * the host rescales the axes. */
    void refreshMarkers();

    /*! \brief QSettings group of the stack settings of this host ("traceeditor/<key>/..."); reads them. Empty: not stored. Claude Generated */
    void setSettingsKey(const QString& key);
    /*! \brief Stack offset in % of the reference height (0: not stacked in y). Claude Generated */
    double stackPercent() const;
    void setStackPercent(double percent);
    /*! \brief Extra x offset per level in % of the selected x span (screen direction: right). Claude Generated */
    double stackXPercent() const;
    void setStackXPercent(double percent);
    /*! \brief Reverse order: the first trace on top. Claude Generated */
    bool stackReversed() const;
    void setStackReversed(bool reversed);
    /*! \brief True if a stack offset (y or x) is set. Claude Generated */
    bool isStacked() const;
    /*! \brief Shift (x, y in data units) with which trace traceIndex is drawn; (0, 0) when not stacked or excluded.
     * A host draws its own series of that trace (fit, residual, markers) with it. Claude Generated */
    QPointF stackOffset(int traceIndex) const;
    /*! \brief Height the y stack offset refers to (largest max - min within the selected range). Claude Generated */
    inline double stackReferenceHeight() const { return m_stack_reference; }
    /*! \brief Traces left out by the host (true = excluded): they do not count for the stack and are not drawn when
     * stacked. Claude Generated */
    void setExcludedTraces(const QVector<bool>& excluded);
    /*! \brief Draws trace index wider (-1: none), e.g. the one whose fit the host shows. Claude Generated */
    void setHighlightedTrace(int index);

    /*! \brief Sets y to min/max of the drawn traces (with stack offsets) within the visible x range plus 5 %. Claude Generated */
    void fitYToVisible();
    /*! \brief Whole range in x, y scaled to all series; the zoom of the user is dropped (middle button). Claude Generated */
    void resetView();
    /*! \brief Scales y by factor about the value y (wheel); factor < 1 zooms in. Claude Generated */
    void scaleY(double y, double factor);
    /*! \brief True while the user's zoom is kept (left drag, wheel, fit y, zoom to range). Claude Generated */
    inline bool hasUserView() const { return m_user_view; }

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
    void applyAxisDirection();
    // Claude Generated (Viewer 2): stacking and the kept view
    int stackLevel(int traceIndex) const;
    bool isExcluded(int traceIndex) const;
    void updateStackReference();
    /*! \brief Moves the drawn series to the current stack offsets (no new series, no processing). */
    void applyStack();
    void rangeMoved();
    QList<QPointF> shiftedPoints(int traceIndex) const;
    void homeX(double* lo, double* hi) const;
    void applyView();
    void captureView();
    void saveStackSettings();
    void updateLineWidths();

    TraceSet m_traces;
    TraceOpList m_operations;
    QVector<QVector<TracePeak>> m_peaks;
    QVector<XYTrace> m_shown; ///< the drawn traces (raw or processed) without stack offsets
    QList<QPointer<LineSeries>> m_lines; ///< series of the traces (order of traces())
    QList<QPointer<ScatterSeries>> m_peak_series; ///< peak markers per trace (null without peaks)
    QList<QColor> m_colors;
    QString m_error, m_edit_error;
    bool m_updating = false;
    AxisDirection m_axis_direction = AxisDirection::Auto;
    double m_y_min = 0, m_y_max = 0; ///< y span of the drawn traces, height of the range limits
    double m_stack_reference = 0;
    QVector<bool> m_excluded;
    int m_highlight = -1;
    QString m_settings_key;
    bool m_user_view = false; ///< the axes below are the user's zoom, kept over redraws
    double m_view_x0 = 0, m_view_x1 = 0, m_view_y0 = 0, m_view_y1 = 0;

    ChartView* m_chart;
    QDoubleSpinBox *m_start, *m_end;
    QDoubleSpinBox *m_stack_y, *m_stack_x;
    QCheckBox* m_stack_reverse;
    QPushButton* m_fit_y;
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
