/*
 * SupraFit - metadynamics-like sampling of a least-squares landscape
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
 * Claude Generated (roadmap/landscape_sampling.md, WP-A).
 */

#include "landscapesampler.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <unordered_map>

namespace {
constexpr int MaxDimensionsForMinima = 6; ///< 3^d neighbour lookups per cell
constexpr int MaxGridDimensions = 3; ///< dense bias grid up to here, hill list beyond
}

/*! \brief The history-dependent bias V(x): a dense grid with multilinear interpolation for up to three
 *  variables, a plain hill list (cut off at 4 sigma) for more (Claude Generated). */
class LandscapeSampler::Bias {
public:
    Bias(const std::vector<double>& lower, const std::vector<double>& upper, double hillWidth, int nodes)
        : m_d(int(lower.size()))
        , m_lower(lower)
        , m_width(m_d)
        , m_sigma(m_d)
        , m_grid(m_d <= MaxGridDimensions)
    {
        for (int i = 0; i < m_d; ++i) {
            m_width[i] = upper[i] - lower[i];
            m_sigma[i] = hillWidth * m_width[i];
        }
        if (m_grid) {
            // at least ~2 nodes per sigma, otherwise a hill is not resolved by the interpolation
            m_n = std::max(nodes, int(std::ceil(2.0 / hillWidth)) + 1);
            m_dx.resize(m_d);
            size_t total = 1;
            for (int i = 0; i < m_d; ++i) {
                m_dx[i] = m_width[i] / (m_n - 1);
                total *= size_t(m_n);
            }
            m_values.assign(total, 0.0);
        }
    }

    void Deposit(const std::vector<double>& x, double height)
    {
        if (!m_grid) {
            m_hills.push_back(x);
            m_heights.push_back(height);
            return;
        }
        std::vector<int> first(m_d), last(m_d), idx(m_d);
        for (int i = 0; i < m_d; ++i) {
            first[i] = std::max(0, int(std::floor((x[i] - 4 * m_sigma[i] - m_lower[i]) / m_dx[i])));
            last[i] = std::min(m_n - 1, int(std::ceil((x[i] + 4 * m_sigma[i] - m_lower[i]) / m_dx[i])));
            idx[i] = first[i];
        }
        while (true) {
            double r2 = 0;
            size_t flat = 0;
            for (int i = m_d - 1; i >= 0; --i) {
                const double dxn = (m_lower[i] + idx[i] * m_dx[i] - x[i]) / m_sigma[i];
                r2 += dxn * dxn;
                flat = flat * size_t(m_n) + size_t(idx[i]);
            }
            m_values[flat] += height * std::exp(-0.5 * r2);

            int i = 0;
            for (; i < m_d; ++i) {
                if (++idx[i] <= last[i])
                    break;
                idx[i] = first[i];
            }
            if (i == m_d)
                break;
        }
    }

    double Value(const std::vector<double>& x) const
    {
        if (!m_grid) {
            double v = 0;
            for (size_t h = 0; h < m_hills.size(); ++h) {
                double r2 = 0;
                bool inside = true;
                for (int i = 0; i < m_d; ++i) {
                    const double d = (x[i] - m_hills[h][i]) / m_sigma[i];
                    if (std::abs(d) > 4) {
                        inside = false;
                        break;
                    }
                    r2 += d * d;
                }
                if (inside)
                    v += m_heights[h] * std::exp(-0.5 * r2);
            }
            return v;
        }
        // multilinear interpolation between the 2^d surrounding nodes
        std::vector<int> base(m_d);
        std::vector<double> frac(m_d);
        for (int i = 0; i < m_d; ++i) {
            double t = (x[i] - m_lower[i]) / m_dx[i];
            t = std::min(std::max(t, 0.0), double(m_n - 1));
            base[i] = std::min(int(t), m_n - 2);
            frac[i] = t - base[i];
        }
        double v = 0;
        for (int corner = 0; corner < (1 << m_d); ++corner) {
            double weight = 1;
            size_t flat = 0;
            for (int i = m_d - 1; i >= 0; --i) {
                const int bit = (corner >> i) & 1;
                weight *= bit ? frac[i] : 1 - frac[i];
                flat = flat * size_t(m_n) + size_t(base[i] + bit);
            }
            if (weight != 0)
                v += weight * m_values[flat];
        }
        return v;
    }

