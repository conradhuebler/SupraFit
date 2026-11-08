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
            // ToolSet::DoubleList2String writes 6 significant digits
            for (int i = 0; i < a.size(); ++i) {
                QVERIFY(qAbs(b[i].x() - a[i].x()) <= 1e-5 * qAbs(a[i].x()));
                QVERIFY(qAbs(b[i].y() - a[i].y()) <= 1e-5 * qAbs(a[i].y()));
            }
        }
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

    void shortSpectrumIsReported()
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
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("SpectraHandler.*coarse\\.dat.*26.*51"));
        handler.VarCovarSelect(1, false, false);
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
        // setSpectrafromFile leaves out the last table row because LoadTableFile makes a garbage
        // row from the empty line after a final newline; without that newline a spectrum is lost.
        QEXPECT_FAIL("", "last spectrum lost without final newline (LoadTableFile keeps empty lines)", Continue);
        QCOMPARE(handler.getOrder().size(), 3);
    }
};

QTEST_GUILESS_MAIN(TestSpectraHandler)
#include "test_spectrahandler.moc"
