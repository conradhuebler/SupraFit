/*
 * SupraFit - accuracy of the BC50 integration at the configurable integration density
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

/* The `BC50IntegrationPoints` setting lets the operator trade BC50 accuracy for Monte-Carlo speed.
 * A setting like that is only safe if the accuracy it buys is pinned somewhere, so this test
 * measures it against an independent high-accuracy reference rather than against stored numbers.
 *
 * The reference is computed here from the ANALYTIC form of the substituted integrand,
 *   int_0^1 sqrt(b11^2 + 4 b12 alpha)/(1+alpha) dx  =  2 int_0^1 t^2 sqrt(b11^2 t^2 + 4 b12 (1-t^2)) dt
 * (x = 1 - t^2, alpha = x/(1-x)), evaluated with far more panels than the production path. That
 * makes it independent of bc50.cpp: a regression there cannot move the reference with it.
 * Claude Generated (2026). */

#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

#include <cmath>

#include "src/core/bc50.h"

class BC50AccuracyTest : public QObject {
    Q_OBJECT

private:
    /*! \brief High-accuracy reference for BC50::ItoII::BC50, independent of bc50.cpp. */
    static double reference(double lgK11, double lgK12, int panels = 2000000)
    {
        const double b11 = std::pow(10, lgK11);
        const double b12 = std::pow(10, lgK11 + lgK12);
        auto f = [b11, b12](double t) { return 2.0 * t * t * std::sqrt(b11 * b11 * t * t + 4.0 * b12 * (1.0 - t * t)); };

        const double h = 1.0 / panels;
        double sum = f(0.0) + f(1.0);
        for (int i = 1; i < panels; ++i)
            sum += (i % 2 ? 4.0 : 2.0) * f(i * h);
        return 1.0 / (2.0 * sum * h / 3.0);
    }

    static void setPoints(int points) { qApp->setProperty("BC50IntegrationPoints", points); }

private slots:

    void cleanup() { setPoints(10000); }

    /*! Accuracy actually delivered at each density the tooltip advertises. Tolerances are one
     *  order of magnitude looser than measured, so this fails on a real regression, not on noise. */
    void accuracyPerDensity_data()
    {
        QTest::addColumn<int>("points");
        QTest::addColumn<double>("tolerance");
        QTest::newRow("100 points") << 100 << 1e-6;
        QTest::newRow("1000 points") << 1000 << 1e-9;
        QTest::newRow("10000 points (default)") << 10000 << 1e-11;
    }

    void accuracyPerDensity()
    {
        QFETCH(int, points);
        QFETCH(double, tolerance);
        setPoints(points);

        struct Case { double lgK11, lgK12; };
        const Case cases[] = { { 4.0, 3.0 }, { 2.0, 1.5 }, { 6.0, 2.0 }, { 3.5, 4.2 } };

        for (const Case& c : cases) {
            const double ref = reference(c.lgK11, c.lgK12);
            const double got = BC50::ItoII::BC50(c.lgK11, c.lgK12);
            const double error = std::abs(got - ref) / ref;
            qInfo().noquote() << QString("  %1 pts, lgK11=%2 lgK12=%3: rel. error %4")
                                     .arg(points).arg(c.lgK11).arg(c.lgK12).arg(error, 0, 'e', 2);
            QVERIFY2(error < tolerance,
                qPrintable(QString("BC50 at %1 points: relative error %2 exceeds %3")
                        .arg(points).arg(error, 0, 'e', 2).arg(tolerance, 0, 'e', 1)));
        }
    }

    /*! The setting must actually reach the integration - a silently ignored control is worse than
     *  no control. 20 panels is coarse enough that the result has to differ from the default. */
    void settingIsHonoured()
    {
        setPoints(10000);
        const double fine = BC50::ItoII::BC50(2.0, 1.5);
        setPoints(20);
        const double coarse = BC50::ItoII::BC50(2.0, 1.5);
        QVERIFY2(std::abs(coarse - fine) / fine > 1e-9,
            "BC50IntegrationPoints had no effect on the result");
    }

    /*! An unset or absurd value must fall back to the default rather than divide by zero - the CLI
     *  and these tests run without the settings registry ever being applied. */
    void fallsBackWhenUnset()
    {
        const double expected = BC50::ItoII::BC50(2.0, 1.5); // at the default 10000

        qApp->setProperty("BC50IntegrationPoints", QVariant());
        const double unset = BC50::ItoII::BC50(2.0, 1.5);
        QVERIFY(std::isfinite(unset));
        QCOMPARE(unset, expected);

        setPoints(0);
        const double zero = BC50::ItoII::BC50(2.0, 1.5);
        QVERIFY2(std::isfinite(zero), "a zero point count must not produce an infinite step");
        QCOMPARE(zero, expected);
    }
};

QTEST_MAIN(BC50AccuracyTest)

#include "test_bc50_accuracy.moc"
