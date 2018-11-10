/*
 * Tools to calculate Equilibrium Concentrations for different models
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#pragma once

#include "libmath.h"

#include <QtCore/QObject>
#include <QtCore/QRunnable>

namespace ItoI {

inline qreal HostConcentration(qreal host_0, qreal guest_0, const qreal& constant)
{
    qreal K11 = qPow(10, constant);
    qreal a, b, c;
    qreal complex;
    a = K11;
    b = -1 * (K11 * host_0 + K11 * guest_0 + 1);
    c = K11 * guest_0 * host_0;
    complex = MinQuadraticRoot(a, b, c);
    return host_0 - complex;
}
}

namespace IItoI_ItoI {

inline qreal HostConcentration(qreal host_0, qreal guest_0, const QList<qreal>& constants)
{
    if (constants.size() < 2)
        return host_0;

    qreal K21 = constants.first();
    qreal K11 = constants.last();
    qreal host;
    qreal a, b, c;
    a = K11 * K21;
    b = K11 * (2 * K21 * guest_0 - K21 * host_0 + 1);
    c = K11 * (guest_0 - host_0) + 1;
    host = MinCubicRoot(a, b, c, -host_0);
    return host;
}
}

namespace ItoI_ItoII {

inline qreal GuestConcentration(qreal host_0, qreal guest_0, const QList<qreal>& constants)
{
    if (constants.size() < 2)
        return guest_0;

    qreal K12 = constants.last();
    qreal K11 = constants.first();
    qreal a = K11 * K12;
    qreal b = K11 * (2 * K12 * host_0 - K12 * guest_0 + 1);
    qreal c = K11 * (host_0 - guest_0) + 1;
    qreal guest = MinCubicRoot(a, b, c, -guest_0);
    return guest;
}

inline qreal HostConcentration(qreal host_0, qreal guest_0, const QList<qreal>& constants)
{

    if (constants.size() < 2)
        return host_0;

    qreal K12 = constants.last();
    qreal K11 = constants.first();
    qreal guest = ItoI_ItoII::GuestConcentration(host_0, guest_0, constants);
    qreal host;
    host = host_0 / (K11 * guest + K11 * K12 * guest * guest + 1);
    return host;
}
}

class IItoI_ItoI_ItoII_Solver : public QObject, public QRunnable {

public:
    IItoI_ItoI_ItoII_Solver();
    ~IItoI_ItoI_ItoII_Solver();
    virtual void run();
    void setInput(double A_0, double B_0);
    inline void setConstants(const QList<qreal>& parameter) { m_parameter = parameter; }
    inline void setConfig(OptimizerConfig opt_config) { m_opt_config = opt_config; }
    inline QPair<double, double> Concentrations() const { return m_concentration; }
    bool Ok() const { return m_ok; }

private:
    QList<qreal> m_parameter;
    qreal m_A_0, m_B_0;
    QPair<double, double> HostConcentration(double a0, double b0);
    QPair<double, double> m_concentration;
    bool m_ok;
    OptimizerConfig m_opt_config;
};
