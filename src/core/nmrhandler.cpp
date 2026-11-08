/*
 * NmrHandler - series of 1D NMR spectra of a titration and the chemical shifts of tracked signals
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

// Claude Generated: WP3 of roadmap/input_data_editing.md

#include <algorithm>
#include <cmath>

#include <QtCore/QCollator>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QVariant>

#include "src/core/models/datatable.h"
#include "src/core/trace/traceio.h"
#include "src/core/trace/tracejson.h"
#include "src/version.h"

#include "nmrhandler.h"

namespace {
const double kNaN = std::numeric_limits<double>::quiet_NaN();
const char* kProfile = "nmr";

void setError(QString* error, const QString& message)
{
    if (error)
        *error = message;
}

/// tracking state of one signal: the last two known positions and their spectrum index
struct TrackState {
    double last = 0, prev = 0;
    double height = std::numeric_limits<double>::quiet_NaN(); ///< height of the last peak found (NaN: none yet)
    int lastIndex = -1, prevIndex = -1;

    /// linear extrapolation of the last two known positions to spectrum k (the last position if only one is known)
    double predict(int k) const
    {
        if (prevIndex < 0 || lastIndex < 0 || prevIndex == lastIndex)
            return last;
        return last + (last - prev) / double(lastIndex - prevIndex) * double(k - lastIndex);
    }
    void update(double position, int k)
    {
        if (k != lastIndex) {
            prev = last;
            prevIndex = lastIndex;
        }
        last = position;
        lastIndex = k;
    }
};
} // namespace

// ---------------------------------------------------------------------------------------------
// spectra

bool NmrHandler::addFile(const QString& filename, QString* error)
{
    QVector<XYTrace> traces;
    if (!TraceIO::readTraceFile(filename, &traces, error))
        return false;
    for (int i = 0; i < traces.size(); ++i) {
        const QString id = m_spectra.add(traces[i]);
        m_columns[id] = i + 1;
    }
    return true;
}

bool NmrHandler::addDirectory(const QString& directory, const QString& suffix, QString* error)
{
    QDir dir(directory);
    if (!dir.exists()) {
        setError(error, QString("directory %1 does not exist").arg(directory));
        return false;
    }
    QStringList files;
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files | QDir::Readable)) {
        if (suffix.isEmpty() || info.fileName().endsWith(suffix, Qt::CaseInsensitive))
            files << info.absoluteFilePath();
    }
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(files.begin(), files.end(), [&collator](const QString& a, const QString& b) { return collator.compare(a, b) < 0; });
    QStringList failed;
    int added = 0;
    for (const QString& file : files) {
        QString reason;
        if (addFile(file, &reason))
            ++added;
        else
            failed << reason;
    }
    if (files.isEmpty()) {
        setError(error, QString("no file%1 in %2").arg(suffix.isEmpty() ? QString() : QString(" ending with %1").arg(suffix), directory));
        return false;
    }
    if (!failed.isEmpty())
        setError(error, failed.join("\n"));
    return added > 0;
}

QString NmrHandler::addSpectrum(const XYTrace& spectrum)
{
    const QString id = m_spectra.add(spectrum);
    m_columns[id] = 1;
    return id;
}

bool NmrHandler::removeSpectrum(int index)
{
    if (index < 0 || index >= m_spectra.size())
        return false;
    const QString id = m_spectra.idAt(index);
    m_spectra.remove(id);
    m_columns.remove(id);
    m_manual.remove(id);
    m_excluded.remove(id);
    if (index < m_reference)
        --m_reference; // the reference stays the same spectrum
    if (m_reference >= m_spectra.size())
        m_reference = std::max(0, m_spectra.size() - 1);
    return true;
}

bool NmrHandler::moveSpectrum(int from, int to)
{
    const QString reference = m_spectra.idAt(m_reference);
    if (!m_spectra.move(from, to))
        return false;
    m_reference = std::max(0, m_spectra.indexOf(reference)); // the reference stays the same spectrum
    return true;
}

void NmrHandler::clear()
{
    m_spectra.clear();
    m_columns.clear();
    m_manual.clear();
    m_excluded.clear();
    m_cells.clear();
    m_reference = 0;
}

// ---------------------------------------------------------------------------------------------
// processing

void NmrHandler::setReferenceOffset(double offset, bool moveSignals)
{
    if (!std::isfinite(offset))
        return;
    const double delta = offset - m_reference_offset;
    m_reference_offset = offset;
    if (!moveSignals || delta == 0)
        return;
    for (NmrSignal& signal : m_signals)
        signal.position += delta;
    for (auto& row : m_manual)
        for (double& value : row)
            value += delta;
}

TraceSet NmrHandler::referencedSpectra() const
{
    TraceSet result;
    const ShiftAxisOp shift(m_reference_offset);
    for (int i = 0; i < m_spectra.size(); ++i)
        result.add(shift.apply(m_spectra.at(i)), m_spectra.idAt(i));
    return result;
}

TraceSet NmrHandler::processedSpectra(QString* error) const
{
    const TraceSet referenced = referencedSpectra();
    if (m_operations.isEmpty())
        return referenced;
    TraceSet processed;
    QString first;
    for (int i = 0; i < referenced.size(); ++i) {
        QString reason;
        XYTrace trace = m_operations.apply(referenced.at(i), &reason);
        if (!reason.isEmpty()) {
            if (first.isEmpty())
                first = QString("%1: %2").arg(referenced.at(i).name(), reason);
            trace = XYTrace();
            trace.setName(referenced.at(i).name());
        }
        processed.add(trace, referenced.idAt(i));
    }
    setError(error, first);
    return processed;
}

/*! \brief Peaks of every spectrum: those of the last enabled PickPeaks step of the processing, or of
 * the default picking (relative threshold 0.05) on the processed spectrum if there is no such step. */
