/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <Eigen/Dense>

#include <QtCore/QCollator>
#include <QtCore/QDebug>
#include <QtCore/QDirIterator>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QUuid>
#include <QtCore/QVector>

#include <src/core/models/dataclass.h>

#include <src/core/toolset.h>

#include "spectrahandler.h"

SpectraHandler::SpectraHandler(QObject* parent)
    : QObject(parent)
{
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
    Spectrum p = MakeSpectrum(spec, file);
    QUuid uuid;
    QString id = uuid.createUuid().toString();
    m_spectra.insert(id, p);
    m_order << id;

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

DataTable* SpectraHandler::CompileSimpleTable()
{
    std::sort(
        m_x.begin(),
        m_x.end());

    int cols = m_x.size();
    int rows = m_order.size();
    QStringList header;
    DataTable* table = new DataTable(cols, rows, this);
    for (int i = 0; i < m_x.size(); ++i) {
        double x = m_x[i];
        header << QString::number(x);
        for (int j = 0; j < m_order.size(); ++j) {
            auto spec = m_spectra[m_order[j]];
            double val = 0;
            double diff = 1e10;
            for (const auto& p : spec.m_xy) {
                if (qAbs(x - p.x()) < diff) {
                    diff = qAbs(x - p.x());
                    val = p.y();
                }
            }
            table->data(i, j) = val / 1000.0;
        }
    }
    table->setHeader(header);

    return table;
}

void SpectraHandler::ParseData()
{
    double min = -1e27, max = 1e27;
    for (const auto& spec : m_spectra) {
        min = qMax(min, spec.m_spectrum.XMin());
        max = qMin(max, spec.m_spectrum.XMax());
    }
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

Eigen::MatrixXd SpectraHandler::PrepareMatrix() const
{
    int size = 0;
    QVector<Vector> tmp_matrix;
    for (const auto& spectra : m_order) {
        auto spec = m_spectra[spectra];
        auto vector = spec.m_spectrum.getRangedSpectrum(m_x_start, m_x_end);
        if (size == 0) {
            size = vector.size();

            auto l = spec.m_spectrum.getRangedX(m_x_start, m_x_end);
            m_x_ranges = QVector<double>(l.begin(), l.end());
        }
        if (size == vector.size())
            tmp_matrix << vector;
    }

    Eigen::MatrixXd matrix = Eigen::MatrixXd::Ones(tmp_matrix.size(), size);
    for (int i = 0; i < tmp_matrix.size(); ++i)
        for (int j = 0; j < size; ++j) {
            matrix(i, j) = tmp_matrix[i](j);
        }

    DataTable *tmp = new DataTable(matrix);
    QFile file("export.txt");
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&file);

                 stream << tmp->ExportAsString();

                 file.close();
    }
    delete tmp;

    return matrix;
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
    int i = 1;
    for (const auto& spectra : m_order) {
        const auto spec = m_spectra[spectra];
        files << spec.m_path + QDir::separator() + spec.m_filename;
        QJsonObject individual;
        individual["x"] = ToolSet::DoubleList2String(spec.m_spectrum.x());
        individual["y"] = ToolSet::DoubleList2String(spec.m_spectrum.y());
        individual["name"] = spec.m_filename;
        data[QString::number(i)] = individual;
        ++i;
    }
    spectradata["Files"] = files.join("||");
    data["Count"] = m_spectra.count();

    if (qApp->instance()->property("StoreRawData").toBool()) {
        spectradata["Data"] = data;
    }

    spectradata["XValues"] = ToolSet::DoubleVec2String(m_x);
    spectradata["SupraFit"] = qint_version;

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
            for (int i = 1; i < count; ++i) {
                QJsonObject block = raw[QString::number(i)].toObject();
                Spectrum p = MakeSpectrum(ToolSet::String2DoubleEigVec(block["x"].toString()), ToolSet::String2DoubleEigVec(block["y"].toString()));
                p.m_filename = block["name"].toString(QString::number(i));
                QUuid uuid;
                QString id = uuid.createUuid().toString();
                m_spectra.insert(id, p);
                m_order << id;
            }
        }
    }

    for (double d : ToolSet::String2DoubleVec(data["XValues"].toString()))
        addXValue(d);
}

Spectrum SpectraHandler::MakeSpectrum(const Vector& x, const Vector& y, const QString& filename)
{
    QList<QPointF> xy;
    for (int i = 0; i < x.size(); ++i)
        xy << (QPointF(x[i], y[i]));
    PeakPick::spectrum spectrum(x, y);
    Spectrum p;
    p.m_spectrum = spectrum;
    if (!filename.isEmpty()) {
        QFileInfo info(filename);
        p.m_filename = info.fileName();
        p.m_path = info.path();
    }
    p.m_xy = xy;
    return p;
}
Spectrum SpectraHandler::MakeSpectrum(const QPair<Vector, Vector>& spect, const QString& filename)
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

QVector<double> SpectraHandler::VarCovarSelect(int number)
{
    auto cov = VarCovarMatrix();
    QVector<double> x;
    QVector<int> index_x;
    QMultiMap<double, int> diag;
    for (int i = 0; i < cov.cols(); ++i)
        diag.insert(cov(i, i), i);
    if (diag.end().value() >= cov.cols())

        return x;

    index_x << diag.end().value();
    while (index_x.size() < number) {
        double covar = 1e27;
        int index = 0;
        for (auto iter = diag.end(); iter != diag.begin(); --iter) {
            if (index_x.contains(iter.value()))
                continue;
            for (auto curr_index : index_x) {
                double cv = cov(curr_index, iter.value()) / sqrt(cov(curr_index) * cov(iter.value()));
                if (qAbs(cv) < qAbs(covar)) {
                    covar = cv;
                    index = iter.value();
                }
            }
        }
        if (index < diag.size())
            index_x << index;
        else
            break;
    }

    for (auto s_x : index_x)
        x << m_x_ranges[s_x];
    m_x = x;
    return x;
}
