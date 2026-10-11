/*
 * SupraFit - unit test for the reaction-equation parser
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
 * Validates ReactionParser: arrow syntax -> ordered components + stoichiometry matrix,
 * self-aggregation, N-component systems, arrow variants, and error diagnostics.
 * Claude Generated.
 */

#include <QtTest/QtTest>

#include <Eigen/Core>

#include "src/core/reactionparser.h"

class TestReactionParser : public QObject {
    Q_OBJECT

private:
    // "A2" with a real U+2082 subscript, matching ToolSet::UnicodeLowerInteger output.
    static QString sub(const QString& base, int n)
    {
        return base + QString(QChar(0x2080 + n));
    }

private slots:
    /** Classic 1:1 host-guest: two components, one complex, stoichiometry (1,1). */
    void test_1_1()
    {
        const ReactionSystem s = ReactionParser::Parse("A + B <=> AB");
        QVERIFY(s.valid);
        QCOMPARE(s.components, QStringList() << "A" << "B");
        QCOMPARE(s.species.size(), 1);
        QCOMPARE(s.species[0].label, QStringLiteral("AB"));
        QCOMPARE(s.stoich.rows(), 2);
        QCOMPARE(s.stoich.cols(), 1);
        QCOMPARE(s.stoich(0, 0), 1);
        QCOMPARE(s.stoich(1, 0), 1);
        QCOMPARE(s.toLegacySpeciesString(), QStringLiteral("1,1"));
    }

    /** Self-aggregation: a single component forms a homo-dimer, column (2). */
    void test_self_aggregation()
    {
        const ReactionSystem s = ReactionParser::Parse("2 A <=> A2");
        QVERIFY(s.valid);
        QCOMPARE(s.components, QStringList() << "A");
        QCOMPARE(s.species.size(), 1);
        QCOMPARE(s.species[0].label, sub("A", 2));
        QCOMPARE(s.stoich.rows(), 1);
        QCOMPARE(s.stoich(0, 0), 2);
    }

    /** No-space coefficient prefix "2A" parses like "2 A". */
    void test_coefficient_no_space()
    {
        const ReactionSystem s = ReactionParser::Parse("2A + B <=> A2B");
        QVERIFY(s.valid);
        QCOMPARE(s.species.size(), 1);
        QCOMPARE(s.stoich(0, 0), 2);
        QCOMPARE(s.stoich(1, 0), 1);
        QCOMPARE(s.species[0].label, sub("A", 2) + "B");
    }

    /** Three components (competitive binding) keep first-appearance order A,B,C. */
    void test_three_components()
    {
        const ReactionSystem s = ReactionParser::Parse("A + B <=> AB\nA + C <=> AC");
        QVERIFY(s.valid);
        QCOMPARE(s.components, QStringList() << "A" << "B" << "C");
        QCOMPARE(s.species.size(), 2);
        QCOMPARE(s.stoich.rows(), 3);
        QCOMPARE(s.stoich.cols(), 2);
        // AB = (1,1,0)
        QCOMPARE(s.stoich(0, 0), 1);
        QCOMPARE(s.stoich(1, 0), 1);
        QCOMPARE(s.stoich(2, 0), 0);
        // AC = (1,0,1)
        QCOMPARE(s.stoich(0, 1), 1);
        QCOMPARE(s.stoich(1, 1), 0);
        QCOMPARE(s.stoich(2, 1), 1);
        QVERIFY(s.toLegacySpeciesString().isEmpty()); // not a 2-component system
    }

    /** All accepted arrow variants produce the same system. */
    void test_arrow_variants()
    {
        const QStringList arrows = { "<=>", "=", "<->", "->", "=>", QString(QChar(0x21CC)) };
        for (const QString& arrow : arrows) {
            const ReactionSystem s = ReactionParser::Parse(QStringLiteral("A + B %1 AB").arg(arrow));
            QVERIFY2(s.valid, qPrintable(QStringLiteral("arrow '%1' failed").arg(arrow)));
            QCOMPARE(s.species.size(), 1);
            QCOMPARE(s.stoich(0, 0), 1);
            QCOMPARE(s.stoich(1, 0), 1);
        }
    }

