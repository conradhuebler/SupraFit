/*
 * SpectraHandler - import and preparation of spectra series (UV-Vis titrations)
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

// #include "external/CxxClust/src/cxxcluster.h"
#include "cxxcluster.h"

#include <Eigen/Dense>

#include <QtCore/QCollator>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDirIterator>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include <src/core/models/dataclass.h>

#include <src/core/toolset.h>
#include <src/core/trace/tracejson.h>

#include "spectrahandler.h"

SpectraHandler::SpectraHandler(QObject* parent)
    : QObject(parent)
{
}

SpectraHandler::~SpectraHandler()
{
    delete m_table;
}

/*! \brief Reads a table of spectra: first row = x axis (wavelength), every further row = one spectrum.
 *
 * The table read is kept in m_table (owned by the handler, replaced on the next call). Empty lines
 * (also after the final newline) are skipped by ToolSet::LoadTableFile, so every row is a spectrum.
 */
void SpectraHandler::setSpectrafromFile(const QString& file)
{
    int index = 1;
    DataTable* table = ToolSet::LoadTableFile(file);
    if (table->rowCount() < 2) {
        qWarning() << "SpectraHandler: no spectra found in" << file;
        delete table;
        return;
    }
    delete m_table;
    m_table = table;
    Vector first = table->Row(0);
    for (int i = 1; i < table->rowCount(); ++i) {
        m_spectra.add(MakeSpectrum(first, table->Row(i), QString::number(index)));
        index++;
    }
    ParseData();
}

void SpectraHandler::addSpectrum(const QString& file)
{
    QPair<Vector, Vector> spec;
    if (file.toLower().contains("csv"))
        spec = ToolSet::LoadCSVFile(file);
    else if (file.toLower().contains("absorb"))
        spec = ToolSet::LoadAbsorbFile(file);
    else
        spec = ToolSet::LoadXYFile(file);
    m_spectra.add(MakeSpectrum(spec, file));

    ParseData();
}

void SpectraHandler::addDirectory(const QString& dir, const QString& suffix)
{
    QDirIterator it(dir, QDirIterator::NoIteratorFlags);
    QStringList files;
    while (it.hasNext()) {
        QString file = it.next();
        if (file.contains(suffix))
            files.append(file);
    }
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(
        files.begin(),
        files.end(),
        [&collator](const QString& file1, const QString& file2) {
            return collator.compare(file1, file2) < 0;
        });
    for (const auto& f : files)
        addSpectrum(f);
}

/*! \brief Table of intensities at the selected x values (rows = spectra, columns = x values).
 *
 * Sorts m_x. Each entry is the intensity of the data point nearest to x (XYTrace::nearestIndex, ties to
 * the lower index; no tolerance: an x outside a spectrum takes its first/last point), divided by
 * m_intensity_divisor; 0 for an empty spectrum.
 */
DataTable* SpectraHandler::CompileSimpleTable()
{
    std::sort(
        m_x.begin(),
        m_x.end());

    int cols = m_x.size();
    int rows = m_spectra.size();
    QStringList header;
    DataTable* table = new DataTable(rows, cols, this);

    for (int i = 0; i < m_x.size(); ++i) {
        double x = m_x[i];
        header << QString::number(x);
        for (int j = 0; j < m_spectra.size(); ++j) {
            const XYTrace& spec = m_spectra.at(j);
            const Eigen::Index nearest = spec.nearestIndex(x);
            const double val = (nearest >= 0 && nearest < spec.y().size()) ? spec.y()(nearest) : 0.0;
            table->data(j, i) = val / m_intensity_divisor;
        }
    }
    table->setHeader(header);

    return table;
}

/*! \brief Common x range of all spectra [x_min, x_max] (TraceSet::commonRange; empty spectra are
 * ignored); without spectra [-1e27, 1e27] as before. Resets the selected range to it. */
