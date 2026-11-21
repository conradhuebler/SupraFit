/*
 * LineshapeFit - model-free least-squares fit of pseudo-Voigt lines to a trace
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

// Claude Generated: WP4 of roadmap/input_data_editing.md

#include <algorithm>
#include <cmath>

#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>

#include "lineshapefit.h"

namespace {
const double kNaN = std::numeric_limits<double>::quiet_NaN();
const double kInf = std::numeric_limits<double>::infinity();
const double kPi = 3.14159265358979323846;
const double kLn2 = std::log(2.0);
const double kGaussK = 4.0 * kLn2; ///< exponent factor: exp(-4 ln2 u²), u = (x - δ) / w
const double kGaussNorm = std::sqrt(4.0 * kLn2 / kPi); ///< G(δ) · w
const double kLorentzNorm = 2.0 / kPi; ///< L(δ) · w

/// index of every parameter of one line in the parameter vector
struct LineIndex {
    int position = -1, fwhm = -1, area = -1, eta = -1;
};

/// the whole problem: parameter vector with bounds and the map line -> parameters
struct Problem {
    const QVector<LineshapeBlock>* blocks = nullptr;
    LineshapeBaseline baseline = LineshapeBaseline::Constant;
    int nb = 1; ///< baseline parameters per block
    QVector<int> baselineIndex; ///< first baseline parameter of each block
    QVector<QVector<LineIndex>> lines; ///< [block][line]
    QVector<double> centre, halfwidth; ///< of the baseline variable t per block
    Eigen::VectorXd theta, lower, upper;
    QVector<bool> fixed;
    int rows = 0;

    int add(const LineshapeParameter& p)
    {
        const int i = int(theta.size());
        theta.conservativeResize(i + 1);
        lower.conservativeResize(i + 1);
        upper.conservativeResize(i + 1);
        theta(i) = p.value;
        lower(i) = p.lower;
        upper(i) = p.upper;
        fixed << p.fixed;
        return i;
    }

    PseudoVoigtLine line(int b, int l, const Eigen::VectorXd& t) const
    {
        const LineIndex& i = lines[b][l];
        return PseudoVoigtLine{ t(i.position), t(i.fwhm), t(i.area), t(i.eta) };
    }

    double baselineValue(int b, double x, const Eigen::VectorXd& t) const
    {
        if (nb == 0)
            return 0;
        double v = t(baselineIndex[b]);
        if (nb == 2)
            v += t(baselineIndex[b] + 1) * (x - centre[b]) / halfwidth[b];
        return v;
    }

    /// r = model - data, stacked over the blocks
    void residuals(const Eigen::VectorXd& t, Eigen::VectorXd* r) const
    {
        r->resize(rows);
        int row = 0;
        for (int b = 0; b < blocks->size(); ++b) {
            const LineshapeBlock& block = blocks->at(b);
            QVector<PseudoVoigtLine> ls;
            for (int l = 0; l < block.lines.size(); ++l)
                ls << line(b, l, t);
            for (Eigen::Index i = 0; i < block.x.size(); ++i, ++row) {
                double v = baselineValue(b, block.x(i), t);
                for (const PseudoVoigtLine& pl : ls)
                    v += LineshapeFit::evaluate(pl, block.x(i));
                (*r)(row) = v - block.y(i);
            }
        }
    }

    /*! \brief Normal equations A = J^T J and g = J^T r of the analytic Jacobian J = ∂model/∂θ.
     *
     * A row of J is non-zero only in the parameters of its block (baseline, its lines) and in the shared
     * widths/η of its lines, so A is accumulated block by block from the small dense J_b over these
     * columns: cost Σ_b n_b m_b² instead of rows · n² for the stacked J (m_b ≈ 10, n up to a few hundred). */
    void normal(const Eigen::VectorXd& t, const Eigen::VectorXd& r, Eigen::MatrixXd* A, Eigen::VectorXd* g) const
    {
        const int n = int(theta.size());
        A->setZero(n, n);
        g->setZero(n);
        QVector<int> local(n, -1);
        int row = 0;
        for (int b = 0; b < blocks->size(); ++b) {
            const LineshapeBlock& block = blocks->at(b);
            QVector<int> cols;
            auto column = [&](int i) {
                if (local[i] < 0) {
                    local[i] = int(cols.size());
                    cols << i;
                }
                return local[i];
            };
            for (int k = 0; k < nb; ++k)
                column(baselineIndex[b] + k);
            QVector<PseudoVoigtLine> ls;
            QVector<Eigen::Vector4i> lc;
            for (int l = 0; l < block.lines.size(); ++l) {
                ls << line(b, l, t);
                const LineIndex& li = lines[b][l];
                lc << Eigen::Vector4i(column(li.position), column(li.fwhm), column(li.area), column(li.eta));
            }
            const Eigen::Index m = block.x.size();
            Eigen::MatrixXd Jb = Eigen::MatrixXd::Zero(m, cols.size());
            for (Eigen::Index i = 0; i < m; ++i) {
                if (nb >= 1)
                    Jb(i, 0) = 1;
                if (nb == 2)
                    Jb(i, 1) = (block.x(i) - centre[b]) / halfwidth[b];
                for (int l = 0; l < ls.size(); ++l) {
                    Eigen::Vector4d d;
                    LineshapeFit::evaluate(ls[l], block.x(i), &d);
                    // += : a shared width/η collects the derivatives of all its lines
                    for (int q = 0; q < 4; ++q)
                        Jb(i, lc[l](q)) += d(q);
                }
            }
            const Eigen::MatrixXd Ab = Jb.transpose() * Jb;
            const Eigen::VectorXd gb = Jb.transpose() * r.segment(row, m);
            for (int a = 0; a < cols.size(); ++a) {
                (*g)(cols[a]) += gb(a);
                for (int c = 0; c < cols.size(); ++c)
                    (*A)(cols[a], cols[c]) += Ab(a, c);
            }
            for (int i : cols)
                local[i] = -1;
            row += int(m);
        }
    }

    void clamp(Eigen::VectorXd* t) const
    {
        for (Eigen::Index i = 0; i < t->size(); ++i)
            (*t)(i) = std::clamp((*t)(i), lower(i), upper(i));
    }
};