QVector<QVector<TracePeak>> NmrHandler::peaks(QString* error) const
{
    bool hasPick = false;
    for (int i = 0; i < m_operations.size(); ++i)
        hasPick = hasPick || (m_operations.at(i)->enabled && m_operations.at(i)->type() == "PickPeaks");

    const TraceSet referenced = referencedSpectra();
    QVector<QVector<TracePeak>> result(referenced.size());
    QString first;
    PickPeaksOp fallback;
    fallback.threshold = 0.05;
    for (int i = 0; i < referenced.size(); ++i) {
        QString reason;
        QVector<TracePeak> found;
        const XYTrace processed = m_operations.apply(referenced.at(i), &reason, &found);
        if (reason.isEmpty() && !hasPick)
            found = fallback.pick(processed, &reason);
        if (!reason.isEmpty() && first.isEmpty())
            first = QString("%1: %2").arg(referenced.at(i).name(), reason);
        result[i] = reason.isEmpty() ? found : QVector<TracePeak>();
    }
    setError(error, first);
    return result;
}

// ---------------------------------------------------------------------------------------------
// signals

void NmrHandler::setReferenceIndex(int index)
{
    m_reference = std::clamp(index, 0, std::max(0, m_spectra.size() - 1));
}

void NmrHandler::setSignals(const QVector<NmrSignal>& list)
{
    m_signals = list;
    for (auto& row : m_manual) {
        for (auto it = row.begin(); it != row.end();) {
            if (it.key() >= m_signals.size())
                it = row.erase(it);
            else
                ++it;
        }
    }
}

int NmrHandler::addSignal(double position, double window, const QString& name)
{
    NmrSignal signal;
    signal.position = position;
    signal.window = (window > 0 && std::isfinite(window)) ? window : m_window;
    signal.name = name.isEmpty() ? QString::number(position, 'f', 3) : name;
    m_signals << signal;
    return m_signals.size() - 1;
}

bool NmrHandler::removeSignal(int index)
{
    if (index < 0 || index >= m_signals.size())
        return false;
    m_signals.removeAt(index);
    // the manual shifts of the following columns move one column to the left
    for (auto& row : m_manual) {
        QMap<int, double> moved;
        for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
            if (it.key() < index)
                moved[it.key()] = it.value();
            else if (it.key() > index)
                moved[it.key() - 1] = it.value();
        }
        row = moved;
    }
    return true;
}

bool NmrHandler::setSignal(int index, const NmrSignal& signal)
{
    if (index < 0 || index >= m_signals.size() || !std::isfinite(signal.position) || !(signal.window > 0))
        return false;
    m_signals[index] = signal;
    return true;
}