    /** Blank lines and #/// comments are ignored; free monomers are skipped. */
    void test_comments_and_free_monomer()
    {
        const ReactionSystem s = ReactionParser::Parse("# a system\nA <=> A\n\n// note\nA + B <=> AB");
        QVERIFY(s.valid);
        QCOMPARE(s.components, QStringList() << "A" << "B");
        QCOMPARE(s.species.size(), 1); // "A <=> A" is a free monomer, not a species
        QCOMPARE(s.species[0].label, QStringLiteral("AB"));
    }

    /** Duplicate stoichiometry is reported and only counted once. */
    void test_duplicate_species()
    {
        const ReactionSystem s = ReactionParser::Parse("A + B <=> AB\nA + B <=> AB");
        QVERIFY(s.valid);
        QCOMPARE(s.species.size(), 1);
    }

    /** A line without an arrow is a fatal parse error and invalidates the system. */
    void test_missing_arrow()
    {
        const ReactionSystem s = ReactionParser::Parse("A + B AB");
        QVERIFY(!s.valid);
        QCOMPARE(s.species.size(), 0);
        QCOMPARE(s.diagnostics.size(), 1);
        QVERIFY(!s.diagnostics[0].ok);
    }

    /** A malformed term (stray '+') is reported as an error. */
    void test_malformed_term()
    {
        const ReactionSystem s = ReactionParser::Parse("A + <=> AB");
        QVERIFY(!s.valid);
        QVERIFY(s.diagnostics.size() >= 1);
        QVERIFY(!s.diagnostics[0].ok);
    }

    /** Stepwise binding: an intermediate complex used as a reactant resolves to the components. */
    void test_stepwise_species_as_reactant()
    {
        // A + AB -> A2B means A2B = A + (A+B) = 2A + B, and AB is NOT a third component.
        const ReactionSystem s = ReactionParser::Parse("A + B -> AB\nA + AB -> A2B");
        QVERIFY(s.valid);
        QCOMPARE(s.components, QStringList() << "A" << "B"); // two components, not three
        QCOMPARE(s.species.size(), 2);
        QCOMPARE(s.species[0].label, QStringLiteral("AB"));
        QCOMPARE(s.species[0].stoich(0), 1);
        QCOMPARE(s.species[0].stoich(1), 1);
        QCOMPARE(s.species[1].label, sub("A", 2) + "B");
        QCOMPARE(s.species[1].stoich(0), 2); // 2 A
        QCOMPARE(s.species[1].stoich(1), 1); // 1 B
    }

    /** Stepwise and direct notations for the same system are equivalent. */
    void test_stepwise_equals_direct()
    {
        const ReactionSystem stepwise = ReactionParser::Parse("A + B <=> AB\nA + AB <=> A2B");
        const ReactionSystem direct = ReactionParser::Parse("A + B <=> AB\n2 A + B <=> A2B");
        QVERIFY(stepwise.valid && direct.valid);
        QCOMPARE(stepwise.components, direct.components);
        QCOMPARE(stepwise.stoich.rows(), direct.stoich.rows());
        QCOMPARE(stepwise.stoich.cols(), direct.stoich.cols());
        QVERIFY(stepwise.stoich == direct.stoich);
    }

    /** Forward references (a complex used before it is defined) still resolve. */
    void test_forward_reference()
    {
        const ReactionSystem s = ReactionParser::Parse("A + AB <=> A2B\nA + B <=> AB");
        QVERIFY(s.valid);
        QCOMPARE(s.components, QStringList() << "A" << "B");
        QCOMPARE(s.species.size(), 2);
    }

    /** A cyclic definition is reported as an error instead of hanging/crashing. */
    void test_cycle_is_error()
    {
        const ReactionSystem s = ReactionParser::Parse("A + Y <=> X\nB + X <=> Y");
        QVERIFY(!s.valid);
        bool hasError = false;
        for (const ReactionDiagnostic& d : s.diagnostics)
            hasError = hasError || !d.ok;
        QVERIFY(hasError);
    }

    /** Empty input yields an empty, invalid system without crashing. */
    void test_empty()
    {
        const ReactionSystem s = ReactionParser::Parse("   \n\n");
        QVERIFY(!s.valid);
        QCOMPARE(s.species.size(), 0);
        QCOMPARE(s.components.size(), 0);
    }
};

QTEST_MAIN(TestReactionParser)

#include "test_reactionparser.moc"
