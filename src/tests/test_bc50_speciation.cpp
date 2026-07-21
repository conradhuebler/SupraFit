/*
 * SupraFit - BC50 for reaction-defined (*_any) models must match the fixed-stoichiometry models
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

/* The `*_any` models used to report BC50 = 1/beta11 - the pure 1:1 formula - whatever reaction
 * system they actually carried. On a 1:1/1:2 system that is wrong by up to a factor of 3.7, while
 * the hardcoded nmr_ItoI_ItoII got it right on identical constants. BC50::FromSpeciation now
 * dispatches on the stoichiometry matrix instead; this pins that the two agree, and that a system
 * with no implemented BC50 reports nothing rather than a wrong number.
 * Claude Generated (2026). */

#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

#include <Eigen/Dense>

#include "src/core/bc50.h"

/* QFETCH cannot parse a template argument list containing a comma. */
using SpeciesList = QVector<QPair<int, int>>;
Q_DECLARE_METATYPE(SpeciesList)

class BC50SpeciationTest : public QObject {
    Q_OBJECT

private:
    /*! Stoichiometry matrix (2 components x N species) from a list of (a, b) pairs. */
    static Eigen::MatrixXi stoich(const SpeciesList& species)
    {
        Eigen::MatrixXi m(2, species.size());
        for (int j = 0; j < species.size(); ++j) {
            m(0, j) = species[j].first;
            m(1, j) = species[j].second;
        }
        return m;
    }

private slots:

    /*! A 1:1/1:2 system defined by reactions must give the same BC50 as nmr_ItoI_ItoII does on the
     *  same constants — the case that was wrong before. Species order is deliberately the natural
     *  one (AB then AB2); ordering independence is covered separately. */
    void matchesItoII_data()
    {
        QTest::addColumn<double>("lgK11");
        QTest::addColumn<double>("lgK12");
        QTest::newRow("4.0/3.0") << 4.0 << 3.0;
        QTest::newRow("2.0/1.5") << 2.0 << 1.5;
        QTest::newRow("6.0/2.0") << 6.0 << 2.0;
        QTest::newRow("3.5/4.2") << 3.5 << 4.2;
    }

    void matchesItoII()
    {
        QFETCH(double, lgK11);
        QFETCH(double, lgK12);

        // The *_any models carry CUMULATIVE lg beta per species.
        const QVector<qreal> lgBeta = { lgK11, lgK11 + lgK12 };
        const Eigen::MatrixXi s = stoich({ { 1, 1 }, { 1, 2 } });

        QCOMPARE(BC50::Classify(s), BC50::System::ItoII);
        const double fromSpeciation = BC50::FromSpeciation(s, lgBeta);
        const double hardcoded = BC50::ItoII::BC50(lgK11, lgK12);
        QVERIFY2(std::abs(fromSpeciation - hardcoded) / hardcoded < 1e-12,
            qPrintable(QString("reaction-defined %1 vs hardcoded %2")
                    .arg(fromSpeciation, 0, 'g', 12).arg(hardcoded, 0, 'g', 12)));

        // And it must differ from the 1:1 answer that used to be reported.
        const double wrong = BC50::ItoI::BC50(lgK11);
        qInfo().noquote() << QString("  lgK11=%1 lgK12=%2: correct %3 uM, old 1:1 answer %4 uM (%5 %)")
                                 .arg(lgK11).arg(lgK12)
                                 .arg(fromSpeciation * 1e6, 0, 'g', 6)
                                 .arg(wrong * 1e6, 0, 'g', 6)
                                 .arg((wrong - fromSpeciation) / fromSpeciation * 100, 0, 'f', 1);
    }

    void matchesIItoI()
    {
        const double lgK11 = 4.0, lgK21 = 2.5;
        const QVector<qreal> lgBeta = { lgK11 + lgK21, lgK11 }; // A2B first, then AB
        const Eigen::MatrixXi s = stoich({ { 2, 1 }, { 1, 1 } });

        QCOMPARE(BC50::Classify(s), BC50::System::IItoI);
        QVERIFY(std::abs(BC50::FromSpeciation(s, lgBeta) - BC50::IItoI::BC50(lgK21, lgK11))
            / BC50::IItoI::BC50(lgK21, lgK11) < 1e-12);
    }

    void matchesItoI()
    {
        const Eigen::MatrixXi s = stoich({ { 1, 1 } });
        QCOMPARE(BC50::Classify(s), BC50::System::ItoI);
        QCOMPARE(BC50::FromSpeciation(s, { 4.0 }), BC50::ItoI::BC50(4.0));
    }

    /*! The species order in a reaction system is whatever the user typed, so the dispatcher must
     *  key on the stoichiometry, not on the column index. */
    void isIndependentOfSpeciesOrder()
    {
        const double lgK11 = 3.0, lgK12 = 2.0;
        const double natural = BC50::FromSpeciation(stoich({ { 1, 1 }, { 1, 2 } }), { lgK11, lgK11 + lgK12 });
        const double reversed = BC50::FromSpeciation(stoich({ { 1, 2 }, { 1, 1 } }), { lgK11 + lgK12, lgK11 });
        QVERIFY(natural > 0);
        QCOMPARE(reversed, natural);
    }

    /*! Systems outside the four implemented patterns must report nothing at all. Reporting the
     *  1:1 value for them is exactly the defect this replaces. */
    void rejectsUnsupportedSystems_data()
    {
        QTest::addColumn<SpeciesList>("species");
        QTest::newRow("self-aggregation A2") << SpeciesList{ { 1, 1 }, { 2, 0 } };
        QTest::newRow("2:2 complex") << SpeciesList{ { 1, 1 }, { 2, 2 } };
        QTest::newRow("1:3 complex") << SpeciesList{ { 1, 1 }, { 1, 3 } };
        QTest::newRow("no 1:1 present") << SpeciesList{ { 1, 2 } };
    }

    void rejectsUnsupportedSystems()
    {
        QFETCH(SpeciesList, species);
        const Eigen::MatrixXi s = stoich(species);
        QVector<qreal> lgBeta;
        for (int j = 0; j < species.size(); ++j)
            lgBeta << 4.0;

        QCOMPARE(BC50::Classify(s), BC50::System::Unsupported);
        QVERIFY2(BC50::FromSpeciation(s, lgBeta) < 0, "unsupported system must not yield a value");
        QVERIFY2(BC50::Format_FromSpeciation(s, lgBeta).isEmpty(), "unsupported system must format to nothing");
    }

    /*! Three or more components (the N-component reaction systems) have no BC50 definition yet. */
    void rejectsThreeComponents()
    {
        Eigen::MatrixXi s(3, 1);
        s << 1, 1, 1;
        QCOMPARE(BC50::Classify(s), BC50::System::Unsupported);
        QVERIFY(BC50::FromSpeciation(s, { 4.0 }) < 0);
    }

    /*! A mismatch between the constant count and the species count must not index out of bounds. */
    void rejectsMismatchedParameterCount()
    {
        const Eigen::MatrixXi s = stoich({ { 1, 1 }, { 1, 2 } });
        QVERIFY(BC50::FromSpeciation(s, { 4.0 }) < 0);
        QVERIFY(BC50::FromSpeciation(s, { 4.0, 6.0, 7.0 }) < 0);
    }
};

QTEST_MAIN(BC50SpeciationTest)

#include "test_bc50_speciation.moc"