int NmrHandler::pickSignals(double window, QString* error)
{
    if (m_spectra.isEmpty()) {
        setError(error, "no spectra");
        return 0;
    }
    setDefaultWindow(window);
    const QVector<QVector<TracePeak>> all = peaks(error);
    const QVector<TracePeak>& reference = all.value(m_reference);
    m_signals.clear();
    m_manual.clear();
    for (const TracePeak& peak : reference)
        addSignal(peak.x, m_window);
    return m_signals.size();
}

// ---------------------------------------------------------------------------------------------
// tracking

/*! \brief Walks from spectrum start in direction step (+1/-1) to the end of the series.
 *
 * The state of each signal starts at its reference position (forward walk) or at the result of the
 * reference spectrum (backward walk, start = reference - 1); see the class description for the rules. */
void NmrHandler::trackDirection(int start, int step, const QVector<QVector<TracePeak>>& peaks)
{
    const int n = m_spectra.size();
    QVector<TrackState> states(m_signals.size());
    for (int s = 0; s < m_signals.size(); ++s) {
        states[s].last = m_signals[s].position;
        states[s].lastIndex = m_reference;
        if (step < 0 && m_reference < n) {
            const NmrCell& ref = m_cells[m_reference][s];
            if (ref.status == NmrCellStatus::Ok || ref.status == NmrCellStatus::Manual)
                states[s].last = ref.shift;
            states[s].height = ref.height;
        }
    }
    for (int k = start; k >= 0 && k < n; k += step) {
        const QString id = m_spectra.idAt(k);
        if (m_excluded.contains(id))
            continue;
        const QVector<TracePeak>& found = peaks.value(k);
        QVector<int> choice(m_signals.size(), -1);
        for (int s = 0; s < m_signals.size(); ++s) {
            NmrCell& cell = m_cells[k][s];
            cell = NmrCell();
            if (!m_signals[s].enabled)
                continue;
            if (m_manual.value(id).contains(s)) {
                cell.shift = m_manual.value(id).value(s);
                cell.status = NmrCellStatus::Manual;
                states[s].update(cell.shift, k);
                continue;
            }
            const double center = states[s].last;
            const double prediction = states[s].predict(k);
            const double window = m_signals[s].window;
            const double minHeight = std::isfinite(states[s].height) ? m_min_height * states[s].height : -std::numeric_limits<double>::infinity();
            const double lastHeight = states[s].height;
            QVector<int> candidates;
            double highest = -std::numeric_limits<double>::infinity();
            for (int p = 0; p < found.size(); ++p) {
                // inside the window around the last known position or around the prediction
                if (std::abs(found[p].x - center) > window && std::abs(found[p].x - prediction) > window)
                    continue;
                // a noise maximum or another, much weaker line is not the signal
                if (found[p].height < minHeight)
                    continue;
                candidates << p;
                highest = std::max(highest, found[p].height);
            }
            int best = -1;
            double bestCost = std::numeric_limits<double>::infinity();
            for (int p : candidates) {
                double cost;
                if (!std::isfinite(lastHeight) || !(lastHeight > 0) || !(found[p].height > 0)) {
                    // first peak of the signal: the nearest of the peaks at least half as high as the highest
                    if (found[p].height < 0.5 * highest)
                        continue;
                    cost = std::abs(found[p].x - center);
                } else {
                    // distance to the prediction in windows + change of the height (log ratio)
                    cost = std::abs(found[p].x - prediction) / window + std::abs(std::log(found[p].height / lastHeight));
                }
                if (cost < bestCost || (cost == bestCost && best >= 0 && found[p].height > found[best].height)) {
                    bestCost = cost;
                    best = p;
                }
            }
            choice[s] = best;
            if (best < 0)
                continue; // Missing: NaN, the state stays
            cell.shift = found[best].x;
            cell.height = found[best].height;
            cell.integral = found[best].integral;
            cell.status = NmrCellStatus::Ok;
        }
        for (int s = 0; s < m_signals.size(); ++s) {
            if (choice[s] < 0)
                continue;
            bool shared = false;
            for (int t = 0; t < m_signals.size(); ++t)
                shared = shared || (t != s && choice[t] == choice[s]);
            if (shared)
                m_cells[k][s].status = NmrCellStatus::Conflict;
            else {
                states[s].update(m_cells[k][s].shift, k);
                states[s].height = m_cells[k][s].height;
            }
        }
    }
}