void SpectraHandler::ParseData()
{
    double min = -1e27, max = 1e27;
    double lo = 0, hi = 0;
    if (m_spectra.commonRange(&lo, &hi)) {
        min = lo;
        max = hi;
    } else if (!m_spectra.isEmpty())
        qWarning() << "SpectraHandler: the spectra do not share an x range";
    x_min = min;
    x_max = max;
    resetRange();
}

void SpectraHandler::resetRange()
{
    m_x_start = x_min;
    m_x_end = x_max;
}

void SpectraHandler::setXRange(double x_start, double x_end)
{
    m_x_start = x_start;
    m_x_end = x_end;
}

/*! \brief Matrix of all spectra (rows) over the x range common to all spectra [x_min, x_max].
 *
 * The x values of the first spectrum in that range go to m_x_ranges. The selected range
 * [m_x_start, m_x_end] is applied later (VarCovarSelect). Spectra on the same axis are copied as they
 * are; if the sampling differs, all spectra are linearly interpolated onto the axis of the first one
 * (TraceSet::resampleToCommonAxis; the first spectrum keeps its values bit for bit). Only if that is
 * impossible (a spectrum with a non-monotone axis or non-finite values) a spectrum with another number
 * of points is left out with a warning, as before. Spectra without a point in the range are left out.
 */
Eigen::MatrixXd SpectraHandler::PrepareMatrix() const
{
    TraceSet ranged;
    for (int i = 0; i < m_spectra.size(); ++i) {
        XYTrace part = m_spectra.at(i).range(x_min, x_max);
        if (part.size() == 0) {
            qWarning().noquote() << QString("SpectraHandler: spectrum %1 has no point in the common range; it is left out.").arg(part.name());
            continue;
        }
        ranged.add(part, m_spectra.idAt(i));
    }
    m_x_ranges.clear();
    if (ranged.isEmpty())
        return Eigen::MatrixXd();

    if (!ranged.hasCommonAxis()) {
        QString error;
        TraceSet resampled = ranged.resampleToCommonAxis(&error);
        if (!resampled.isEmpty())
            ranged = resampled;
        else {
            qWarning().noquote() << QString("SpectraHandler: spectra not interpolated onto one axis (%1)").arg(error);
            const Eigen::Index size = ranged.at(0).size();
            TraceSet kept;
            for (int i = 0; i < ranged.size(); ++i) {
                if (ranged.at(i).size() == size && ranged.at(i).y().size() == size)
                    kept.add(ranged.at(i), ranged.idAt(i));
                else
                    qWarning().noquote() << QString("SpectraHandler: spectrum %1 has %2 points in the common range, the first spectrum %3; it is left out.")
                                                .arg(ranged.at(i).name())
                                                .arg(ranged.at(i).size())
                                                .arg(size);
            }
            ranged = kept;
        }
    }

    const Eigen::VectorXd& axis = ranged.at(0).x();
    m_x_ranges = QVector<double>(axis.data(), axis.data() + axis.size());
    Eigen::MatrixXd matrix(ranged.size(), axis.size());
    for (int i = 0; i < ranged.size(); ++i)
        matrix.row(i) = ranged.at(i).y().transpose();
    return matrix;
}

void SpectraHandler::clearFiles()
{
    m_spectra.clear();
}

void SpectraHandler::PCA()
{
    Eigen::MatrixXd mat = PrepareMatrix();
    Eigen::MatrixXd centered = mat.rowwise() - mat.colwise().mean();
    Eigen::MatrixXd cov = centered.adjoint() * centered;
    /*
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eig(cov);
    std::cout << eig.eigenvalues()/eig.eigenvalues().sum()*100<< std::endl;
    std::cout << eig.eigenvectors().col(eig.eigenvectors().cols()-1) << std::endl;
   std::cout << eig.eigenvectors().row(eig.eigenvectors().rows()-1) << std::endl;
*/
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(centered, Eigen::ComputeThinU | Eigen::ComputeThinV);
    //std::cout << svd.singularValues()/svd.singularValues().sum()*100 << std::endl;
    Eigen::MatrixXd V = svd.matrixV();
    // std::cout << V.rightCols(1) << std::endl;
    //std::cout << svd.matrixU().transpose().leftCols(5) << std::endl;
    // std::cout << V.rightCols(2) *svd.singularValues()(2)* svd.matrixU().transpose().leftCols(2).transpose() << std::endl;
}

