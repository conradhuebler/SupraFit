/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include "src/core/libmath.h"

#include <QtCore/QVector>


namespace BC50 {

    inline qreal ItoI_BC50(const qreal logK11) { return 1 / qPow(10, logK11); }

    inline qreal IItoI_ItoI_BC50(const qreal logK21, const qreal logK11)
    {
        qreal b11 = qPow(10, logK11);
        qreal b21 = qPow(10, (logK11 + logK21));
        return -b11 / b21 / double(2) + sqrt(qPow(b11 / double(2) / b21, 2) + 1 / b21);
    }


    inline qreal BC50_ItoI_ItoII_Y(qreal x, const QVector<qreal>& parameter)
    {
        if (2 != parameter.size())
            return 0;
        qreal alpha = x / (1 - x);
        qreal b11 = parameter[0];
        qreal b12 = parameter[1];
        return sqrt(b11 * b11 + 4 * b12 * alpha) / (1 + alpha);
    }

    inline qreal ItoI_ItoII_BC50(const qreal logK11, const qreal logK12)
    {
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_ItoI_ItoII_Y;
        qreal integ = SimpsonIntegrate(0, 1, function, parameter);
        return double(1) / double(2) / integ;
    }

    inline qreal BC50_ItoI_ItoII_SF_Y_0(qreal x, const QVector<qreal>& parameter)
    {
        if (2 != parameter.size())
            return 0;
        qreal b11 = parameter[0];
        qreal b12 = parameter[1];
        qreal A = 1 / (sqrt(b11 * b11 + 4 * b12 * (x / (1 - x))));
        return A;
    }

    inline qreal ItoI_ItoII_BC50_SF(const qreal logK11, const qreal logK12)
    {
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_ItoI_ItoII_SF_Y_0;
        qreal integ = SimpsonIntegrate(0, 1, function, parameter);
        return integ;
    }


    inline qreal BC50_IItoI_ItoI_ItoII_SF_Y(qreal x, const QVector<qreal>& parameter)
    {
        if (3 != parameter.size())
            return 0;
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];
        qreal b12 = parameter[2];

        qreal epsilon = 1e-12;

        auto calc_a = [](double b, double b11, double b21, double b12) {
            double x1 = b21;
            double x2 = 2 * b12 * b + b11;
            double x3 = -1;
            return MaxQuadraticRoot(x1, x2, x3);
        };

        auto calc_b = [](double a, double b11, double b21, double b12, double x) {
            double x1 = b12;
            double x2 = 2 * b21 * a + b11;
            double x3 = -x;
            return MaxQuadraticRoot(x1, x2, x3);
        };

        qreal A = x / 2;
        qreal B = 0;
        qreal a_1 = 0, b_1 = 0;
        int i;
        for (i = 0; i < 150; ++i) {
            a_1 = A;
            b_1 = B;
            B = calc_b(A, b11, b21, b12, x);
            if (B < 0)
                B *= -1;

            A = calc_a(B, b11, b21, b12);
            if (A < 0)
                A *= -1;

            if (qAbs(b21 * a_1 * a_1 * b_1 - b21 * A * A * B) < epsilon && qAbs(b12 * a_1 * b_1 * b_1 - b12 * A * B * B) < epsilon && qAbs(b11 * a_1 * b_1 - b11 * A * B) < epsilon)
                break;
        }
    #ifdef _DEBUG
        std::cout << a_1 << " " << b_1 << " " << b11 * a_1 * b_1 << " " << b21 * a_1 * a_1 * b_1 << " " << b12 * a_1 * b_1 * b_1 << std::endl;
        std::cout << A << " " << B << " " << b11 * A * B << " " << b21 * A * A * B << " " << b12 * A * B * B << std::endl;
        std::cout << "last Change: " << qAbs(b21 * a_1 * a_1 * b_1 - b21 * A * A * B) << " " << qAbs(b12 * a_1 * b_1 * b_1 - b12 * A * B * B) << " " << qAbs(b11 * a_1 * b_1 * b11 * A * B) << std::endl;
        std::cout << "Guess A: " << x / 2 << " .. Final A: " << A << " .. Iterations:" << i << std::endl;
    #endif
        return 1. / (A + b11 * A * B + b12 * A * B * B + 2 * b21 * A * A * B);
    }

    inline qreal IItoI_ItoI_ItoII_BC50(const qreal logK21, const qreal logK11, const qreal logK12)
    {
        qreal b21 = qPow(10, logK21 + logK11);
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b21 << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_IItoI_ItoI_ItoII_SF_Y;
        qreal integ = SimpsonIntegrate(0, 1, function, parameter);
        return double(1) / double(2) / integ;
    }

    inline qreal BC50_IItoI_ItoI_ItoII_SF_Y_0(qreal x, const QVector<qreal>& parameter)
    {
        if (3 != parameter.size())
            return 0;
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];
        qreal b12 = parameter[2];

        qreal epsilon = 1e-12;

        auto calc_a = [](double b, double b11, double b21, double b12) {
            double x1 = b21;
            double x2 = 2 * b12 * b + b11;
            double x3 = -1;
            return MaxQuadraticRoot(x1, x2, x3);
        };

        auto calc_b = [](double a, double b11, double b21, double b12, double x) {
            double x1 = b12;
            double x2 = 2 * b21 * a + b11;
            double x3 = -x;
            return MaxQuadraticRoot(x1, x2, x3);
        };

        qreal A = x / 2;
        qreal B = 0;
        qreal a_1 = 0, b_1 = 0;
        int i;
        for (i = 0; i < 150; ++i) {
            a_1 = A;
            b_1 = B;
            B = calc_b(A, b11, b21, b12, x);
            if (B < 0)
                B *= -1;

            A = calc_a(B, b11, b21, b12);
            if (A < 0)
                A *= -1;

            if (qAbs(b21 * a_1 * a_1 * b_1 - b21 * A * A * B) < epsilon && qAbs(b12 * a_1 * b_1 * b_1 - b12 * A * B * B) < epsilon && qAbs(b11 * a_1 * b_1 - b11 * A * B) < epsilon)
                break;
        }
    #ifdef _DEBUG
        std::cout << a_1 << " " << b_1 << " " << b11 * a_1 * b_1 << " " << b21 * a_1 * a_1 * b_1 << " " << b12 * a_1 * b_1 * b_1 << std::endl;
        std::cout << A << " " << B << " " << b11 * A * B << " " << b21 * A * A * B << " " << b12 * A * B * B << std::endl;
        std::cout << "last Change: " << qAbs(b21 * a_1 * a_1 * b_1 - b21 * A * A * B) << " " << qAbs(b12 * a_1 * b_1 * b_1 - b12 * A * B * B) << " " << qAbs(b11 * a_1 * b_1 - b11 * A * B) << std::endl;
        std::cout << "Guess A: " << x / 2 << " .. Final A: " << A << " .. Iterations:" << i << std::endl;
    #endif
        return A;
    }

    inline qreal IItoI_ItoI_ItoII_BC50_SF(const qreal logK21, const qreal logK11, const qreal logK12)
    {
        qreal b21 = qPow(10, logK21 + logK11);
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b21 << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_IItoI_ItoI_ItoII_SF_Y_0;
        qreal integ = SimpsonIntegrate(0, 1, function, parameter);
        return integ;
    }

}