void NmrHandler::track()
{
    const int n = m_spectra.size();
    m_cells = QVector<QVector<NmrCell>>(n, QVector<NmrCell>(m_signals.size()));
    if (n == 0 || m_signals.isEmpty())
        return;
    setReferenceIndex(m_reference);
    QString error;
    const QVector<QVector<TracePeak>> found = peaks(&error);
#ifdef DEBUG_ON
    if (!error.isEmpty())
        qDebug() << "NmrHandler::track:" << error;
#endif
    trackDirection(m_reference, +1, found);
    trackDirection(m_reference - 1, -1, found);
}

NmrCell NmrHandler::cell(int spectrum, int signal) const
{
    if (spectrum < 0 || spectrum >= m_cells.size() || signal < 0 || signal >= m_cells[spectrum].size())
        return NmrCell();
    return m_cells[spectrum][signal];
}

bool NmrHandler::setManualShift(int spectrum, int signal, double shift)
{
    if (spectrum < 0 || spectrum >= m_spectra.size() || signal < 0 || signal >= m_signals.size() || !std::isfinite(shift))
        return false;
    m_manual[m_spectra.idAt(spectrum)][signal] = shift;
    track();
    return true;
}

bool NmrHandler::clearManualShift(int spectrum, int signal)
{
    if (!hasManualShift(spectrum, signal))
        return false;
    const QString id = m_spectra.idAt(spectrum);
    m_manual[id].remove(signal);
    if (m_manual[id].isEmpty())
        m_manual.remove(id);
    track();
    return true;
}

bool NmrHandler::hasManualShift(int spectrum, int signal) const
{
    if (spectrum < 0 || spectrum >= m_spectra.size())
        return false;
    return m_manual.value(m_spectra.idAt(spectrum)).contains(signal);
}

void NmrHandler::setSpectrumExcluded(int index, bool excluded)
{
    if (index < 0 || index >= m_spectra.size())
        return;
    if (excluded)
        m_excluded.insert(m_spectra.idAt(index));
    else
        m_excluded.remove(m_spectra.idAt(index));
}

bool NmrHandler::isSpectrumExcluded(int index) const
{
    return index >= 0 && index < m_spectra.size() && m_excluded.contains(m_spectra.idAt(index));
}

QVector<int> NmrHandler::includedSpectra() const
{
    QVector<int> rows;
    for (int i = 0; i < m_spectra.size(); ++i)
        if (!m_excluded.contains(m_spectra.idAt(i)))
            rows << i;
    return rows;
}

QVector<int> NmrHandler::enabledSignals() const
{
    QVector<int> columns;
    for (int s = 0; s < m_signals.size(); ++s)
        if (m_signals[s].enabled)
            columns << s;
    return columns;
}

QString NmrHandler::spectrumLabel(int index) const
{
    const QString name = m_spectra.at(index).name();
    return name.isEmpty() ? QString("spectrum %1").arg(index + 1) : QString("spectrum %1 (%2)").arg(index + 1).arg(name);
}

QStringList NmrHandler::problems() const
{
    QStringList list;
    for (int k : includedSpectra()) {
        for (int s : enabledSignals()) {
            const NmrCellStatus status = cell(k, s).status;
            if (status == NmrCellStatus::Missing || status == NmrCellStatus::Conflict)
                list << QString("%1, signal %2: %3").arg(spectrumLabel(k), m_signals[s].name, statusName(status));
        }
    }
    return list;
}

DataTable* NmrHandler::shiftTable(QString* error) const
{
    const QVector<int> rows = includedSpectra(), columns = enabledSignals();
    if (rows.isEmpty() || columns.isEmpty()) {
        setError(error, rows.isEmpty() ? QString("no spectrum is used") : QString("no signal is used"));
        return nullptr;
    }
    const QStringList list = problems();
    if (!list.isEmpty()) {
        const int shown = 10;
        QStringList lines = list.mid(0, shown);
        if (list.size() > shown)
            lines << QString("... and %1 more").arg(list.size() - shown);
        setError(error, QString("%1 cell(s) without a unique shift; set them by hand or leave the spectrum or signal out:\n%2").arg(list.size()).arg(lines.join("\n")));
        return nullptr;
    }
    Eigen::MatrixXd matrix(rows.size(), columns.size());
    QStringList header;
    for (int c = 0; c < columns.size(); ++c) {
        header << QString("δ %1 /ppm").arg(m_signals[columns[c]].name);
        for (int r = 0; r < rows.size(); ++r)
            matrix(r, c) = cell(rows[r], columns[c]).shift;
    }
    DataTable* table = new DataTable(matrix);
    table->setHeader(header);
    return table;
}

