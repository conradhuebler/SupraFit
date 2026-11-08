/*
 * Headless tests for the spectra import (SpectraHandler)
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

// Claude Generated: pins the JSON persistence (raw spectra, selected range, intensity divisor),
// the numbers of CompileSimpleTable and the robustness of VarCovarSelect (WP0 of
// roadmap/input_data_editing.md). Synthetic spectra only, written to a QTemporaryDir.

#include <cmath>

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtTest/QtTest>

#include "src/core/models/datatable.h"
#include "src/core/spectrahandler.h"
#include "src/core/toolset.h"

class TestSpectraHandler : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;

    /// Grid 400..600 nm, step 4 nm (51 points)
    static QVector<double> grid()
    {
        QVector<double> x;
        for (int i = 0; i <= 50; ++i)
            x << 400.0 + 4.0 * i;
        return x;
    }

    /// Gaussian band, amplitude grows with the titration step
    static double intensity(double x, int step)
    {
        return 100.0 * (step + 1) * std::exp(-std::pow((x - 500.0) / 30.0, 2)) + 10.0 * step + 0.01 * x;
    }

    /// Writes one "x y" file per step and returns the file names
    QStringList writeXYFiles(int count, const QVector<double>& x)
    {
        QStringList files;
        for (int s = 0; s < count; ++s) {
            QString name = m_dir.filePath(QString("spec_%1.dat").arg(s));
            QFile file(name);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return {};
            QTextStream stream(&file);
            for (double xi : x)
                stream << QString::number(xi, 'g', 17) << " " << QString::number(intensity(xi, s), 'g', 17) << "\n";
            files << name;
        }
        return files;
    }

    /// Table file: first row x, one row of intensities per spectrum
    QString writeTableFile(const QString& name, int count, bool trailingNewline)
    {
        const QVector<double> x = grid();
        QStringList lines;
        QStringList row;
        for (double xi : x)
            row << QString::number(xi, 'g', 17);
        lines << row.join(" ");
        for (int s = 0; s < count; ++s) {
            row.clear();
            for (double xi : x)
                row << QString::number(intensity(xi, s), 'g', 17);
            lines << row.join(" ");
        }
        QString path = m_dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return QString();
        file.write(lines.join("\n").toUtf8());
        if (trailingNewline)
            file.write("\n");
        return path;
    }

    void fillHandler(SpectraHandler& handler, int count)
    {
        for (const QString& f : writeXYFiles(count, grid()))
            handler.addSpectrum(f);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        qApp->setProperty("StoreRawData", true);
    }

    void rawRoundtripKeepsAllSpectra()
    {
        SpectraHandler source;
        fillHandler(source, 3);
        QCOMPARE(source.getOrder().size(), 3);

        QJsonObject json = source.getSpectraData();
        QVERIFY(json.contains("Data"));

        SpectraHandler target;
        target.LoadData(json);
        QCOMPARE(target.getOrder().size(), 3);

        for (int s = 0; s < 3; ++s) {
            QList<QPointF> a = source.Data(source.getOrder()[s]);
            QList<QPointF> b = target.Data(target.getOrder()[s]);
            QCOMPARE(b.size(), a.size());
            // raw data are written losslessly (TraceJson, WP1): bit for bit
            for (int i = 0; i < a.size(); ++i) {
                QCOMPARE(b[i].x(), a[i].x());
                QCOMPARE(b[i].y(), a[i].y());
            }
        }
        // also through the serialised document
        SpectraHandler parsed;
        parsed.LoadData(QJsonDocument::fromJson(QJsonDocument(json).toJson()).object());
        QCOMPARE(parsed.Data(parsed.getOrder()[2]), source.Data(source.getOrder()[2]));
    }

    void oldSixDigitFormatIsRead()
    {
        // block as written before WP1: ToolSet::DoubleList2String (6 significant digits)
        SpectraHandler source;
        fillHandler(source, 2);
        QJsonObject json = source.getSpectraData();
        QJsonObject data = json["Data"].toObject();
        for (int i = 1; i <= 2; ++i) {
            QJsonObject block = data[QString::number(i)].toObject();
            Vector x, y;
            QList<QPointF> points = source.Data(source.getOrder()[i - 1]);
            x.resize(points.size());
            y.resize(points.size());
            for (int k = 0; k < points.size(); ++k) {
                x(k) = points[k].x();
                y(k) = points[k].y();
            }
            block["x"] = ToolSet::DoubleList2String(x);
            block["y"] = ToolSet::DoubleList2String(y);
            data[QString::number(i)] = block;
        }
        json["Data"] = data;
        SpectraHandler target;
        target.LoadData(json);
        QCOMPARE(target.getOrder().size(), 2);
        QList<QPointF> a = source.Data(source.getOrder()[1]);
        QList<QPointF> b = target.Data(target.getOrder()[1]);
        QCOMPARE(b.size(), a.size());
        for (int k = 0; k < a.size(); ++k)
            QVERIFY(qAbs(b[k].y() - a[k].y()) <= 1e-5 * qAbs(a[k].y()));
    }

    void rangeRoundtrip()
    {
        SpectraHandler source;
        fillHandler(source, 3);
        source.setXRange(420.0, 580.0);
        QJsonObject json = source.getSpectraData();
        QCOMPARE(json["XStart"].toDouble(), 420.0);
        QCOMPARE(json["XEnd"].toDouble(), 580.0);

        SpectraHandler target;
        target.LoadData(json);
        QJsonObject again = target.getSpectraData();
        QCOMPARE(again["XStart"].toDouble(), 420.0);
        QCOMPARE(again["XEnd"].toDouble(), 580.0);
    }

    void intensityDivisorRoundtripAndDefault()
    {
        SpectraHandler source;
        fillHandler(source, 3);
        QCOMPARE(source.IntensityDivisor(), 1000.0);
        source.setIntensityDivisor(1.0);
        QJsonObject json = source.getSpectraData();
        QCOMPARE(json["IntensityDivisor"].toDouble(), 1.0);

        SpectraHandler target;
        target.LoadData(json);
        QCOMPARE(target.IntensityDivisor(), 1.0);

        // a file written before the divisor was stored keeps the old /1000
        json.remove("IntensityDivisor");
        SpectraHandler old;
        old.LoadData(json);
        QCOMPARE(old.IntensityDivisor(), 1000.0);
    }

    void compileSimpleTablePinsDivisionBy1000()
    {
        SpectraHandler handler;
        fillHandler(handler, 3);
        // 500.0 is on the grid, 421.0 takes its nearest neighbour 420.0
        handler.setXValues({ 500.0, 421.0 });
        DataTable* table = handler.CompileSimpleTable();
        QCOMPARE(table->rowCount(), 3);
        QCOMPARE(table->columnCount(), 2);
        for (int s = 0; s < 3; ++s) {
            // CompileSimpleTable sorts the x values: column 0 = 421 -> 420, column 1 = 500
            QCOMPARE(table->data(s, 0), intensity(420.0, s) / 1000.0);
            QCOMPARE(table->data(s, 1), intensity(500.0, s) / 1000.0);
        }

        handler.setIntensityDivisor(1.0);
        DataTable* raw = handler.CompileSimpleTable();
        QCOMPARE(raw->data(2, 1), intensity(500.0, 2));
    }

    void varCovarSelectOnEmptyHandler()
    {
        SpectraHandler handler;
        QVector<double> x = handler.VarCovarSelect(3, false, false);
        QVERIFY(x.isEmpty());
        x = handler.VarCovarSelect(3, true, false);
        QVERIFY(x.isEmpty());
    }

    void varCovarSelectStaysInRangeAndReturnsSelection()
    {
        SpectraHandler handler;
        fillHandler(handler, 4);
        handler.setXRange(440.0, 560.0);
        QVector<double> x = handler.VarCovarSelect(2, false, false);
        QCOMPARE(x.size(), 6);
        QCOMPARE(x, handler.XValues());
        for (double d : x)
            QVERIFY(d >= 440.0 && d <= 560.0);

        QVector<double> clustered = handler.VarCovarSelect(2, true, true);
        QCOMPARE(clustered, handler.XValues());
    }

    void coarseSpectrumIsResampled()
    {
        SpectraHandler handler;
        fillHandler(handler, 3);
        QVector<double> coarse;
        for (int i = 0; i <= 25; ++i)
            coarse << 400.0 + 8.0 * i;
        QString name = m_dir.filePath("coarse.dat");
        {
            QFile file(name);
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
            QTextStream stream(&file);
            for (double xi : coarse)
                stream << xi << " " << intensity(xi, 1) << "\n";
        }
        handler.addSpectrum(name);
        // WP1: interpolated onto the axis of the first spectrum instead of being left out
        Eigen::MatrixXd matrix = handler.SpectraMatrix();
        QCOMPARE(matrix.rows(), Eigen::Index(4));
        QCOMPARE(matrix.cols(), Eigen::Index(51));
        const QVector<double> x = grid();
        // the coarse file was written with 6 digits: take its values as read
        const QList<QPointF> stored = handler.Data(handler.getOrder()[3]);
        QCOMPARE(stored.size(), 26);
        for (int i = 0; i < x.size(); ++i) {
            QCOMPARE(matrix(0, i), intensity(x[i], 0));
            const int k = i / 2; // coarse point at or below x[i]
            const double expected = (i % 2 == 0) ? stored[k].y() : 0.5 * (stored[k].y() + stored[k + 1].y());
            QVERIFY(qAbs(matrix(3, i) - expected) <= 1e-12 * qAbs(expected));
        }
        QVERIFY(!handler.VarCovarSelect(1, false, false).isEmpty());
    }

    void sameSamplingMatrixUnchanged()
    {
        SpectraHandler handler;
        fillHandler(handler, 3);
        Eigen::MatrixXd matrix = handler.SpectraMatrix();
        QCOMPARE(matrix.rows(), Eigen::Index(3));
        const QVector<double> x = grid();
        for (int s = 0; s < 3; ++s)
            for (int i = 0; i < x.size(); ++i)
                QCOMPARE(matrix(s, i), intensity(x[i], s));
    }

    void varCovarSelectWithFewCandidates()
    {
        // [496, 504] holds 3 grid points, 3 * max_number = 6 are asked for: only the 3 in range,
        // no repeated index 0 (x = 400, outside the range)
        SpectraHandler handler;
        fillHandler(handler, 4);
        handler.setXRange(496.0, 504.0);
        QVector<double> x = handler.VarCovarSelect(2, false, false);
        QCOMPARE(x.size(), 3);
        for (double d : x)
            QVERIFY(d >= 496.0 && d <= 504.0);
        // clustering to more clusters than candidates does not index out of the cluster storage
        QVector<double> clustered = handler.VarCovarSelect(5, true, false);
        QVERIFY(clustered.size() <= 3);
        for (double d : clustered)
            QVERIFY(d >= 496.0 && d <= 504.0);
    }

    void varCovarSelectWithEqualVariances()
    {
        // symmetric band: x = 500 - d and 500 + d have bit-identical variances
        QStringList files;
        for (int s = 0; s < 4; ++s) {
            QString name = m_dir.filePath(QString("sym_%1.dat").arg(s));
            QFile file(name);
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
            QTextStream stream(&file);
            for (double xi : grid())
                stream << QString::number(xi, 'g', 17) << " "
                       << QString::number(100.0 * (s + 1) * std::exp(-std::pow((xi - 500.0) / 30.0, 2)), 'g', 17) << "\n";
            files << name;
        }
        SpectraHandler handler;
        for (const QString& f : files)
            handler.addSpectrum(f);
        handler.setXRange(492.0, 508.0); // 492 496 500 504 508: variances pairwise equal
        QVector<double> x = handler.VarCovarSelect(2, false, false);
        QCOMPARE(x.size(), 5);
        std::sort(x.begin(), x.end());
        QCOMPARE(x, QVector<double>({ 492.0, 496.0, 500.0, 504.0, 508.0 }));
    }

    void clusteredSelectionTakesLargestVariance()
    {
        // one cluster: without averaging, its member of largest variance is the x of the band maximum
        SpectraHandler handler;
        fillHandler(handler, 4);
        handler.setXRange(440.0, 560.0);
        QVector<double> unclustered = handler.VarCovarSelect(1, false, false);
        QVERIFY(!unclustered.isEmpty());
        QCOMPARE(unclustered.first(), 500.0); // largest variance comes first
        QVector<double> clustered = handler.VarCovarSelect(1, true, false);
        QCOMPARE(clustered, QVector<double>({ 500.0 }));
    }

    void tableFileWithTrailingNewline()
    {
        SpectraHandler handler;
        handler.setSpectrafromFile(writeTableFile("table_nl.txt", 3, true));
        QCOMPARE(handler.getOrder().size(), 3);
        QList<QPointF> last = handler.Data(handler.getOrder().last());
        QCOMPARE(last.size(), 51);
        QCOMPARE(last[25].y(), intensity(500.0, 2));
    }

    void tableFileWithoutTrailingNewline()
    {
        SpectraHandler handler;
        handler.setSpectrafromFile(writeTableFile("table_no_nl.txt", 3, false));
        QCOMPARE(handler.getOrder().size(), 3);
        QList<QPointF> last = handler.Data(handler.getOrder().last());
        QCOMPARE(last[25].y(), intensity(500.0, 2));
    }

    void tableFileWithBlankLinesAndCrLf()
    {
        // empty lines (also "\r" of CRLF files) are skipped by ToolSet::LoadTableFile
        QString path = m_dir.filePath("table_crlf.txt");
        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("400 404 408\r\n1 2 3\r\n\r\n4 5 6\r\n\r\n");
        }
        SpectraHandler handler;
        handler.setSpectrafromFile(path);
        QCOMPARE(handler.getOrder().size(), 2);
        QList<QPointF> last = handler.Data(handler.getOrder().last());
        QCOMPARE(last.size(), 3);
        QCOMPARE(last[2], QPointF(408, 6));
    }
};

QTEST_GUILESS_MAIN(TestSpectraHandler)
#include "test_spectrahandler.moc"