double sumOfSquares(const Eigen::VectorXd& r)
{
    return r.squaredNorm();
}
} // namespace

namespace LineshapeFit {

/*! \brief Pseudo-Voigt with common FWHM (formulas: PseudoVoigtLine). Claude Generated. */
double evaluate(const PseudoVoigtLine& line, double x)
{
    const double w = line.fwhm;
    const double u = (x - line.position) / w;
    const double G = kGaussNorm / w * std::exp(-kGaussK * u * u);
    const double L = kLorentzNorm / w / (1.0 + 4.0 * u * u);
    return line.area * ((1.0 - line.eta) * G + line.eta * L);
}

/*! \brief Value and analytic gradient (Claude Generated). With u = (x - δ) / w:
 *   ∂G/∂δ = G · 2k u / w,  ∂G/∂w = G (2k u² - 1) / w            (k = 4 ln2)
 *   ∂L/∂δ = L · 8u / (w (1 + 4u²)),  ∂L/∂w = -L (1 - 4u²) / (w (1 + 4u²))
 *   ∂f/∂A = (1 - η) G + η L,  ∂f/∂η = A (L - G),  ∂f/∂δ, ∂f/∂w = A [(1 - η) ∂G + η ∂L]. */
double evaluate(const PseudoVoigtLine& line, double x, Eigen::Vector4d* grad)
{
    const double w = line.fwhm, A = line.area, eta = line.eta;
    const double u = (x - line.position) / w;
    const double q = 1.0 + 4.0 * u * u;
    const double G = kGaussNorm / w * std::exp(-kGaussK * u * u);
    const double L = kLorentzNorm / w / q;
    const double dGd = G * 2.0 * kGaussK * u / w;
    const double dGw = G * (2.0 * kGaussK * u * u - 1.0) / w;
    const double dLd = L * 8.0 * u / (w * q);
    const double dLw = -L * (1.0 - 4.0 * u * u) / (w * q);
    if (grad) {
        (*grad)(0) = A * ((1.0 - eta) * dGd + eta * dLd);
        (*grad)(1) = A * ((1.0 - eta) * dGw + eta * dLw);
        (*grad)(2) = (1.0 - eta) * G + eta * L;
        (*grad)(3) = A * (L - G);
    }
    return A * ((1.0 - eta) * G + eta * L);
}

double height(const PseudoVoigtLine& line)
{
    return line.area * ((1.0 - line.eta) * kGaussNorm + line.eta * kLorentzNorm) / line.fwhm;
}

double areaFromHeight(double h, double fwhm, double eta)
{
    return h * fwhm / ((1.0 - eta) * kGaussNorm + eta * kLorentzNorm);
}

/*! \brief Half-height crossing on both sides of a peak (Claude Generated). A side fails if the data end
 * or y starts to rise again (a neighbour line) before it drops below half height. */
double estimateFwhm(const Eigen::VectorXd& x, const Eigen::VectorXd& y, Eigen::Index peak, double baseline)
{
    if (peak < 0 || peak >= y.size() || x.size() != y.size())
        return kNaN;
    const double half = 0.5 * (y(peak) - baseline);
    if (!(half > 0))
        return kNaN;
    auto side = [&](int step) {
        for (Eigen::Index j = peak + step; j >= 0 && j < y.size(); j += step) {
            const double v = y(j) - baseline, before = y(j - step) - baseline;
            if (v < half) {
                // linear interpolation between j - step (above) and j (below)
                const double f = (before - half) / (before - v);
                const double xc = x(j - step) + f * (x(j) - x(j - step));
                return std::abs(xc - x(peak));
            }
            if (v > before)
                return kNaN; // rises again before half height: overlapped side
        }
        return kNaN;
    };
    const double left = side(-1), right = side(+1);
    if (std::isfinite(left) && std::isfinite(right))
        return 2.0 * std::min(left, right);
    if (std::isfinite(left))
        return 2.0 * left;
    if (std::isfinite(right))
        return 2.0 * right;
    return kNaN;
}

LineshapeResult fit(const XYTrace& trace, double from, double to, const QVector<LineshapeLineStart>& lines, const LineshapeOptions& options)
{
    const XYTrace part = trace.range(from, to);
    LineshapeBlock block;
    block.x = part.x();
    block.y = part.y();
    block.lines = lines;
    return fitSeries({ block }, options);
}

/*! \brief Damped Gauss-Newton with projected bounds (method: see the namespace documentation). Claude Generated. */
LineshapeResult fitSeries(const QVector<LineshapeBlock>& blocks, const LineshapeOptions& options)
{
    QElapsedTimer timer;
    timer.start();
    LineshapeResult result;
    auto invalid = [&](const QString& message) {
        result.status = LineshapeStatus::Invalid;
        result.message = message;
        result.milliseconds = timer.nsecsElapsed() * 1e-6;
        return result;
    };

    Problem P;
    P.blocks = &blocks;
    P.baseline = options.baseline;
    P.nb = options.baseline == LineshapeBaseline::None ? 0 : (options.baseline == LineshapeBaseline::Constant ? 1 : 2);
    if (blocks.isEmpty())
        return invalid("no data");

    // ---- checks of the data and the starts
    for (int b = 0; b < blocks.size(); ++b) {
        const LineshapeBlock& block = blocks[b];
        if (block.x.size() != block.y.size())
            return invalid(QString("block %1: x and y differ in length").arg(b + 1));
        if (block.x.size() < 2)
            return invalid(QString("block %1: too few points in the fit range").arg(b + 1));
        if (!block.x.allFinite() || !block.y.allFinite())
            return invalid(QString("block %1: the data contain NaN or infinite values").arg(b + 1));
        if (block.lines.isEmpty())
            return invalid(QString("block %1: no line").arg(b + 1));
        P.rows += int(block.x.size());
    }
    if (options.maxIterations < 1)
        return invalid("the iteration limit must be at least 1");

    // ---- parameter vector: per block baseline, per line position/area (+ own width/η), then shared sets
    struct SharedSet {
        LineshapeParameter fwhm, eta;
        double sumFwhm = 0, sumEta = 0;
        int count = 0, fwhmIndex = -1, etaIndex = -1;
    };
    QMap<int, SharedSet> shared;
    for (const LineshapeBlock& block : blocks)
        for (const LineshapeLineStart& l : block.lines) {
            if (l.shared < 0)
                continue;
            SharedSet& set = shared[l.shared];
            if (set.count == 0) {
                set.fwhm = l.fwhm;
                set.eta = l.eta;
            }
            set.sumFwhm += l.fwhm.value;
            set.sumEta += l.eta.value;
            ++set.count;
        }

    auto checkStart = [](LineshapeParameter p, double lower, double upper, const QString& name, QString* error) {
        p.lower = std::max(p.lower, lower);
        p.upper = std::min(p.upper, upper);
        if (!std::isfinite(p.value) || std::isnan(p.lower) || std::isnan(p.upper) || p.lower > p.upper) {
            *error = QString("%1: invalid start or bounds").arg(name);
            return p;
        }
        if (p.value < p.lower || p.value > p.upper)
            *error = QString("%1: start %2 outside its bounds [%3, %4]").arg(name).arg(p.value).arg(p.lower).arg(p.upper);
        return p;
    };

    P.lines.resize(blocks.size());
    for (int b = 0; b < blocks.size(); ++b) {
        const LineshapeBlock& block = blocks[b];
        const double xmin = block.x.minCoeff(), xmax = block.x.maxCoeff();
        const double span = xmax - xmin;
        P.centre << 0.5 * (xmin + xmax);
        P.halfwidth << (span > 0 ? 0.5 * span : 1.0);
        P.baselineIndex << int(P.theta.size());
        // baseline start from the two ends of the range (the tails of the lines lie above it)
        const Eigen::Index lo = block.x(0) < block.x(block.x.size() - 1) ? 0 : block.x.size() - 1;
        const double yLeft = block.y(lo), yRight = block.y(block.x.size() - 1 - lo);
        if (P.nb == 1)
            P.add(LineshapeParameter{ std::min(yLeft, yRight) });
        else if (P.nb == 2) {
            P.add(LineshapeParameter{ 0.5 * (yLeft + yRight) });
            P.add(LineshapeParameter{ 0.5 * (yRight - yLeft) });
        }
        const double minWidth = std::max(1e-9 * span, 1e-300);
        for (int l = 0; l < block.lines.size(); ++l) {
            const LineshapeLineStart& s = block.lines[l];
            const QString name = QString("block %1, line %2").arg(b + 1).arg(l + 1);
            QString error;
            if (std::isfinite(s.position.value) && (s.position.value < xmin || s.position.value > xmax))
                return invalid(QString("%1: position %2 outside the fit range [%3, %4]").arg(name).arg(s.position.value).arg(xmin).arg(xmax));
            LineIndex index;
            index.position = P.add(checkStart(s.position, -kInf, kInf, name + " position", &error));
            index.area = P.add(checkStart(s.area, -kInf, kInf, name + " area", &error));
            if (s.shared < 0) {
                index.fwhm = P.add(checkStart(s.fwhm, minWidth, kInf, name + " width", &error));
                index.eta = P.add(checkStart(s.eta, 0.0, 1.0, name + " eta", &error));
            }
            if (!(s.fwhm.value > 0))
                error = name + ": the width must be positive";
            if (!error.isEmpty())
                return invalid(error);
            P.lines[b] << index;
        }
    }
    for (auto it = shared.begin(); it != shared.end(); ++it) {
        SharedSet& set = it.value();
        QString error;
        LineshapeParameter fwhm = set.fwhm, eta = set.eta;
        fwhm.value = set.sumFwhm / set.count;
        eta.value = set.sumEta / set.count;
        set.fwhmIndex = P.add(checkStart(fwhm, 1e-300, kInf, QString("shared width %1").arg(it.key()), &error));
        set.etaIndex = P.add(checkStart(eta, 0.0, 1.0, QString("shared eta %1").arg(it.key()), &error));
        if (!error.isEmpty())
            return invalid(error);
    }
    for (int b = 0; b < blocks.size(); ++b)
        for (int l = 0; l < blocks[b].lines.size(); ++l)
            if (blocks[b].lines[l].shared >= 0) {
                P.lines[b][l].fwhm = shared[blocks[b].lines[l].shared].fwhmIndex;
                P.lines[b][l].eta = shared[blocks[b].lines[l].shared].etaIndex;
            }

    const int n = int(P.theta.size());
    int nfree = 0;
    for (int i = 0; i < n; ++i)
        nfree += !P.fixed[i] && P.lower(i) < P.upper(i);
    if (P.rows <= nfree)
        return invalid(QString("too few points (%1) for %2 free parameters").arg(P.rows).arg(nfree));

    // ---- iteration
    Eigen::VectorXd theta = P.theta, r, rTrial;
    P.residuals(theta, &r);
    double sse = sumOfSquares(r);
    if (!std::isfinite(sse))
        return invalid("the model is not finite at the start values");
    Eigen::MatrixXd normalMatrix;
    Eigen::VectorXd g;
    double lambda = 1e-3;
    // roundoff level of the sum of squares: an exact model (noise-free data) cannot get below it
    double ymax = 0;
    for (const LineshapeBlock& block : blocks)
        ymax = std::max(ymax, block.y.cwiseAbs().maxCoeff());
    const double sseFloor = P.rows * std::pow(1e-13 * ymax, 2);
    LineshapeStatus status = LineshapeStatus::MaxIterations;
    int iteration = 0;

    // free parameters not held at a bound (the active set: on the bound and the descent -g points out)
    auto activeFree = [&](const Eigen::VectorXd& t, const Eigen::VectorXd& g) {
        QVector<int> F;
        for (int i = 0; i < n; ++i) {
            if (P.fixed[i] || !(P.lower(i) < P.upper(i)))
                continue;
            if ((t(i) <= P.lower(i) && g(i) > 0) || (t(i) >= P.upper(i) && g(i) < 0))
                continue;
            F << i;
        }
        return F;
    };

    for (iteration = 0; iteration < options.maxIterations; ++iteration) {
        P.normal(theta, r, &normalMatrix, &g);
        const QVector<int> F = activeFree(theta, g);
        if (F.isEmpty() || sse <= sseFloor) {
            status = LineshapeStatus::Converged;
            break;
        }
        Eigen::MatrixXd A(F.size(), F.size());
        Eigen::VectorXd gF(F.size());
        for (int a = 0; a < F.size(); ++a) {
            gF(a) = g(F[a]);
            for (int c = 0; c < F.size(); ++c)
                A(a, c) = normalMatrix(F[a], F[c]);
        }
        // gradient test (MINPACK gtol): every column of J nearly orthogonal to the residual
        const double rnorm = std::sqrt(sse);
        double cosine = 0;
        for (int c = 0; c < F.size(); ++c) {
            const double cn = std::sqrt(A(c, c));
            if (cn > 0)
                cosine = std::max(cosine, std::abs(gF(c)) / (cn * rnorm));
        }
        if (cosine < 1e-12) {
            status = LineshapeStatus::Converged;
            break;
        }
        Eigen::VectorXd D = A.diagonal();
        const double dmax = D.maxCoeff();
        for (Eigen::Index i = 0; i < D.size(); ++i)
            D(i) = std::max(D(i), 1e-12 * dmax + 1e-300);

        bool accepted = false, stalled = false;
        while (!accepted) {
            Eigen::MatrixXd M = A;
            M.diagonal() += lambda * D;
            const Eigen::VectorXd step = M.ldlt().solve(-gF);
            Eigen::VectorXd trial = theta;
            for (int c = 0; c < F.size(); ++c)
                trial(F[c]) += step(c);
            P.clamp(&trial);
            P.residuals(trial, &rTrial);
            const double sseTrial = sumOfSquares(rTrial);
            if (step.allFinite() && std::isfinite(sseTrial) && sseTrial < sse) {
                const double decrease = (sse - sseTrial) / sse;
                // scaled step: every parameter moved by less than tolerance relative to its value
                bool small = true;
                for (int c = 0; c < F.size(); ++c)
                    small = small && std::abs(trial(F[c]) - theta(F[c])) <= options.tolerance * (std::abs(theta(F[c])) + options.tolerance);
                theta = trial;
                r = rTrial;
                sse = sseTrial;
                lambda = std::max(lambda / 3.0, 1e-12);
                accepted = true;
                if (decrease < options.tolerance || small || sse <= sseFloor)
                    stalled = true;
            } else {
                lambda *= 4.0;
                if (lambda > 1e16) {
                    stalled = true; // no decrease possible: a minimum within machine precision
                    break;
                }
            }
        }
        if (stalled) {
            status = LineshapeStatus::Converged;
            ++iteration;
            break;
        }
    }
    result.iterations = iteration;

    // ---- covariance of the free parameters not at a bound
    P.normal(theta, r, &normalMatrix, &g);
    QVector<bool> atBound(n, false);
    for (int i = 0; i < n; ++i)
        atBound[i] = !P.fixed[i] && (theta(i) <= P.lower(i) || theta(i) >= P.upper(i));
    QVector<bool> undetermined(n, false); // parameters of a line whose area vanished
    for (int b = 0; b < blocks.size(); ++b)
        for (int l = 0; l < blocks[b].lines.size(); ++l) {
            const LineIndex& li = P.lines[b][l];
            if (theta(li.area) == 0) {
                undetermined[li.position] = true;
                if (blocks[b].lines[l].shared < 0) {
                    undetermined[li.fwhm] = true;
                    undetermined[li.eta] = true;
                }
            }
        }
    QVector<int> C;
    for (int i = 0; i < n; ++i)
        if (!P.fixed[i] && !atBound[i] && !undetermined[i] && P.lower(i) < P.upper(i))
            C << i;
    Eigen::VectorXd sigma = Eigen::VectorXd::Constant(n, kNaN);
    result.points = P.rows;
    result.freeParameters = int(C.size());
    result.sse = sse;
    result.rms = std::sqrt(sse / P.rows);
    result.s2 = P.rows > C.size() ? sse / double(P.rows - C.size()) : kNaN;
    if (!C.isEmpty()) {
        // J^T J of the columns normalised to unit length
        Eigen::VectorXd norms(C.size());
        for (int c = 0; c < C.size(); ++c)
            norms(c) = std::sqrt(std::max(0.0, normalMatrix(C[c], C[c])));
        Eigen::MatrixXd M(C.size(), C.size());
        for (int a = 0; a < C.size(); ++a)
            for (int c = 0; c < C.size(); ++c)
                M(a, c) = norms(a) > 0 && norms(c) > 0 ? normalMatrix(C[a], C[c]) / (norms(a) * norms(c)) : 0.0;
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigen(M);
        const Eigen::VectorXd ev = eigen.eigenvalues();
        const double emax = ev.maxCoeff(), emin = ev.minCoeff();
        if (!(emax > 0) || !(emin > 1e-12 * emax) || !(norms.minCoeff() > 0)) {
            if (status == LineshapeStatus::Converged) {
                status = LineshapeStatus::Degenerate;
                result.message = QString("the parameters are not independent (J^T J singular, eigenvalue ratio %1); no standard errors").arg(emax > 0 ? emin / emax : 0.0, 0, 'g', 3);
            }
        } else {
            const Eigen::MatrixXd inverse = eigen.eigenvectors() * ev.cwiseInverse().asDiagonal() * eigen.eigenvectors().transpose();
            for (int c = 0; c < C.size(); ++c)
                sigma(C[c]) = std::sqrt(std::max(0.0, inverse(c, c) * result.s2)) / norms(c);
        }
    }
    if (status == LineshapeStatus::MaxIterations)
        result.message = QString("not converged within %1 iterations").arg(options.maxIterations);
    result.status = status;

    // ---- per block
    Eigen::VectorXd rr;
    P.residuals(theta, &rr);
    int row = 0;
    for (int b = 0; b < blocks.size(); ++b) {
        const LineshapeBlock& block = blocks[b];
        LineshapeBlockResult br;
        br.centre = P.centre[b];
        br.halfwidth = P.halfwidth[b];
        br.baseline = Eigen::VectorXd(P.nb);
        for (int k = 0; k < P.nb; ++k)
            br.baseline(k) = theta(P.baselineIndex[b] + k);
        br.x = block.x;
        br.residuals = -rr.segment(row, block.x.size());
        br.sse = br.residuals.squaredNorm();
        row += int(block.x.size());
        for (int l = 0; l < block.lines.size(); ++l) {
            const LineIndex& li = P.lines[b][l];
            br.lines << P.line(b, l, theta);
            br.errors << PseudoVoigtLine{ sigma(li.position), sigma(li.fwhm), sigma(li.area), sigma(li.eta) };
            br.positionAtBound << atBound[li.position];
            br.fwhmAtBound << atBound[li.fwhm];
            br.areaAtBound << atBound[li.area];
        }
        result.blocks << br;
    }
    result.milliseconds = timer.nsecsElapsed() * 1e-6;
#ifdef DEBUG_ON
    qDebug() << "LineshapeFit:" << statusName(result.status) << result.iterations << "iterations," << n << "parameters," << P.rows << "points," << result.milliseconds << "ms";
#endif
    return result;
}

QString statusName(LineshapeStatus status)
{
    switch (status) {
    case LineshapeStatus::Converged:
        return "converged";
    case LineshapeStatus::MaxIterations:
        return "max iterations";
    case LineshapeStatus::Degenerate:
        return "degenerate";
    case LineshapeStatus::Invalid:
        return "invalid";
    }
    return QString();
}

QString baselineName(LineshapeBaseline baseline)
{
    switch (baseline) {
    case LineshapeBaseline::None:
        return "None";
    case LineshapeBaseline::Constant:
        return "Constant";
    case LineshapeBaseline::Linear:
        return "Linear";
    }
    return QString();
}

LineshapeBaseline baselineFromName(const QString& name, LineshapeBaseline fallback)
{
    if (name == "None")
        return LineshapeBaseline::None;
    if (name == "Constant")
        return LineshapeBaseline::Constant;
    if (name == "Linear")
        return LineshapeBaseline::Linear;
    return fallback;
}

} // namespace LineshapeFit

double LineshapeBlockResult::evaluate(double x) const
{
    double v = 0;
    if (baseline.size() >= 1)
        v += baseline(0);
    if (baseline.size() >= 2)
        v += baseline(1) * (x - centre) / halfwidth;
    for (const PseudoVoigtLine& l : lines)
        v += LineshapeFit::evaluate(l, x);
    return v;
}