DataTable* NmrHandler::importTable(const DataTable* independent, QString* error) const
{
    if (!independent || independent->columnCount() == 0) {
        setError(error, "no concentrations");
        return nullptr;
    }
    const QVector<int> rows = includedSpectra();
    QVector<int> source;
    if (independent->rowCount() == m_spectra.size())
        source = rows;
    else if (independent->rowCount() == rows.size()) {
        for (int r = 0; r < rows.size(); ++r)
            source << r;
    } else {
        setError(error, QString("the concentration table has %1 rows; it needs one per spectrum (%2) or per used spectrum (%3)").arg(independent->rowCount()).arg(m_spectra.size()).arg(rows.size()));
        return nullptr;
    }
    DataTable* shifts = shiftTable(error);
    if (!shifts)
        return nullptr;
    const int ci = independent->columnCount(), cs = shifts->columnCount();
    Eigen::MatrixXd matrix(rows.size(), ci + cs);
    for (int r = 0; r < rows.size(); ++r) {
        for (int c = 0; c < ci; ++c)
            matrix(r, c) = independent->data(source[r], c);
        for (int c = 0; c < cs; ++c)
            matrix(r, ci + c) = shifts->data(r, c);
    }
    QStringList header = independent->header().mid(0, ci);
    while (header.size() < ci)
        header << QString::number(header.size() + 1);
    header << shifts->header();
    delete shifts;
    DataTable* table = new DataTable(matrix);
    table->setHeader(header);
    return table;
}

// ---------------------------------------------------------------------------------------------
// persistence

QString NmrHandler::statusName(NmrCellStatus status)
{
    switch (status) {
    case NmrCellStatus::Ok:
        return "ok";
    case NmrCellStatus::Missing:
        return "missing";
    case NmrCellStatus::Conflict:
        return "conflict";
    case NmrCellStatus::Manual:
        return "manual";
    }
    return QString();
}

bool NmrHandler::isNmrBlock(const QJsonObject& raw)
{
    return raw.value("Profile").toString() == kProfile;
}

/*! \brief Raw block of an NMR project. The keys "Files", "Data" (with "Count" and "1".."n" {x, y, name})
 * and "SupraFit" are those of SpectraHandler, so an older SupraFit shows the spectra in its spectra
 * dialog. */
QJsonObject NmrHandler::toJson() const
{
    QJsonObject json, data;
    QStringList files;
    QJsonArray sources;
    for (int i = 0; i < m_spectra.size(); ++i) {
        const XYTrace& spec = m_spectra.at(i);
        files << spec.source();
        sources.append(QJsonObject{ { "File", spec.source() }, { "Column", m_columns.value(m_spectra.idAt(i), 1) } });
        QJsonObject individual;
        individual["x"] = TraceJson::vectorToString(spec.x());
        individual["y"] = TraceJson::vectorToString(spec.y());
        individual["name"] = spec.name();
        data[QString::number(i + 1)] = individual;
    }
    data["Count"] = m_spectra.size();
    json["Profile"] = kProfile;
    json["SupraFit"] = qint_version;
    json["Files"] = files.join("||");
    json["Sources"] = sources;
    if (qApp && qApp->property("StoreRawData").toBool())
        json["Data"] = data;
    if (!m_operations.isEmpty())
        json["TraceOps"] = m_operations.toJson();
    json["ReferenceOffset"] = m_reference_offset;
    json["ReferenceSpectrum"] = m_reference;
    json["Window"] = m_window;
    json["MinHeight"] = m_min_height;

    QJsonArray signalArray;
    for (const NmrSignal& signal : m_signals) {
        QJsonObject s{ { "Name", signal.name }, { "Position", signal.position }, { "Window", signal.window } };
        if (!signal.enabled)
            s["Enabled"] = false;
        signalArray.append(s);
    }
    json["Signals"] = signalArray;

    QJsonArray manual, excluded;
    for (int i = 0; i < m_spectra.size(); ++i) {
        const QString id = m_spectra.idAt(i);
        const QMap<int, double> row = m_manual.value(id);
        for (auto it = row.constBegin(); it != row.constEnd(); ++it)
            manual.append(QJsonObject{ { "Spectrum", i }, { "Signal", it.key() }, { "Shift", it.value() } });
        if (m_excluded.contains(id))
            excluded.append(i);
    }
    json["Manual"] = manual;
    json["Excluded"] = excluded;
    return json;
}