QJsonObject SpectraHandler::getSpectraData() const
{
    QJsonObject spectradata, data;

    QStringList files;
    for (int i = 0; i < m_spectra.size(); ++i) {
        const XYTrace& spec = m_spectra.at(i);
        files << spec.source() + QDir::separator() + spec.name();
        // same keys as before WP1; x/y now lossless (TraceJson, still a blank-separated string)
        QJsonObject individual;
        individual["x"] = TraceJson::vectorToString(spec.x());
        individual["y"] = TraceJson::vectorToString(spec.y());
        individual["name"] = spec.name();
        data[QString::number(i + 1)] = individual;
    }
    spectradata["Files"] = files.join("||");
    data["Count"] = m_spectra.size();

    if (qApp->instance()->property("StoreRawData").toBool()) {
        spectradata["Data"] = data;
    }

    spectradata["XValues"] = ToolSet::DoubleVec2String(m_x);
    spectradata["SupraFit"] = qint_version;
    spectradata["XStart"] = m_x_start;
    spectradata["XEnd"] = m_x_end;
    spectradata["IntensityDivisor"] = m_intensity_divisor;

    return spectradata;
}

void SpectraHandler::LoadData(const QJsonObject& data)
{
    if (data.find("Data") == data.end()) {
        QStringList files = data["Files"].toString().split("||");
        for (const auto& file : files)
            addSpectrum(file);
    } else {
        QJsonObject raw = data["Data"].toObject();
        int count = raw["Count"].toInt();
        if (count != 0) {
            for (int i = 1; i <= count; ++i) { // getSpectraData numbers the spectra 1..Count
                QJsonObject block = raw[QString::number(i)].toObject();
                // lossless format and the old 6-digit strings are both read
                XYTrace p;
                QString error;
                if (!XYTrace::fromJson(block, &p, &error)) {
                    qWarning().noquote() << QString("SpectraHandler: stored spectrum %1 is not read: %2").arg(i).arg(error);
                    continue;
                }
                p.setName(block["name"].toString(QString::number(i)));
                p.setSource(QString());
                m_spectra.add(p);
            }
        }
    }
    for (double d : ToolSet::String2DoubleVec(data["XValues"].toString()))
        addXValue(d);

    ParseData();
    m_x_start = data["XStart"].toDouble(m_x_start);
    m_x_end = data["XEnd"].toDouble(m_x_end);
    // files written before the divisor was stored used a fixed 1/1000
    m_intensity_divisor = data["IntensityDivisor"].toDouble(1000.0);
}

/*! \brief Trace of one spectrum; name = file name, source = its directory (both empty without a file name). */
XYTrace SpectraHandler::MakeSpectrum(const Vector& x, const Vector& y, const QString& filename) const
{
    XYTrace trace(x, y);
    if (!filename.isEmpty()) {
        QFileInfo info(filename);
        trace.setName(info.fileName());
        trace.setSource(info.path());
    }
    QString reason;
    if (!trace.isValid(&reason))
        qWarning().noquote() << QString("SpectraHandler: spectrum %1: %2").arg(filename, reason);
    return trace;
}

XYTrace SpectraHandler::MakeSpectrum(const QPair<Vector, Vector>& spect, const QString& filename) const
{
    return MakeSpectrum(spect.first, spect.second, filename);
}

