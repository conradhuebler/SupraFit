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
#include <functional>

#include <QtCore/QCollator>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QVariant>

#include "src/core/models/datatable.h"
#include "src/core/trace/jeolreader.h"
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

/*! \brief Window threshold of the tracking (NmrThresholdScope::Window, class description step 3a). Claude Generated.
 *
 * maxima: all local maxima of trace, sorted by index (TraceOps::pickPeaks order); returns the indices into
 * maxima of those with x in [lo, hi] that stand out of their window by one of two measures:
 * - height above the chord h_c = y_p - b(x_p), b the straight line through the medians of the first and the
 *   last tenth (at least 2 points) of the points in [lo - margin, hi + margin] (the margin keeps a peak at the
 *   edge of [lo, hi] out of the baseline): h_c >= max(relative x H, sigmas x noise);
 * - prominence h_p = y_p - max(lowest point on the left, lowest point on the right), each side walked until a
 *   point higher than y_p or the end of the widened window: h_p >= max(relative x H, prominenceSigmas x noise).
 * H = the largest max(h_c, h_p) of the maxima in [lo, hi]. The chord keeps lines that overlap a neighbour (small
 * prominence); the prominence keeps a line on the curved wing of a strong line (the chord lies above a convex
 * wing). Noise passes neither: on a flat baseline its maxima are about 3-4 σ above the chord and 5-7 σ above the
 * lower of their side minima; on a wing the chord lies above and the next noise wiggle limits the prominence.
 * Fewer than 5 points: nothing passes. */
QVector<int> windowCandidates(const XYTrace& trace, double noise, const QVector<TracePeak>& maxima, double lo, double hi,
    double margin, double relative, double sigmas, double prominenceSigmas)
{
    QVector<int> result;
    const Eigen::Index n = trace.size();
    if (n < 5 || maxima.isEmpty() || !(hi >= lo))
        return result;
    const Eigen::VectorXd& x = trace.x();
    const Eigen::VectorXd& y = trace.y();
    const bool ascending = x(n - 1) > x(0);
    // first index whose x lies beyond value in index order (binary search on the monotone axis)
    auto boundIndex = [&](double value, bool upper) -> Eigen::Index {
        Eigen::Index a = 0, b = n;
        while (a < b) {
            const Eigen::Index m = (a + b) / 2;
            const bool before = ascending ? (upper ? x(m) <= value : x(m) < value) : (upper ? x(m) >= value : x(m) > value);
            if (before)
                a = m + 1;
            else
                b = m;
        }
        return a;
    };
    const double blo = lo - margin, bhi = hi + margin;
    const Eigen::Index first = ascending ? boundIndex(blo, false) : boundIndex(bhi, false);
    const Eigen::Index end = ascending ? boundIndex(bhi, true) : boundIndex(blo, true);
    const Eigen::Index count = end - first;
    if (count < 5)
        return result;
    const Eigen::Index edge = std::max<Eigen::Index>(2, count / 10);
    auto segmentMedian = [&](Eigen::Index from) {
        std::vector<double> v(y.data() + from, y.data() + from + edge);
        const size_t half = v.size() / 2;
        std::nth_element(v.begin(), v.begin() + half, v.end());
        double m = v[half];
        if (v.size() % 2 == 0)
            m = 0.5 * (m + *std::max_element(v.begin(), v.begin() + half));
        return m;
    };
    const double y0 = segmentMedian(first), y1 = segmentMedian(end - edge);
    const double x0 = 0.5 * (x(first) + x(first + edge - 1)), x1 = 0.5 * (x(end - edge) + x(end - 1));
    auto chord = [&](double at) { return x1 == x0 ? 0.5 * (y0 + y1) : y0 + (y1 - y0) * (at - x0) / (x1 - x0); };

    // maxima inside the region [lo, hi]: their indices lie within [first, end) (maxima sorted by index)
    auto byIndex = [](const TracePeak& p, Eigen::Index i) { return p.index < i; };
    const int from = int(std::lower_bound(maxima.begin(), maxima.end(), first, byIndex) - maxima.begin());
    const int to = int(std::lower_bound(maxima.begin(), maxima.end(), end, byIndex) - maxima.begin());
    double highest = -std::numeric_limits<double>::infinity();
    QVector<int> inside;
    QVector<double> hc, hp;
    for (int p = from; p < to; ++p) {
        const TracePeak& peak = maxima[p];
        if (peak.x < lo || peak.x > hi || peak.index < first || peak.index >= end)
            continue;
        const double top = y(peak.index);
        double left = top, right = top;
        for (Eigen::Index i = peak.index - 1; i >= first && y(i) <= top; --i)
            left = std::min(left, y(i));
        for (Eigen::Index i = peak.index + 1; i < end && y(i) <= top; ++i)
            right = std::min(right, y(i));
        inside << p;
        hc << peak.height - chord(peak.x);
        hp << peak.height - std::max(left, right);
        highest = std::max(highest, std::max(hc.last(), hp.last()));
    }
    for (int i = 0; i < inside.size(); ++i) {
        const double floor = relative * highest;
        const bool byChord = hc[i] > 0 && hc[i] >= std::max(floor, sigmas * noise);
        const bool byProminence = hp[i] > 0 && hp[i] >= std::max(floor, prominenceSigmas * noise);
        if (byChord || byProminence)
            result << inside[i];
    }
    return result;
}
} // namespace