    /*! \brief dV/dx by central differences (one-sided at the box edge), step a quarter of a hill sigma. */
    void Gradient(const std::vector<double>& x, const std::vector<double>& upper, std::vector<double>& g) const
    {
        std::vector<double> xp = x;
        g.assign(m_d, 0.0);
        for (int i = 0; i < m_d; ++i) {
            const double h = m_sigma[i] / 4.0;
            const double lo = std::max(m_lower[i], x[i] - h);
            const double hi = std::min(upper[i], x[i] + h);
            xp[i] = hi;
            const double vh = Value(xp);
            xp[i] = lo;
            const double vl = Value(xp);
            xp[i] = x[i];
            g[i] = (hi > lo) ? (vh - vl) / (hi - lo) : 0.0;
        }
    }

private:
    int m_d, m_n = 0;
    std::vector<double> m_lower, m_width, m_sigma, m_dx;
    bool m_grid;
    std::vector<double> m_values; ///< grid mode
    std::vector<std::vector<double>> m_hills; ///< list mode
    std::vector<double> m_heights;
};

LandscapeSampler::LandscapeSampler(Function f, std::vector<double> lower, std::vector<double> upper)
    : m_f(std::move(f))
    , m_lower(std::move(lower))
    , m_upper(std::move(upper))
{
    if (m_lower.empty() || m_lower.size() != m_upper.size())
        throw std::invalid_argument("LandscapeSampler: bounds must be non-empty and of equal size");
    for (size_t i = 0; i < m_lower.size(); ++i)
        if (!(m_upper[i] > m_lower[i]))
            throw std::invalid_argument("LandscapeSampler: upper bound must exceed lower bound");
}

LandscapeSampler::~LandscapeSampler() = default;

double LandscapeSampler::NaturalTemperature(double sseMin, int datapoints, int parameters)
{
    const int dof = std::max(1, datapoints - parameters);
    return 2.0 * sseMin / double(dof);
}

double LandscapeSampler::BiasAt(const std::vector<double>& x) const
{
    return m_bias ? m_bias->Value(x) : 0.0;
}

double LandscapeSampler::FreeEnergyAt(const std::vector<double>& x) const
{
    const double v = BiasAt(x);
    const double dT = m_settings.biasTemperature;
    if (dT <= 0)
        return -v;
    return -(m_settings.temperature + dT) / dT * v;
}

