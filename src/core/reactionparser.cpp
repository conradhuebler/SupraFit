/*
 * SupraFit - live parser for chemical reaction equations
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

#include <QtCore/QChar>
#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>

#include "src/core/toolset.h"

#include "reactionparser.h"

// Claude Generated.
namespace {

struct Term {
    int coeff = 1;
    QString symbol;
};

// One reaction after a successful split: the left-hand terms and the single right-hand product name.
struct RxLine {
    int line = 0;
    QVector<Term> lhs;
    QString product;
};

// Accepted equilibrium arrows, longest / most specific first so "<=>" wins over the "=" it contains.
// U+21CC (⇌) is the chemical-equilibrium arrow.
const QStringList& arrowTokens()
{
    static const QStringList tokens = { QStringLiteral("<=>"), QStringLiteral("<->"),
        QStringLiteral("->"), QStringLiteral("=>"), QString(QChar(0x21CC)), QStringLiteral("=") };
    return tokens;
}

// Locate the equilibrium arrow: the earliest occurrence, longest token on ties. Returns -1 if none.
int findArrow(const QString& line, int& arrowLen)
{
    int bestPos = -1;
    int bestLen = 0;
    for (const QString& arrow : arrowTokens()) {
        const int pos = line.indexOf(arrow);
        if (pos < 0)
            continue;
        if (bestPos < 0 || pos < bestPos || (pos == bestPos && arrow.size() > bestLen)) {
            bestPos = pos;
            bestLen = arrow.size();
        }
    }
    arrowLen = bestLen;
    return bestPos;
}

// Split a "2 A + B + 3 C" half-reaction into terms. Returns false (with @p error) on malformed input.
bool parseSide(const QString& side, QVector<Term>& terms, QString& error)
{
    static const QRegularExpression symbolRe(QStringLiteral("^[A-Za-z][A-Za-z0-9_]*$"));
    const QStringList parts = side.split('+');
    for (const QString& raw : parts) {
        const QString part = raw.trimmed();
        if (part.isEmpty()) {
            error = QStringLiteral("empty term (stray '+')");
            return false;
        }
        int i = 0;
        while (i < part.size() && part[i].isDigit())
            ++i;
        Term term;
        if (i > 0) {
            term.coeff = part.left(i).toInt();
            term.symbol = part.mid(i).trimmed();
        } else {
            term.symbol = part;
        }
        if (term.coeff <= 0) {
            error = QStringLiteral("coefficient must be a positive integer");
            return false;
        }
        if (!symbolRe.match(term.symbol).hasMatch()) {
            error = QStringLiteral("invalid species symbol '%1'").arg(term.symbol);
            return false;
        }
        terms << term;
    }
    return terms.isEmpty() ? (error = QStringLiteral("empty side"), false) : true;
}

// Resolve a species/component name to its stoichiometry over the elementary components. A name that
// is itself the product of another reaction (an intermediate complex used as a reactant, e.g.
// "A + AB <=> A2B") is substituted recursively. Returns false on a cyclic/undefined reference.
bool resolveStoich(const QString& name,
    const QHash<QString, QVector<Term>>& productDef,
    const QHash<QString, int>& compIndex,
    int nComp,
    QHash<QString, Eigen::VectorXi>& memo,
    QSet<QString>& visiting,
    Eigen::VectorXi& out)
{
    if (memo.contains(name)) {
        out = memo[name];
        return true;
    }
    const auto ci = compIndex.constFind(name);
    if (ci != compIndex.constEnd()) { // elementary component -> unit vector
        Eigen::VectorXi v = Eigen::VectorXi::Zero(nComp);
        v(ci.value()) = 1;
        memo[name] = v;
        out = v;
        return true;
    }
    const auto pd = productDef.constFind(name);
    if (pd == productDef.constEnd())
        return false; // neither component nor product
    if (visiting.contains(name))
        return false; // cycle
    visiting.insert(name);
    Eigen::VectorXi v = Eigen::VectorXi::Zero(nComp);
    for (const Term& t : pd.value()) {
        Eigen::VectorXi sub;
        if (!resolveStoich(t.symbol, productDef, compIndex, nComp, memo, visiting, sub)) {
            visiting.remove(name);
            return false;
        }
        v += t.coeff * sub;
    }
    visiting.remove(name);
    memo[name] = v;
    out = v;
    return true;
}

} // namespace

QString ReactionParser::SpeciesLabel(const QStringList& components, const Eigen::VectorXi& stoich)
{
    QString label;
    for (int i = 0; i < components.size() && i < stoich.size(); ++i) {
        const int n = stoich(i);
        if (n <= 0)
            continue;
        label += components[i];
        if (n > 1)
            label += ToolSet::UnicodeLowerInteger(QString::number(n));
    }
    return label;
}

ReactionSystem ReactionParser::Parse(const QString& text)
{
    ReactionSystem system;
    QVector<RxLine> reactions;
    QSet<QString> productNames;
    bool fatal = false;

    // 1) split each line into left-hand terms and a single right-hand product name
    const QStringList lines = text.split('\n');
    for (int idx = 0; idx < lines.size(); ++idx) {
        QString line = lines[idx];
        line.remove('\r');
        const QString trimmed = line.trimmed();
        const int lineNo = idx + 1;

        if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(QStringLiteral("//")))
            continue; // blank line or comment

        int arrowLen = 0;
        const int arrowPos = findArrow(trimmed, arrowLen);
        if (arrowPos < 0) {
            system.diagnostics << ReactionDiagnostic{ lineNo, false, QStringLiteral("missing reaction arrow (use '<=>')") };
            fatal = true;
            continue;
        }

        QVector<Term> lhs, rhs;
        QString error;
        if (!parseSide(trimmed.left(arrowPos), lhs, error)) {
            system.diagnostics << ReactionDiagnostic{ lineNo, false, QStringLiteral("left side: %1").arg(error) };
            fatal = true;
            continue;
        }
        if (!parseSide(trimmed.mid(arrowPos + arrowLen), rhs, error)) {
            system.diagnostics << ReactionDiagnostic{ lineNo, false, QStringLiteral("right side: %1").arg(error) };
            fatal = true;
            continue;
        }
        if (rhs.size() != 1 || rhs[0].coeff != 1) {
            system.diagnostics << ReactionDiagnostic{ lineNo, false, QStringLiteral("right side must name a single product") };
            fatal = true;
            continue;
        }

        const QString product = rhs[0].symbol;
        // trivial identity "A <=> A": defines nothing, A stays a free component
        if (lhs.size() == 1 && lhs[0].coeff == 1 && lhs[0].symbol == product) {
            system.diagnostics << ReactionDiagnostic{ lineNo, true, QStringLiteral("free component — implicit, skipped") };
            continue;
        }
        reactions << RxLine{ lineNo, lhs, product };
        productNames << product;
    }

    // 2) components = names used as reactants that are never a product (first-appearance order)
    for (const RxLine& r : reactions) {
        for (const Term& t : r.lhs) {
            if (!productNames.contains(t.symbol) && !system.components.contains(t.symbol))
                system.components << t.symbol;
        }
    }
    QHash<QString, int> compIndex;
    for (int i = 0; i < system.components.size(); ++i)
        compIndex[system.components[i]] = i;
    const int nComp = system.components.size();

    // 3) collect product definitions (first definition wins) preserving declaration order
    QHash<QString, QVector<Term>> productDef;
    QHash<QString, int> productLine;
    QStringList productOrder;
    for (const RxLine& r : reactions) {
        if (!productDef.contains(r.product)) {
            productDef[r.product] = r.lhs;
            productLine[r.product] = r.line;
            productOrder << r.product;
        } else {
            system.diagnostics << ReactionDiagnostic{ r.line, true, QStringLiteral("redefinition of '%1' — ignored").arg(r.product) };
        }
    }

    // 4) resolve every product down to the elementary components -> species
    QHash<QString, Eigen::VectorXi> memo;
    for (const QString& product : productOrder) {
        const int lineNo = productLine.value(product, 0);
        QSet<QString> visiting;
        Eigen::VectorXi vec;
        if (!resolveStoich(product, productDef, compIndex, nComp, memo, visiting, vec)) {
            system.diagnostics << ReactionDiagnostic{ lineNo, false, QStringLiteral("cannot resolve '%1' (cyclic or undefined reactant)").arg(product) };
            fatal = true;
            continue;
        }
        if (vec.sum() < 2) {
            system.diagnostics << ReactionDiagnostic{ lineNo, true, QStringLiteral("'%1' is a free component — skipped").arg(product) };
            continue;
        }
        bool duplicate = false;
        for (const ReactionSpecies& existing : system.species) {
            if (existing.stoich == vec) {
                duplicate = true;
                break;
            }
        }
        const QString label = SpeciesLabel(system.components, vec);
        if (duplicate) {
            system.diagnostics << ReactionDiagnostic{ lineNo, true, QStringLiteral("duplicate species '%1' — skipped").arg(label) };
            continue;
        }
        system.species << ReactionSpecies{ label, vec };
        system.diagnostics << ReactionDiagnostic{ lineNo, true, label };
    }

    system.stoich = Eigen::MatrixXi(nComp, system.species.size());
    for (int j = 0; j < system.species.size(); ++j)
        system.stoich.col(j) = system.species[j].stoich;

    system.valid = !fatal && !system.species.isEmpty();
    return system;
}

QString ReactionSystem::toLegacySpeciesString() const
{
    if (components.size() != 2)
        return QString();
    QStringList tokens;
    for (const ReactionSpecies& s : species)
        tokens << QStringLiteral("%1,%2").arg(s.stoich(0)).arg(s.stoich(1));
    return tokens.join('|');
}