// ---------------------------------------------------------------------------------------------
// spectra

/*! \brief A JEOL file (recognised by its magic, not by the ending) gives its FID, any other file its text
 * spectra. Claude Generated */
bool NmrHandler::readSource(const QString& filename, QVector<XYTrace>* traces, Fid* fid, bool* isFid, QString* error) const
{
    *isFid = JeolReader::isJeolFile(filename);
    if (!*isFid)
        return TraceIO::readTraceFile(filename, traces, error);
    JeolReader::Data data;
    if (!JeolReader::readFile(filename, &data, error))
        return false;
    return FidProcessing::fromJeol(data, fid, error);
}

bool NmrHandler::addFile(const QString& filename, QString* error)
{
    QVector<XYTrace> traces;
    Fid fid;
    bool isFid = false;
    if (!readSource(filename, &traces, &fid, &isFid, error))
        return false;
    if (isFid) {
        XYTrace spectrum;
        FidPhase phase;
        if (!FidProcessing::process(fid, m_fid_settings, &spectrum, &phase, error))
            return false;
        const QString id = m_spectra.add(spectrum);
        m_columns[id] = 1;
        m_fids[id] = fid;
        m_fid_phases[id] = phase;
        return true;
    }
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
    m_fids.remove(id);
    m_fid_phases.remove(id);
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
    m_fids.clear();
    m_fid_phases.clear();
    m_manual.clear();
    m_excluded.clear();
    m_cells.clear();
    m_fit_regions.clear();
    m_reference = 0;
}

// ---------------------------------------------------------------------------------------------
// FID processing (Claude Generated, JEOL import)

bool NmrHandler::isFid(int index) const
{
    return index >= 0 && index < m_spectra.size() && m_fids.contains(m_spectra.idAt(index));
}

bool NmrHandler::setFidSettings(const FidSettings& settings, QString* error)
{
    QString reason;
    if (!settings.isValid(&reason)) {
        setError(error, reason);
        return false;
    }
    // transform everything first: on a failure nothing changes
    QHash<QString, XYTrace> spectra;
    QHash<QString, FidPhase> phases;
    for (auto it = m_fids.constBegin(); it != m_fids.constEnd(); ++it) {
        XYTrace spectrum;
        FidPhase phase;
        if (!FidProcessing::process(it.value(), settings, &spectrum, &phase, error))
            return false;
        spectra[it.key()] = spectrum;
        phases[it.key()] = phase;
    }
    m_fid_settings = settings;
    for (auto it = spectra.constBegin(); it != spectra.constEnd(); ++it) {
        const int index = m_spectra.indexOf(it.key());
        if (index >= 0)
            m_spectra.replace(index, it.value());
    }
    m_fid_phases = phases;
    return true;
}

FidPhase NmrHandler::fidPhase(int index) const
{
    return isFid(index) ? m_fid_phases.value(m_spectra.idAt(index)) : FidPhase();
}

QString NmrHandler::fidDescription(int index) const
{
    return isFid(index) ? m_fids.value(m_spectra.idAt(index)).description : QString();
}