LandscapeResult LandscapeSampler::Run(const std::vector<double>& start)
{
    const LandscapeSettings& s = m_settings;
    const int d = int(m_lower.size());
    if (int(start.size()) != d)
        throw std::invalid_argument("LandscapeSampler: start has the wrong dimension");
    if (!(s.temperature > 0) || !(s.stepSize > 0) || !(s.hillWidth > 0) || s.stride < 1 || s.bins < 2 || s.maxSteps < 1)
        throw std::invalid_argument("LandscapeSampler: temperature, stepSize, hillWidth, stride, bins and maxSteps must be positive");
    if (d * std::log2(double(s.bins)) > 62)
        throw std::invalid_argument("LandscapeSampler: bins^dimensions does not fit the cell key");

    std::vector<double> width(d);
    for (int i = 0; i < d; ++i)
        width[i] = m_upper[i] - m_lower[i];

    LandscapeResult result;
    m_bias = std::make_unique<Bias>(m_lower, m_upper, s.hillWidth, s.biasBins);
    std::mt19937_64 rng(s.seed);
    std::normal_distribution<double> normal(0.0, 1.0);
    std::uniform_real_distribution<double> uniform(0.0, 1.0);

    auto evaluate = [&](const std::vector<double>& x) {
        ++result.evaluations;
        const double v = m_f(x);
        return std::isfinite(v) ? v : std::numeric_limits<double>::infinity();
    };

    // cells of the sampling box, keyed by their bin indices
    std::unordered_map<uint64_t, LandscapeCell> cells;
    auto cellKey = [&](const std::vector<double>& x) {
        uint64_t key = 0;
        for (int i = d - 1; i >= 0; --i) {
            int b = int(std::floor((x[i] - m_lower[i]) / width[i] * s.bins));
            b = std::min(std::max(b, 0), s.bins - 1);
            key = key * uint64_t(s.bins) + uint64_t(b);
        }
        return key;
    };
    // true when the point opens a new cell or improves the best SSE noticeably
    auto record = [&](const std::vector<double>& x, double sse) {
        bool novel = false;
        auto it = cells.find(cellKey(x));
        if (it == cells.end()) {
            cells.emplace(cellKey(x), LandscapeCell{ x, sse });
            novel = true;
        } else if (sse < it->second.sse) {
            it->second.sse = sse;
            it->second.x = x;
        }
        if (sse < result.bestSSE) {
            if (result.bestSSE - sse > s.minimumTolerance * std::abs(result.bestSSE) || !std::isfinite(result.bestSSE))
                novel = true;
            result.bestSSE = sse;
            result.best = x;
        }
        result.maxAcceptedSSE = std::max(result.maxAcceptedSSE, sse);
        return novel;
    };

    std::vector<double> x = start;
    for (int i = 0; i < d; ++i)
        x[i] = std::min(std::max(x[i], m_lower[i]), m_upper[i]);
    double sse = evaluate(x);
    if (!(sse <= s.sseCut)) {
        result.reason = LandscapeResult::Stop::StartAboveCut;
        return result;
    }
    record(x, sse);

    // gradient of the SSE (finite differences over the sampled variables only)
    std::vector<double> gradSSE(d, 0.0), gradBias(d, 0.0);
    auto sseGradient = [&](const std::vector<double>& at, double sseAt, std::vector<double>& g) {
        std::vector<double> xp = at;
        for (int i = 0; i < d; ++i) {
            const double h = s.gradientStep * width[i];
            const bool central = s.centralGradient;
            double hi = std::min(m_upper[i], at[i] + h);
            double lo = central ? std::max(m_lower[i], at[i] - h) : at[i];
            if (hi == at[i] && !central) { // at the upper edge: step backwards
                lo = std::max(m_lower[i], at[i] - h);
                hi = at[i];
            }
            xp[i] = hi;
            const double fh = (hi == at[i]) ? sseAt : evaluate(xp);
            xp[i] = lo;
            const double fl = (lo == at[i]) ? sseAt : evaluate(xp);
            xp[i] = at[i];
            g[i] = (hi > lo && std::isfinite(fh) && std::isfinite(fl)) ? (fh - fl) / (hi - lo) : 0.0;
        }
    };
    const bool langevin = s.sampler == LandscapeSettings::Sampler::Langevin;
    if (langevin)
        sseGradient(x, sse, gradSSE);

    // Adaptive Metropolis: proposal = scale * L z with L the Cholesky factor of 2.38^2/d times the chain covariance
    const bool adapt = s.adaptive;
    // Langevin keeps adapting: the bias widens the explored region, a covariance frozen in the first well would keep
    // the walker from climbing out
    const int adaptSteps = langevin ? s.maxSteps : (s.adaptSteps > 0 ? s.adaptSteps : s.minSteps);
    std::vector<double> chainMean(d, 0.0), chainM2(size_t(d) * d, 0.0), L(size_t(d) * d, 0.0);
    double epsilon = s.stepSize; // Langevin: dimensionless step, 1 once L is the chain covariance
    bool preconditioned = false;
    long chainN = 0, windowProposed = 0, windowAccepted = 0;
    double scale = 1.0;
    for (int i = 0; i < d; ++i)
        L[size_t(i) * d + i] = (langevin ? 1.0 : s.stepSize) * width[i];

    // Metropolis-adjusted Langevin: forward density of a move, M = L L^T the preconditioner
    const bool corrected = langevin && s.langevinCorrection;
    const double langevinTarget = corrected ? s.langevinAcceptance : std::max(s.langevinAcceptance, 0.8);
    auto logProposal = [&](const std::vector<double>& to, const std::vector<double>& from, const std::vector<double>& g) {
        std::vector<double> w(d, 0.0), mg(d, 0.0), y(d, 0.0);
        for (int i = 0; i < d; ++i)
            for (int j = 0; j <= i; ++j)
                w[j] += L[size_t(i) * d + j] * g[i];
        for (int i = 0; i < d; ++i)
            for (int j = 0; j <= i; ++j)
                mg[i] += L[size_t(i) * d + j] * w[j];
        double q = 0;
        for (int i = 0; i < d; ++i) {
            double sum = to[i] - (from[i] - 0.5 * epsilon * epsilon * mg[i] / s.temperature);
            for (int j = 0; j < i; ++j)
                sum -= L[size_t(i) * d + j] * y[j];
            y[i] = sum / L[size_t(i) * d + i];
            q += y[i] * y[i];
        }
        return -0.5 * q / (epsilon * epsilon);
    };

    long proposed = 0, accepted = 0;
    int idle = 0;
    bool novelSinceHill = false;
    result.reason = LandscapeResult::Stop::MaxSteps;
    std::vector<double> xp(d);

    for (long step = 1; step <= s.maxSteps; ++step) {
        result.steps = step;
        if (m_interrupt && m_interrupt()) {
            result.reason = LandscapeResult::Stop::Interrupted;
            break;
        }
        if (m_progress && step % m_progressStride == 0)
            m_progress(step);

        // propose
        if (langevin) {
            // preconditioned Langevin: x' = x - eps^2/2 M grad(U)/kT + eps L z, M = L L^T (L = diag(width) at first)
            m_bias->Gradient(x, m_upper, gradBias);
            std::vector<double> w(d, 0.0), z(d);
            for (int i = 0; i < d; ++i) {
                z[i] = normal(rng);
                const double g = gradSSE[i] + gradBias[i];
                for (int j = 0; j <= i; ++j)
                    w[j] += L[size_t(i) * d + j] * g; // L^T grad
            }
            for (int i = 0; i < d; ++i) {
                double drift = 0, noise = 0;
                for (int j = 0; j <= i; ++j) {
                    drift += L[size_t(i) * d + j] * w[j];
                    noise += L[size_t(i) * d + j] * z[j];
                }
                xp[i] = x[i] - 0.5 * epsilon * epsilon * drift / s.temperature + epsilon * noise;
            }
        } else if (adapt) {
            std::vector<double> z(d);
            for (int i = 0; i < d; ++i)
                z[i] = normal(rng);
            for (int i = 0; i < d; ++i) {
                double move = 0;
                for (int j = 0; j <= i; ++j)
                    move += L[size_t(i) * d + j] * z[j];
                xp[i] = x[i] + scale * move;
            }
        } else {
            for (int i = 0; i < d; ++i)
                xp[i] = x[i] + s.stepSize * width[i] * normal(rng);
        }
        ++proposed;
        ++windowProposed;

        bool inside = true;
        for (int i = 0; i < d; ++i)
            if (xp[i] < m_lower[i] || xp[i] > m_upper[i]) {
                inside = false;
                break;
            }
        if (inside) {
            const double sp = evaluate(xp);
            if (sp <= s.sseCut) { // the wall
                bool accept = true;
                std::vector<double> gradProposal;
                if (!langevin) {
                    const double dU = (sp + m_bias->Value(xp)) - (sse + m_bias->Value(x));
                    accept = dU <= 0 || uniform(rng) < std::exp(-dU / s.temperature);
                } else if (corrected) {
                    // MALA: the Langevin step is a proposal, judged by the Metropolis-Hastings ratio
                    gradProposal.assign(d, 0.0);
                    sseGradient(xp, sp, gradProposal);
                    std::vector<double> gradBiasProposal(d, 0.0), gx(d), gxp(d);
                    m_bias->Gradient(xp, m_upper, gradBiasProposal);
                    for (int i = 0; i < d; ++i) {
                        gx[i] = gradSSE[i] + gradBias[i];
                        gxp[i] = gradProposal[i] + gradBiasProposal[i];
                    }
                    const double dU = (sp + m_bias->Value(xp)) - (sse + m_bias->Value(x));
                    const double logAlpha = -dU / s.temperature + logProposal(x, xp, gxp) - logProposal(xp, x, gx);
                    accept = logAlpha >= 0 || uniform(rng) < std::exp(logAlpha);
                }
                if (accept) {
                    x = xp;
                    sse = sp;
                    ++accepted;
                    ++windowAccepted;
                    novelSinceHill |= record(x, sse);
                    if (m_listener && accepted % m_listenerStride == 0)
                        m_listener(x, sse);
                    if (langevin)
                        gradProposal.empty() ? sseGradient(x, sse, gradSSE) : void(gradSSE = gradProposal);
                }
            }
        }

        if (adapt && step <= adaptSteps) {
            // running mean and covariance of the chain (Welford), every step, stays included
            ++chainN;
            std::vector<double> delta(d);
            for (int i = 0; i < d; ++i) {
                delta[i] = x[i] - chainMean[i];
                chainMean[i] += delta[i] / double(chainN);
            }
            for (int i = 0; i < d; ++i)
                for (int j = 0; j <= i; ++j)
                    chainM2[size_t(i) * d + j] += delta[i] * (x[j] - chainMean[j]);

            if (step % 100 == 0 && chainN >= 20) {
                // scale towards the target acceptance, shape from the chain
                const double acceptance = windowProposed ? double(windowAccepted) / double(windowProposed) : 0.0;
                if (langevin)
                    epsilon = std::min(std::max(epsilon * std::exp(2.0 * (acceptance - langevinTarget)), 1e-4), 5.0);
                else
                    scale = std::min(std::max(scale * std::exp(acceptance - s.targetAcceptance), 0.05), 20.0);
                windowProposed = windowAccepted = 0;

                std::vector<double> C(size_t(d) * d, 0.0), chol(size_t(d) * d, 0.0);
                const double shape = langevin ? 1.0 : 2.38 * 2.38 / double(d);
                bool ok = true;
                for (int i = 0; i < d; ++i)
                    for (int j = 0; j <= i; ++j) {
                        C[size_t(i) * d + j] = shape * chainM2[size_t(i) * d + j] / double(chainN - 1);
                        if (i == j) { // keeps a stuck chain from collapsing the proposal
                            const double floorStep = 1e-3 * s.stepSize * width[i];
                            C[size_t(i) * d + j] += shape * floorStep * floorStep;
                        }
                    }
                for (int i = 0; i < d && ok; ++i)
                    for (int j = 0; j <= i; ++j) {
                        double sum = C[size_t(i) * d + j];
                        for (int k = 0; k < j; ++k)
                            sum -= chol[size_t(i) * d + k] * chol[size_t(j) * d + k];
                        if (i == j) {
                            if (!(sum > 0)) {
                                ok = false;
                                break;
                            }
                            chol[size_t(i) * d + i] = std::sqrt(sum);
                        } else
                            chol[size_t(i) * d + j] = sum / chol[size_t(j) * d + j];
                    }
                if (ok) {
                    L = chol;
                    if (langevin && !preconditioned) {
                        preconditioned = true;
                        epsilon = 1.0; // the chain covariance now carries the length scales
                    }
                }
            }
        }

        if (s.hillHeight > 0 && step % s.stride == 0) {
            const double h = s.biasTemperature > 0 ? s.hillHeight * std::exp(-m_bias->Value(x) / s.biasTemperature) : s.hillHeight;
            m_bias->Deposit(x, h);
            ++result.hills;
            result.lastHillHeight = h;
            const bool converged = s.biasTemperature <= 0 || h <= s.minHillFraction * s.hillHeight;
            idle = (novelSinceHill || !converged) ? 0 : idle + 1;
            novelSinceHill = false;
            if (idle >= s.patience && step >= s.minSteps) {
                result.reason = LandscapeResult::Stop::Patience;
                break;
            }
        }
    }

    result.acceptance = proposed ? double(accepted) / double(proposed) : 0.0;
    result.cellsVisited = long(cells.size());

    // profile: lowest SSE per bin of each variable
    result.profile.assign(d, std::vector<double>(s.bins, std::numeric_limits<double>::infinity()));
    result.profileAt.assign(d, std::vector<double>(s.bins, std::numeric_limits<double>::quiet_NaN()));
    for (const auto& [key, cell] : cells) {
        uint64_t k = key;
        for (int i = 0; i < d; ++i) {
            const int b = int(k % uint64_t(s.bins));
            k /= uint64_t(s.bins);
            if (cell.sse < result.profile[i][b]) {
                result.profile[i][b] = cell.sse;
                result.profileAt[i][b] = cell.x[i];
            }
        }
    }

    // minimum candidates: no visited neighbour cell is lower
    if (d <= MaxDimensionsForMinima) {
        int neighbours = 1;
        for (int i = 0; i < d; ++i)
            neighbours *= 3;
        for (const auto& [key, cell] : cells) {
            std::vector<int> bin(d);
            uint64_t k = key;
            for (int i = 0; i < d; ++i) {
                bin[i] = int(k % uint64_t(s.bins));
                k /= uint64_t(s.bins);
            }
            bool lowest = true;
            for (int n = 0; n < neighbours && lowest; ++n) {
                int rest = n;
                uint64_t nk = 0, stride = 1;
                bool valid = true, self = true;
                for (int i = 0; i < d; ++i) {
                    const int off = rest % 3 - 1;
                    rest /= 3;
                    const int b = bin[i] + off;
                    if (b < 0 || b >= s.bins) {
                        valid = false;
                        break;
                    }
                    self &= off == 0;
                    nk += uint64_t(b) * stride;
                    stride *= uint64_t(s.bins);
                }
                if (!valid || self)
                    continue;
                auto it = cells.find(nk);
                if (it != cells.end() && it->second.sse < cell.sse)
                    lowest = false;
            }
            if (lowest)
                result.minima.push_back(cell);
        }
        std::sort(result.minima.begin(), result.minima.end(),
            [](const LandscapeCell& a, const LandscapeCell& b) { return a.sse < b.sse; });
    }
    return result;
}