bool NmrHandler::fromJson(const QJsonObject& json, QString* error)
{
    clear();
    m_signals.clear();
    m_operations.clear();
    m_reference_offset = 0;
    QStringList problems;

    const QJsonObject data = json.value("Data").toObject();
    const int count = data.value("Count").toInt();
    if (json.contains("Data") && count > 0) {
        const QJsonArray sources = json.value("Sources").toArray();
        for (int i = 1; i <= count; ++i) {
            const QJsonObject block = data.value(QString::number(i)).toObject();
            XYTrace trace;
            QString reason;
            if (!XYTrace::fromJson(block, &trace, &reason)) {
                problems << QString("stored spectrum %1: %2").arg(i).arg(reason);
                continue;
            }
            trace.setName(block.value("name").toString(QString::number(i)));
            const QJsonObject source = sources.at(i - 1).toObject();
            trace.setSource(source.value("File").toString());
            const QString id = m_spectra.add(trace);
            m_columns[id] = source.value("Column").toInt(1);
        }
    } else {
        // without raw data: read the files again; a file of several spectra is read once
        QHash<QString, QVector<XYTrace>> read;
        const QJsonArray sources = json.value("Sources").toArray();
        for (const QJsonValue& value : sources) {
            const QString file = value.toObject().value("File").toString();
            const int column = value.toObject().value("Column").toInt(1);
            if (!read.contains(file)) {
                QVector<XYTrace> traces;
                QString reason;
                if (!TraceIO::readTraceFile(file, &traces, &reason))
                    problems << reason;
                read[file] = traces;
            }
            const QVector<XYTrace>& traces = read[file];
            if (column < 1 || column > traces.size()) {
                if (!traces.isEmpty())
                    problems << QString("%1: no spectrum in column %2").arg(file).arg(column);
                continue;
            }
            const QString id = m_spectra.add(traces[column - 1]);
            m_columns[id] = column;
        }
    }
    const int stored = std::max(count, int(json.value("Sources").toArray().size()));

    if (json.contains("TraceOps")) {
        QString reason;
        TraceOpList operations;
        if (TraceOpList::fromJson(json.value("TraceOps").toObject(), &operations, &reason))
            m_operations = operations;
        else
            problems << QString("processing steps not read: %1").arg(reason);
    }
    m_reference_offset = json.value("ReferenceOffset").toDouble(0);
    m_window = json.value("Window").toDouble(0.05);
    m_min_height = json.value("MinHeight").toDouble(0.3);
    for (const QJsonValue& value : json.value("Signals").toArray()) {
        const QJsonObject s = value.toObject();
        NmrSignal signal;
        signal.name = s.value("Name").toString();
        signal.position = s.value("Position").toDouble();
        signal.window = s.value("Window").toDouble(m_window);
        signal.enabled = s.value("Enabled").toBool(true);
        m_signals << signal;
    }
    setReferenceIndex(json.value("ReferenceSpectrum").toInt(0));
    // manual shifts and excluded spectra are stored by position: only valid for the complete series
    const bool complete = m_spectra.size() == stored;
    if (!complete && (!json.value("Manual").toArray().isEmpty() || !json.value("Excluded").toArray().isEmpty()))
        problems << QString("only %1 of %2 spectra read: manual shifts and excluded spectra are not restored").arg(m_spectra.size()).arg(stored);
    for (const QJsonValue& value : complete ? json.value("Manual").toArray() : QJsonArray()) {
        const QJsonObject m = value.toObject();
        const int spectrum = m.value("Spectrum").toInt(-1), signal = m.value("Signal").toInt(-1);
        if (spectrum >= 0 && spectrum < m_spectra.size() && signal >= 0 && signal < m_signals.size())
            m_manual[m_spectra.idAt(spectrum)][signal] = m.value("Shift").toDouble();
        else
            problems << QString("manual shift of spectrum %1, signal %2 is out of range").arg(spectrum + 1).arg(signal + 1);
    }
    for (const QJsonValue& value : complete ? json.value("Excluded").toArray() : QJsonArray())
        setSpectrumExcluded(value.toInt(-1), true);

    track();
    setError(error, problems.join("\n"));
    return !(stored > 0 && m_spectra.isEmpty());
}
