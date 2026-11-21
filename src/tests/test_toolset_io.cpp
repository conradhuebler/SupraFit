/*
 * SupraFit - tests of the file helpers of ToolSet (toolset_io.cpp)
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

/* Claude Generated (R0 of roadmap/data_pipeline_work_packages.md):
 * - LoadITCFile: a missing file is replaced by the file of the same name in "lastdir", with a warning that names it. */

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

#include <vector>

#include "libpeakpick/peakpick.h"

#include "src/core/toolset.h"

#ifndef SAMPLE_DIR
#define SAMPLE_DIR "."
#endif

class TestToolSetIo : public QObject {
    Q_OBJECT

private slots:
    /// the file of a stored path that is gone is read from "lastdir"; the warning names the file read
    void itcFileFromLastDirIsReadVisibly()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString copy = dir.filePath("synthetic.itc");
        QVERIFY(QFile::copy(QString(SAMPLE_DIR) + "/synthetic.itc", copy));
        qApp->setProperty("lastdir", dir.path());

        QString filename = QStringLiteral("/no/such/directory/synthetic.itc");
        std::vector<PeakPick::Peak> peaks;
        QVector<qreal> inject;
        qreal offset = 0, freq = 0;
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QRegularExpression::escape(copy)));
        const QPair<PeakPick::spectrum, QJsonObject> pair = ToolSet::LoadITCFile(filename, &peaks, offset, freq, inject);
        QCOMPARE(filename, copy);
        QVERIFY(pair.first.size() > 0);
        QVERIFY(!inject.isEmpty());

        // an existing file is read without a warning
        QString direct = copy;
        ToolSet::LoadITCFile(direct, &peaks, offset, freq, inject);
        QCOMPARE(direct, copy);
    }
};

QTEST_GUILESS_MAIN(TestToolSetIo)
#include "test_toolset_io.moc"