double LandscapeConfidenceLevel(double sseMin, int datapoints, int parameters, double fValue)
{
    const int dof = std::max(1, datapoints - parameters);
    return sseMin * (fValue * double(parameters) / double(dof) + 1.0);
}

std::vector<std::vector<LandscapeSegment>> LandscapeIntervals(const LandscapeResult& result, double level,
    const std::vector<double>& lower, const std::vector<double>& upper)
{
    using Edge = LandscapeSegment::Edge;
    std::vector<std::vector<LandscapeSegment>> intervals(result.profile.size());

    for (size_t i = 0; i < result.profile.size(); ++i) {
        const std::vector<double>& p = result.profile[i];
        const std::vector<double>& at = result.profileAt[i];
        const int bins = int(p.size());
        auto below = [&](int b) { return std::isfinite(p[b]) && p[b] <= level; };
        // coordinate where the line through (a, pa) and (b, pb) meets the level
        auto crossing = [&](int inside, int outside) {
            const double x0 = at[inside], x1 = at[outside], y0 = p[inside], y1 = p[outside];
            return y1 == y0 ? x0 : x0 + (level - y0) / (y1 - y0) * (x1 - x0);
        };

        for (int b = 0; b < bins; ++b) {
            if (!below(b))
                continue;
            int first = b, last = b;
            while (last + 1 < bins && below(last + 1))
                ++last;
            b = last;

            LandscapeSegment seg;
            seg.minimum = *std::min_element(p.begin() + first, p.begin() + last + 1);
            const double binWidth = (upper[i] - lower[i]) / bins;

            if (first == 0) {
                seg.lowerEdge = Edge::Box;
                seg.lower = lower[i];
            } else if (std::isfinite(p[first - 1])) {
                seg.lowerEdge = Edge::Crossing;
                seg.lower = crossing(first, first - 1);
            } else {
                seg.lowerEdge = Edge::Unsampled;
                seg.lower = std::min(at[first], lower[i] + first * binWidth);
            }
            if (last == bins - 1) {
                seg.upperEdge = Edge::Box;
                seg.upper = upper[i];
            } else if (std::isfinite(p[last + 1])) {
                seg.upperEdge = Edge::Crossing;
                seg.upper = crossing(last, last + 1);
            } else {
                seg.upperEdge = Edge::Unsampled;
                seg.upper = std::max(at[last], lower[i] + (last + 1) * binWidth);
            }
            intervals[i].push_back(seg);
        }
    }
    return intervals;
}

void LandscapeResult::Merge(const LandscapeResult& other)
{
    steps += other.steps;
    evaluations += other.evaluations;
    hills += other.hills;
    maxAcceptedSSE = std::max(maxAcceptedSSE, other.maxAcceptedSSE);
    cellsVisited += other.cellsVisited; // sum over walkers, cells seen by several are counted several times
    if (other.bestSSE < bestSSE) {
        bestSSE = other.bestSSE;
        best = other.best;
    }
    if (profile.size() < other.profile.size()) {
        profile = other.profile;
        profileAt = other.profileAt;
    } else {
        for (size_t i = 0; i < other.profile.size(); ++i)
            for (size_t b = 0; b < other.profile[i].size() && b < profile[i].size(); ++b)
                if (other.profile[i][b] < profile[i][b]) {
                    profile[i][b] = other.profile[i][b];
                    profileAt[i][b] = other.profileAt[i][b];
                }
    }
    minima.insert(minima.end(), other.minima.begin(), other.minima.end());
    std::sort(minima.begin(), minima.end(), [](const LandscapeCell& a, const LandscapeCell& b) { return a.sse < b.sse; });
}