bool NmrHandler::autoPhaseFromReference(QString* error)
{
    int index = isFid(m_reference) ? m_reference : -1;
    for (int i = 0; index < 0 && i < m_spectra.size(); ++i)
        if (isFid(i))
            index = i;
    if (index < 0) {
        setError(error, QStringLiteral("no spectrum was transformed from a FID"));
        return false;
    }
    const Fid& fid = m_fids[m_spectra.idAt(index)];
    Eigen::VectorXcd spectrum;
    Eigen::VectorXd ppm;
    if (!FidProcessing::complexSpectrum(fid, m_fid_settings, &spectrum, &ppm, error))
        return false;
    const double pivot = std::isfinite(m_fid_settings.pivot) ? m_fid_settings.pivot : fid.offset;
    const FidPhase phase = FidProcessing::autoPhase(spectrum, ppm, fid.frequency, fid.sweep, pivot, m_fid_settings.autoFirstOrder, m_fid_settings.ph1);
    FidSettings settings = m_fid_settings;
    settings.ph0 = phase.ph0;
    settings.ph1 = phase.ph1;
    settings.autoPhase = false;
    return setFidSettings(settings, error);
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
 * the default picking on the processed spectrum if there is no such step (relative threshold 0.05 of the
 * spectrum, or with the window threshold the maxima that pass it around themselves). */
QVector<QVector<TracePeak>> NmrHandler::peaks(QString* error) const
{
    if (!usesWindowThreshold())
        return peaks(error, nullptr);
    QVector<XYTrace> processed;
    const QVector<QVector<TracePeak>> maxima = peaks(error, &processed, true);
    QVector<double> noise(processed.size());
    for (int k = 0; k < processed.size(); ++k)
        noise[k] = TraceOps::noiseLevel(processed[k]);
    return windowDetectedPeaks(processed, maxima, noise);
}

bool NmrHandler::hasPickStep() const
{
    bool hasPick = false;
    for (int i = 0; i < m_operations.size(); ++i)
        hasPick = hasPick || (m_operations.at(i)->enabled && m_operations.at(i)->type() == "PickPeaks");
    return hasPick;
}

bool NmrHandler::usesWindowThreshold() const
{
    return m_threshold_scope == NmrThresholdScope::Window && !hasPickStep();
}

/*! \brief Maxima that pass the window rule (class description, step 3a) in a window ± defaultWindow() centred on
 * themselves: the peaks of a spectrum without a signal to follow (pickSignals, neighbour lines of the fit).
 * Claude Generated. */
QVector<QVector<TracePeak>> NmrHandler::windowDetectedPeaks(const QVector<XYTrace>& processed, const QVector<QVector<TracePeak>>& maxima, const QVector<double>& noise,
    const QVector<QVector<QPair<double, double>>>* near) const
{
    QVector<QVector<TracePeak>> result(maxima.size());
    for (int k = 0; k < maxima.size() && k < processed.size(); ++k) {
        const XYTrace& trace = processed[k];
        const QVector<TracePeak>& all = maxima[k];
        if (trace.size() < 5)
            continue;
        const double sigma = noise.value(k, 0);
        for (int p = 0; p < all.size(); ++p) {
            if (near) {
                bool inside = false;
                for (const auto& range : near->value(k))
                    inside = inside || (all[p].x >= range.first && all[p].x <= range.second);
                if (!inside)
                    continue;
            }
            const QVector<int> passing = windowCandidates(trace, sigma, all, all[p].x - m_window, all[p].x + m_window, m_window, WindowRelativeThreshold, WindowNoiseSigmas, WindowProminenceSigmas);
            if (passing.contains(p))
                result[k] << all[p];
        }
    }
    return result;
}

/*! \brief peaks() and, if processed is given, the processed spectra they were picked on (one pass); with
 * allMaxima the default picking returns every local maximum (threshold 0, input of the window threshold). */
QVector<QVector<TracePeak>> NmrHandler::peaks(QString* error, QVector<XYTrace>* processedSpectra, bool allMaxima) const
{
    const bool hasPick = hasPickStep();

    const TraceSet referenced = referencedSpectra();
    QVector<QVector<TracePeak>> result(referenced.size());
    QString first;
    PickPeaksOp fallback;
    fallback.threshold = allMaxima ? 0.0 : 0.05;
    if (processedSpectra)
        processedSpectra->resize(referenced.size());
    for (int i = 0; i < referenced.size(); ++i) {
        QString reason;
        QVector<TracePeak> found;
        const XYTrace processed = m_operations.apply(referenced.at(i), &reason, &found);
        if (reason.isEmpty() && !hasPick)
            found = fallback.pick(processed, &reason);
        if (!reason.isEmpty() && first.isEmpty())
            first = QString("%1: %2").arg(referenced.at(i).name(), reason);
        result[i] = reason.isEmpty() ? found : QVector<TracePeak>();
        if (processedSpectra)
            (*processedSpectra)[i] = reason.isEmpty() ? processed : XYTrace();
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
    QVector<QVector<TracePeak>> all;
    if (usesWindowThreshold()) {
        // the window rule only for the reference spectrum (Claude Generated)
        QVector<XYTrace> processed;
        const QVector<QVector<TracePeak>> maxima = peaks(error, &processed, true);
        QVector<double> noise(processed.size(), 0.0);
        QVector<QVector<QPair<double, double>>> near(processed.size());
        if (m_reference < processed.size()) {
            noise[m_reference] = TraceOps::noiseLevel(processed[m_reference]);
            near[m_reference] << qMakePair(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
        }
        all = windowDetectedPeaks(processed, maxima, noise, &near);
    } else
        all = peaks(error);
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
void NmrHandler::trackDirection(int start, int step, const QVector<QVector<TracePeak>>& peaks, const QVector<XYTrace>* processed,
    const QVector<double>* noise)
{
    // with processed spectra and their noise: peaks holds every maximum, the window threshold selects (step 3a)
    const bool windowThreshold = processed && noise;
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
            QVector<char> passes;
            if (windowThreshold) {
                passes = QVector<char>(found.size(), 0);
                const double lo = std::min(center, prediction) - window, hi = std::max(center, prediction) + window;
                for (int p : windowCandidates(processed->value(k), noise->value(k, 0), found, lo, hi, window, WindowRelativeThreshold, WindowNoiseSigmas, WindowProminenceSigmas))
                    passes[p] = 1;
            }
            for (int p = 0; p < found.size(); ++p) {
                if (windowThreshold && !passes[p])
                    continue;
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
            cell.peakShift = found[best].x;
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
    m_fit_regions = QVector<QVector<NmrFitRegion>>(n);
    m_fit_ms = 0;
    if (n == 0 || m_signals.isEmpty())
        return;
    setReferenceIndex(m_reference);
    QString error;
    QVector<XYTrace> processed;
    const bool windowThreshold = usesWindowThreshold();
    const QVector<QVector<TracePeak>> found = peaks(&error, &processed, windowThreshold);
#ifdef DEBUG_ON
    if (!error.isEmpty())
        qDebug() << "NmrHandler::track:" << error;
#endif
    if (!windowThreshold) {
        trackDirection(m_reference, +1, found);
        trackDirection(m_reference - 1, -1, found);
        if (m_fit.method == NmrShiftMethod::LineshapeFit)
            fitLineshapes(processed, found);
        return;
    }
    // window threshold (Claude Generated): every maximum is a candidate, the rule of step 3a selects
    QVector<double> noise(processed.size());
    for (int k = 0; k < processed.size(); ++k)
        noise[k] = TraceOps::noiseLevel(processed[k]);
    trackDirection(m_reference, +1, found, &processed, &noise);
    trackDirection(m_reference - 1, -1, found, &processed, &noise);
    if (m_fit.method == NmrShiftMethod::LineshapeFit) {
        // the fit looks for neighbour lines only near the tracked signals: the rule runs there, not over the whole
        // spectrum (whole JEOL spectra hold thousands of noise maxima)
        QVector<QVector<QPair<double, double>>> near(m_cells.size());
        for (int k = 0; k < m_cells.size(); ++k)
            for (int s = 0; s < m_signals.size(); ++s) {
                const NmrCell& cell = m_cells[k][s];
                if (std::isfinite(cell.shift)) {
                    const double reach = m_signals[s].window + m_fit.range;
                    near[k] << qMakePair(cell.shift - reach, cell.shift + reach);
                }
            }
        fitLineshapes(processed, windowDetectedPeaks(processed, found, noise, &near));
    }
}

// ---------------------------------------------------------------------------------------------
// lineshape fit (WP4)

QJsonObject NmrFitSettings::toJson() const
{
    return QJsonObject{
        { "Method", method == NmrShiftMethod::LineshapeFit ? "LineshapeFit" : "PeakPicking" },
        { "WidthMode", widthMode == NmrWidthMode::Shared ? "Shared" : "Free" },
        { "Baseline", LineshapeFit::baselineName(baseline) },
        { "Range", range },
        { "MaxIterations", maxIterations },
        { "PeakFallback", peakFallback }
    };
}

NmrFitSettings NmrFitSettings::fromJson(const QJsonObject& json)
{
    NmrFitSettings s;
    s.method = json.value("Method").toString() == "LineshapeFit" ? NmrShiftMethod::LineshapeFit : NmrShiftMethod::PeakPicking;
    s.widthMode = json.value("WidthMode").toString() == "Shared" ? NmrWidthMode::Shared : NmrWidthMode::Free;
    s.baseline = LineshapeFit::baselineFromName(json.value("Baseline").toString(), s.baseline);
    s.range = json.value("Range").toDouble(s.range);
    s.maxIterations = json.value("MaxIterations").toInt(s.maxIterations);
    s.peakFallback = json.value("PeakFallback").toBool(s.peakFallback);
    return s;
}

void NmrHandler::setFitSettings(const NmrFitSettings& settings)
{
    m_fit = settings;
    if (!(m_fit.range > 0) || !std::isfinite(m_fit.range))
        m_fit.range = NmrFitSettings().range;
    m_fit.maxIterations = std::clamp(m_fit.maxIterations, 1, 100000);
}

QVector<NmrFitRegion> NmrHandler::fitRegions(int spectrum) const
{
    return m_fit_regions.value(spectrum);
}

/*! \brief Lineshape fit of the tracked signals (class description, steps 5-7). Claude Generated. */
void NmrHandler::fitLineshapes(const QVector<XYTrace>& processed, const QVector<QVector<TracePeak>>& peaks)
{
    QElapsedTimer timer;
    timer.start();
    const int n = m_spectra.size(), ns = m_signals.size();
    const double R = m_fit.range;
    const LineshapeOptions options{ m_fit.baseline, m_fit.maxIterations, 1e-10 };

    // position of the line of signal s in spectrum k: the picked δ (Ok), the manual δ, the shared peak
    // (Conflict: the fit may separate the lines); NaN: no line
    auto startPosition = [&](int k, int s) {
        if (m_excluded.contains(m_spectra.idAt(k)) || !m_signals[s].enabled)
            return kNaN;
        const NmrCell& c = m_cells[k][s];
        if (c.status == NmrCellStatus::Ok)
            return c.peakShift;
        if (c.status == NmrCellStatus::Manual || c.status == NmrCellStatus::Conflict)
            return c.shift;
        return kNaN;
    };

    // starts of signals that claim the same peak (Conflict): spread around the peak by ± 0.25 of its width
    // at half height, in the order of their reference positions; the height is split between them
    QVector<QVector<double>> conflictStart(n, QVector<double>(ns, kNaN)), conflictHeight(n, QVector<double>(ns, kNaN));
    for (int k = 0; k < n; ++k) {
        QMap<double, QVector<int>> byPeak;
        for (int s = 0; s < ns; ++s)
            if (m_cells[k][s].status == NmrCellStatus::Conflict && std::isfinite(startPosition(k, s)))
                byPeak[m_cells[k][s].shift] << s;
        const XYTrace& trace = processed.value(k);
        for (auto it = byPeak.begin(); it != byPeak.end(); ++it) {
            QVector<int> list = it.value();
            std::sort(list.begin(), list.end(), [this](int a, int b) { return m_signals[a].position < m_signals[b].position; });
            double w = trace.isEmpty() ? kNaN : LineshapeFit::estimateFwhm(trace.x(), trace.y(), trace.nearestIndex(it.key()), 0);
            if (!std::isfinite(w) && trace.size() > 1)
                w = 3 * std::abs(trace.x()(1) - trace.x()(0));
            for (int i = 0; i < list.size(); ++i) {
                conflictStart[k][list[i]] = it.key() + (i - 0.5 * (list.size() - 1)) * 0.5 * (std::isfinite(w) ? w : 0);
                conflictHeight[k][list[i]] = m_cells[k][list[i]].height / list.size();
            }
        }
    }

    // groups: signals whose ranges overlap in any spectrum (union-find)
    QVector<int> parent(ns);
    for (int s = 0; s < ns; ++s)
        parent[s] = s;
    std::function<int(int)> find = [&](int s) { return parent[s] == s ? s : (parent[s] = find(parent[s])); };
    for (int k = 0; k < n; ++k)
        for (int s = 0; s < ns; ++s)
            for (int t = s + 1; t < ns; ++t) {
                const double a = startPosition(k, s), b = startPosition(k, t);
                if (std::isfinite(a) && std::isfinite(b) && std::abs(a - b) < 2 * R)
                    parent[find(t)] = find(s);
            }
    QMap<int, QVector<int>> groups;
    for (int s = 0; s < ns; ++s)
        groups[find(s)] << s;

    // block of group in spectrum k; prevFwhm/prevEta: starts from the last fit of each signal (NaN: from the peak)
    auto buildBlock = [&](int k, const QVector<int>& group, const QVector<double>& prevFwhm, const QVector<double>& prevEta,
                          const QVector<double>& prevPosition, LineshapeBlock* block, NmrFitRegion* region) {
        const XYTrace& trace = processed.value(k);
        QVector<int> lineSignals;
        double lo = std::numeric_limits<double>::infinity(), hi = -lo, minHeight = lo;
        for (int s : group) {
            const double p = startPosition(k, s);
            if (!std::isfinite(p))
                continue;
            lineSignals << s;
            lo = std::min(lo, p);
            hi = std::max(hi, p);
            if (m_cells[k][s].status == NmrCellStatus::Conflict) {
                // the own line of a signal in conflict may lie anywhere in its window (around its last fitted
                // position, else around the shared maximum): the range covers it
                const double previous = prevPosition.value(s, kNaN);
                const double centre = std::isfinite(previous) ? previous : p;
                lo = std::min(lo, centre - m_signals[s].window + R);
                hi = std::max(hi, centre + m_signals[s].window - R);
            }
            if (m_cells[k][s].status == NmrCellStatus::Ok)
                minHeight = std::min(minHeight, m_cells[k][s].height);
            else if (m_cells[k][s].status == NmrCellStatus::Conflict)
                minHeight = std::min(minHeight, conflictHeight[k][s]);
        }
        if (lineSignals.isEmpty() || trace.isEmpty())
            return false;
        region->from = lo - R;
        region->to = hi + R;
        const XYTrace part = trace.range(region->from, region->to);
        if (part.size() < 3) {
            region->message = "no data in the fit range";
            region->lineSignal = lineSignals;
            return false;
        }
        const Eigen::Index last = part.size() - 1;
        const double b0 = options.baseline == LineshapeBaseline::None ? 0.0 : std::min(part.y()(0), part.y()(last));
        const double dx = std::abs(part.x()(last) - part.x()(0)) / double(last);
        const double xmin = part.xMin(), xmax = part.xMax();
        if (!std::isfinite(minHeight))
            minHeight = part.y().maxCoeff();

        auto line = [&](double position, double height, double window, double fwhm0, double eta0) {
            LineshapeLineStart l;
            l.position.value = position;
            l.position.lower = std::max(xmin, position - window);
            l.position.upper = std::min(xmax, position + window);
            double w = std::isfinite(fwhm0) ? fwhm0 : LineshapeFit::estimateFwhm(part.x(), part.y(), part.nearestIndex(position), b0);
            if (!std::isfinite(w) || !(w > 0))
                w = 3 * dx;
            l.fwhm.lower = 0.25 * dx;
            l.fwhm.upper = xmax - xmin;
            l.fwhm.value = std::clamp(w, l.fwhm.lower, l.fwhm.upper);
            l.eta.value = std::isfinite(eta0) ? std::clamp(eta0, 0.0, 1.0) : 0.5;
            l.area.lower = 0;
            l.area.value = LineshapeFit::areaFromHeight(std::max(height - b0, 1e-12 * std::abs(height) + 1e-300), l.fwhm.value, l.eta.value);
            return l;
        };
        block->x = part.x();
        block->y = part.y();
        block->lines.clear();
        region->lineSignal.clear();

        // peaks of the range no signal claims (candidates for a neighbour line)
        QVector<TracePeak> unassigned;
        for (const TracePeak& peak : peaks.value(k)) {
            if (peak.x < xmin || peak.x > xmax || peak.height < m_min_height * minHeight)
                continue;
            bool assigned = false;
            for (int s : lineSignals) {
                const NmrCell& c = m_cells[k][s];
                // the peak of a signal, also the one maximum two signals share (Conflict)
                assigned = assigned || (c.status == NmrCellStatus::Ok && c.peakShift == peak.x) || (c.status == NmrCellStatus::Conflict && c.shift == peak.x);
            }
            if (!assigned)
                unassigned << peak;
        }
        QMap<int, TracePeak> claimed; // signal in conflict -> peak that is its start
        QVector<bool> taken(unassigned.size(), false);
        // a signal in conflict whose own line is resolved here: an unclaimed peak within its window (around its
        // last fitted position, else its reference position) is its start; the nearest signal gets it
        for (int p = 0; p < unassigned.size(); ++p) {
            int best = -1;
            double bestDistance = std::numeric_limits<double>::infinity();
            if (taken[p])
                continue;
            for (int s : lineSignals) {
                if (m_cells[k][s].status != NmrCellStatus::Conflict || claimed.contains(s))
                    continue;
                const double previous = prevPosition.value(s, kNaN);
                const double centre = std::isfinite(previous) ? previous : m_signals[s].position;
                const double distance = std::abs(unassigned[p].x - centre);
                if (distance <= m_signals[s].window && distance < bestDistance) {
                    best = s;
                    bestDistance = distance;
                }
            }
            if (best >= 0) {
                claimed[best] = unassigned[p];
                taken[p] = true;
            }
        }
        for (int s : lineSignals) {
            const NmrCell& c = m_cells[k][s];
            const bool manual = c.status == NmrCellStatus::Manual;
            double position = manual ? c.shift : c.peakShift, height = manual ? part.interpolate(position) : c.height;
            if (c.status == NmrCellStatus::Conflict) {
                if (claimed.contains(s)) {
                    position = claimed[s].x;
                    height = claimed[s].height;
                } else {
                    // the shared maximum: alone if the partners found their own peaks, else spread around it;
                    // the position of the last fit of the signal if it is near
                    int partners = 0;
                    for (int t : lineSignals)
                        partners += t != s && m_cells[k][t].status == NmrCellStatus::Conflict && m_cells[k][t].shift == c.shift && !claimed.contains(t);
                    const double previous = prevPosition.value(s, kNaN);
                    if (partners == 0) {
                        position = c.shift;
                        height = c.height;
                    } else {
                        position = std::isfinite(previous) && std::abs(previous - c.shift) <= m_signals[s].window ? previous : conflictStart[k][s];
                        height = conflictHeight[k][s];
                    }
                }
                position = std::clamp(position, xmin, xmax);
            }
            LineshapeLineStart l = line(position, std::isfinite(height) ? height : b0, m_signals[s].window, prevFwhm.value(s, kNaN), prevEta.value(s, kNaN));
            if (manual)
                l.position.fixed = true;
            if (m_fit.widthMode == NmrWidthMode::Shared)
                l.shared = s;
            block->lines << l;
            region->lineSignal << s;
        }
        // neighbour lines: further picked peaks in the range, not much weaker than the signals
        for (int p = 0; p < unassigned.size(); ++p) {
            if (taken[p])
                continue;
            const TracePeak& peak = unassigned[p];
            const double w = LineshapeFit::estimateFwhm(part.x(), part.y(), part.nearestIndex(peak.x), b0);
            const double window = std::isfinite(w) ? w : R / 2;
            block->lines << line(peak.x, peak.height, window, kNaN, kNaN);
            region->lineSignal << -1;
        }
        return true;
    };

    // does the fit of signal line l of block b count? (step 7)
    auto lineFailure = [&](const LineshapeResult& r, int b, int l, bool manual) -> QString {
        if (!r.ok())
            return r.message.isEmpty() ? LineshapeFit::statusName(r.status) : r.message;
        const LineshapeBlockResult& br = r.blocks[b];
        if (!manual && br.positionAtBound[l])
            return "the line centre is at the limit of the search window";
        if (br.fwhmAtBound[l])
            return "the line width is at its limit";
        if (br.areaAtBound[l])
            return "the line area went to zero";
        double largest = 0;
        for (int i = 0; i < br.lines.size(); ++i)
            largest = std::max(largest, br.lines[i].area);
        if (br.lines[l].area < 0.01 * largest)
            return "the line vanished (area below 1 % of the largest line of the range)";
        if (!manual && !std::isfinite(br.errors[l].position))
            return "no standard error of the position";
        return QString();
    };

    auto store = [&](int k, const LineshapeResult& r, int b, const NmrFitRegion& base) {
        NmrFitRegion region = base;
        region.status = r.status;
        region.message = r.message;
        if (b < r.blocks.size())
            region.block = r.blocks[b];
        for (int l = 0; l < region.lineSignal.size() && b < r.blocks.size(); ++l) {
            const int s = region.lineSignal[l];
            if (s < 0)
                continue;
            NmrCell& c = m_cells[k][s];
            const PseudoVoigtLine& fitted = r.blocks[b].lines[l];
            const PseudoVoigtLine& error = r.blocks[b].errors[l];
            c.fitShift = fitted.position;
            c.shiftError = error.position;
            c.fwhm = fitted.fwhm;
            c.area = fitted.area;
            c.eta = fitted.eta;
            c.fitMessage = lineFailure(r, b, l, c.status == NmrCellStatus::Manual);
            c.fitState = c.fitMessage.isEmpty() ? NmrFitState::Fitted : NmrFitState::Failed;
        }
        m_fit_regions[k] << region;
    };

    auto okLines = [&](int k, const LineshapeResult& r, const NmrFitRegion& region) {
        int ok = 0;
        for (int l = 0; l < region.lineSignal.size(); ++l) {
            const int s = region.lineSignal[l];
            ok += s >= 0 && lineFailure(r, 0, l, m_cells[k][s].status == NmrCellStatus::Manual).isEmpty();
        }
        return ok;
    };

    if (m_fit.widthMode == NmrWidthMode::Free) {
        // titration order from the reference outwards; starts from the previous spectrum
        QVector<int> order;
        for (int k = m_reference; k < n; ++k)
            order << k;
        for (int k = m_reference - 1; k >= 0; --k)
            order << k;
        QVector<double> prevFwhm(ns, kNaN), prevEta(ns, kNaN), prevPosition(ns, kNaN), refFwhm(ns, kNaN), refEta(ns, kNaN), refPosition(ns, kNaN);
        const QVector<double> none(ns, kNaN);
        for (int k : order) {
            if (k == m_reference - 1) { // backward walk: start again at the reference spectrum
                prevFwhm = refFwhm;
                prevEta = refEta;
                prevPosition = refPosition;
            }
            if (m_excluded.contains(m_spectra.idAt(k)))
                continue;
            for (const QVector<int>& group : groups) {
                LineshapeBlock block;
                NmrFitRegion region;
                if (!buildBlock(k, group, prevFwhm, prevEta, prevPosition, &block, &region))
                    continue;
                LineshapeResult r = LineshapeFit::fitSeries({ block }, options);
                bool usedPrevious = false;
                for (int s : region.lineSignal)
                    usedPrevious = usedPrevious || (s >= 0 && std::isfinite(prevFwhm[s]));
                if (usedPrevious && okLines(k, r, region) < int(std::count_if(region.lineSignal.begin(), region.lineSignal.end(), [](int s) { return s >= 0; }))) {
                    // fall back to the starts from the peaks of this spectrum
                    LineshapeBlock fresh;
                    NmrFitRegion freshRegion;
                    if (buildBlock(k, group, none, none, none, &fresh, &freshRegion)) {
                        const LineshapeResult again = LineshapeFit::fitSeries({ fresh }, options);
                        if (okLines(k, again, freshRegion) > okLines(k, r, region)) {
                            r = again;
                            region = freshRegion;
                        }
                    }
                }
                store(k, r, 0, region);
                for (int l = 0; l < region.lineSignal.size(); ++l) {
                    const int s = region.lineSignal[l];
                    if (s >= 0 && m_cells[k][s].fitState == NmrFitState::Fitted) {
                        prevFwhm[s] = m_cells[k][s].fwhm;
                        prevEta[s] = m_cells[k][s].eta;
                        prevPosition[s] = m_cells[k][s].fitShift;
                        if (k == m_reference) {
                            refFwhm[s] = prevFwhm[s];
                            refEta[s] = prevEta[s];
                            refPosition[s] = prevPosition[s];
                        }
                    }
                }
            }
        }
    } else {
        // one joint fit per group over the used spectra
        const QVector<double> none(ns, kNaN);
        for (const QVector<int>& group : groups) {
            QVector<LineshapeBlock> blocks;
            QVector<NmrFitRegion> regions;
            QVector<int> spectra;
            for (int k = 0; k < n; ++k) {
                if (m_excluded.contains(m_spectra.idAt(k)))
                    continue;
                LineshapeBlock block;
                NmrFitRegion region;
                if (!buildBlock(k, group, none, none, none, &block, &region))
                    continue;
                blocks << block;
                regions << region;
                spectra << k;
            }
            if (blocks.isEmpty())
                continue;
            const LineshapeResult r = LineshapeFit::fitSeries(blocks, options);
            for (int b = 0; b < blocks.size(); ++b)
                store(spectra[b], r, b, regions[b]);
        }
    }

    // step 7: the shift of the table
    for (int k : includedSpectra())
        for (int s : enabledSignals()) {
            NmrCell& c = m_cells[k][s];
            if (c.status == NmrCellStatus::Conflict && c.fitState == NmrFitState::Fitted) {
                // the signals that shared the maximum have to end on separate lines
                for (int t : enabledSignals()) {
                    const NmrCell& o = m_cells[k][t];
                    if (t == s || o.status != NmrCellStatus::Conflict || o.shift != c.shift || !std::isfinite(o.fitShift))
                        continue;
                    if (std::abs(o.fitShift - c.fitShift) < 0.25 * 0.5 * (o.fwhm + c.fwhm)) {
                        c.fitState = NmrFitState::Failed;
                        c.fitMessage = "the signals sharing one maximum were not separated";
                    }
                }
            }
            if (c.status == NmrCellStatus::Conflict && c.fitState == NmrFitState::Fitted) {
                // the fit separated the lines that share one maximum
                c.status = NmrCellStatus::Ok;
                c.shift = c.fitShift;
                continue;
            }
            if (c.status != NmrCellStatus::Ok)
                continue;
            if (c.fitState == NmrFitState::NotFitted) {
                c.fitState = NmrFitState::Failed;
                c.fitMessage = "no fit (no data in the fit range)";
            }
            if (c.fitState == NmrFitState::Fitted)
                c.shift = c.fitShift;
            else if (!m_fit.peakFallback) {
                c.status = NmrCellStatus::FitFailed;
                c.shift = kNaN;
            }
        }
    m_fit_ms = timer.nsecsElapsed() * 1e-6;
#ifdef DEBUG_ON
    qDebug() << "NmrHandler::fitLineshapes:" << groups.size() << "groups," << m_fit_ms << "ms";
#endif
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
            if (status == NmrCellStatus::Missing || status == NmrCellStatus::Conflict || status == NmrCellStatus::FitFailed)
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

DataTable* NmrHandler::shiftErrorTable(QString* error) const
{
    const QVector<int> rows = includedSpectra(), columns = enabledSignals();
    if (m_fit.method != NmrShiftMethod::LineshapeFit) {
        setError(error, "the shifts are not fitted");
        return nullptr;
    }
    if (rows.isEmpty() || columns.isEmpty()) {
        setError(error, rows.isEmpty() ? QString("no spectrum is used") : QString("no signal is used"));
        return nullptr;
    }
    Eigen::MatrixXd matrix(rows.size(), columns.size());
    QStringList header;
    for (int c = 0; c < columns.size(); ++c) {
        header << QString("σ(δ) %1 /ppm").arg(m_signals[columns[c]].name);
        for (int r = 0; r < rows.size(); ++r) {
            const NmrCell cell = this->cell(rows[r], columns[c]);
            matrix(r, c) = cell.fitState == NmrFitState::Fitted && cell.status == NmrCellStatus::Ok ? cell.shiftError : kNaN;
        }
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
    case NmrCellStatus::FitFailed:
        return "fit failed";
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
    const bool raw = qApp && qApp->property("StoreRawData").toBool();
    if (raw)
        json["Data"] = data;
    if (hasFids()) {
        json["FidProcessing"] = m_fid_settings.toJson();
        if (raw) {
            QJsonObject fids;
            for (int i = 0; i < m_spectra.size(); ++i)
                if (isFid(i))
                    fids[QString::number(i + 1)] = m_fids.value(m_spectra.idAt(i)).toJson();
            json["Fid"] = fids;
        }
    }
    if (!m_operations.isEmpty())
        json["TraceOps"] = m_operations.toJson();
    json["ReferenceOffset"] = m_reference_offset;
    json["ReferenceSpectrum"] = m_reference;
    json["Window"] = m_window;
    json["MinHeight"] = m_min_height;
    if (m_threshold_scope == NmrThresholdScope::Window)
        json["ThresholdScope"] = "Window"; // only then: projects of the default stay as before

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
    json["LineshapeFit"] = m_fit.toJson();
    return json;
}

bool NmrHandler::fromJson(const QJsonObject& json, QString* error)
{
    clear();
    m_signals.clear();
    m_operations.clear();
    m_reference_offset = 0;
    QStringList problems;

    // FID processing first: the FIDs are transformed while they are read (projects without it: defaults)
    m_fid_settings = FidSettings();
    if (json.contains("FidProcessing")) {
        QString reason;
        FidSettings settings;
        if (FidSettings::fromJson(json.value("FidProcessing").toObject(), &settings, &reason))
            m_fid_settings = settings;
        else
            problems << reason;
    }
    // adds the FID's spectrum; false with a problem if the transform fails
    auto addFid = [this, &problems](const Fid& fid, const QString& file) {
        XYTrace spectrum;
        FidPhase phase;
        QString reason;
        if (!FidProcessing::process(fid, m_fid_settings, &spectrum, &phase, &reason)) {
            problems << reason;
            return false;
        }
        spectrum.setSource(file);
        const QString id = m_spectra.add(spectrum);
        m_columns[id] = 1;
        m_fids[id] = fid;
        m_fids[id].source = file;
        m_fid_phases[id] = phase;
        return true;
    };

    const QJsonObject data = json.value("Data").toObject();
    const QJsonObject storedFids = json.value("Fid").toObject();
    const int count = data.value("Count").toInt();
    if (json.contains("Data") && count > 0) {
        const QJsonArray sources = json.value("Sources").toArray();
        for (int i = 1; i <= count; ++i) {
            const QJsonObject block = data.value(QString::number(i)).toObject();
            if (storedFids.contains(QString::number(i))) {
                Fid fid;
                QString reason;
                if (Fid::fromJson(storedFids.value(QString::number(i)).toObject(), &fid, &reason))
                    addFid(fid, sources.at(i - 1).toObject().value("File").toString());
                else
                    problems << QString("stored FID %1: %2").arg(i).arg(reason);
                continue;
            }
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
                Fid fid;
                bool isFid = false;
                QString reason;
                if (!readSource(file, &traces, &fid, &isFid, &reason))
                    problems << reason;
                else if (isFid) {
                    addFid(fid, file);
                    continue; // one spectrum per FID file; a second entry of the same file reads it again
                }
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
    m_threshold_scope = json.value("ThresholdScope").toString() == "Window" ? NmrThresholdScope::Window : NmrThresholdScope::Spectrum;
    // without the key (projects before WP4): peak picking, as before
    setFitSettings(NmrFitSettings::fromJson(json.value("LineshapeFit").toObject()));
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