Eigen::MatrixXd SpectraHandler::VarCovarMatrix() const
{
    Eigen::MatrixXd mat = PrepareMatrix();
    /* DataTable* tmp = new DataTable(mat);

    QFile file("export.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);

        stream << tmp->ExportAsString();

        file.close();
    }
    delete tmp;*/
    //std::cout << mat << std::endl;
    Eigen::MatrixXd centered = mat.rowwise() - mat.colwise().mean();
    Eigen::MatrixXd cov = (centered.adjoint() * centered) / double(mat.rows() - 1);
    return cov;
}

/*! \brief Selects x values for the fit from the variance-covariance matrix of the spectra.
 *
 * Starts at the x of largest variance in [m_x_start, m_x_end] and adds x values of least summed
 * absolute correlation with the ones chosen until 3*max_number are chosen or no candidate is left;
 * optionally clusters them (by x) down to max_number: per cluster its x of largest variance or, with
 * averaged, the cluster mean. With max_number or fewer x values there is nothing to cluster.
 * Result in XValues(); returns the same.
 */
QVector<double> SpectraHandler::VarCovarSelect(int max_number, bool do_clustering, bool averaged)
{
    int number = 3 * max_number;
    auto cov = VarCovarMatrix();
    QVector<double> x;
    QVector<int> index_x;
    QMultiMap<double, int> diag;

    for (int i = 0; i < cov.cols() && i < m_x_ranges.size(); ++i) {
        if (m_x_ranges[i] > m_x_end || m_x_ranges[i] < m_x_start)
            continue;
        diag.insert(cov(i, i), i);
    }
    // no spectra or no point in [m_x_start, m_x_end]: nothing to select
    // (was diag.end().value(), a dereference of end())
    if (diag.isEmpty())
        return x;

    // candidates by decreasing variance; every index once, also for equal variances
    // (QMultiMap::value(key) returned one index per variance, the others were unreachable)
    QVector<int> candidates;
    for (auto it = diag.constEnd(); it != diag.constBegin();) {
        --it;
        candidates << it.value();
    }

    index_x << candidates.first();
    while (index_x.size() < number) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        int index = -1;
        double min_cum_var = 1e27;

        for (int current : candidates) {
            if (index_x.contains(current))
                continue;
            double cum_var = 0;
            for (auto curr_index : index_x) {
                double cv = cov(curr_index, current) / sqrt(cov(curr_index, curr_index) * cov(current, current));
                cum_var += qAbs(cv);
            }
            if (cum_var < min_cum_var) {
                min_cum_var = cum_var;
                index = current;
            }
        }
        // all candidates chosen (or none comparable): stop instead of adding index 0 again and again
        if (index < 0)
            break;
        index_x << index;
    }
    QVector<double> variance; // of the x values in x, for the cluster representative
    for (auto s_x : index_x) {
        if (s_x >= m_x_ranges.size())
            continue;
        x << m_x_ranges[s_x];
        variance << cov(s_x, s_x);
    }

    m_x = x;
    if (!do_clustering || max_number < 1 || m_x.size() <= max_number)
        return m_x;

    CxxClusterMatrix matrix;
    QVector<double> cached;
    for (int i = 0; i < m_x.size(); ++i) {
        matrix.push_back(CxxClusterElement({ i }, { m_x[i] }));
        cached << m_x[i];
    }
    CxxCluster cluster;
    cluster.Run(matrix);
    auto storage = cluster.Storage();
    m_x.clear();

    CxxClusterMatrix local_matrix = storage[storage.size() - max_number];
    for (const auto& row : local_matrix) {
        if (!averaged) {
            // member of largest variance (was: always the first member)
            int index = row.first.front();
            for (const auto d : row.first)
                if (variance[d] > variance[index])
                    index = d;
            m_x.push_back(cached[index]);
        } else {
            for (auto d : row.second)
                m_x.push_back(d);
        }
    }
    return m_x; // the clustered selection, as XValues()
}
