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

#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include <QtCore/QString>
#include <QtCore/QVector>

#include <iostream>

namespace BC50 {

namespace ItoI {
    inline qreal BC50(const qreal logK11) { return 1 / qPow(10, logK11); }

    inline QString Format_BC50(const qreal logK11)
    {
        QString result = QString();
        result += QString("<p>BC50<sub>0</sub> = %1 </p>").arg(Print::printConcentration(BC50(logK11), 3));

        return result;
    }
}

namespace IItoI {
    inline qreal BC50(const qreal logK21, const qreal logK11)
    {
        qreal b11 = qPow(10, logK11);
        qreal b21 = qPow(10, (logK11 + logK21));
        return -b11 / b21 / 2.0 + sqrt(qPow(b11 / 2.0 / b21, 2) + 1 / b21);
    }

    inline qreal BFunction(qreal x, const QVector<qreal>& parameter)
    {
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];
        qreal A = -b11 / b21 / 2.0 + sqrt(qPow(b11 / 2.0 / b21, 2) + 1 / b21);
        qreal alpha = x / (1 - x);

        return alpha / (b11 + 2.0 * b21 * A);
    }

    inline qreal A0Function(qreal x, const QVector<qreal>& parameter)
    {
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];
        qreal A = -b11 / b21 / 2.0 + sqrt(qPow(b11 / 2.0 / b21, 2) + 1 / b21);
        qreal alpha = x / (1 - x);

        qreal B = alpha / (b11 + 2.0 * b21 * A);
        return A + b11 * A * B + 2 * b21 * A * A * B;
    }

    inline qreal ABfunction(qreal x, const QVector<qreal>& parameter)
    {
        qreal B = BFunction(x, parameter);
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];
        qreal A = -b11 / b21 / 2.0 + sqrt(qPow(b11 / 2.0 / b21, 2) + 1 / b21);
        return b11 * A * B;
    }

    inline qreal A2Bfunction(qreal x, const QVector<qreal>& parameter)
    {
        qreal B = BFunction(x, parameter);
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];
        qreal A = -b11 / b21 / 2.0 + sqrt(qPow(b11 / 2.0 / b21, 2) + 1 / b21);
        return b21 * A * A * B;
    }

    inline qreal BCfunction(qreal x, const QVector<qreal>& parameter)
    {
        qreal B = BFunction(x, parameter);
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];
        qreal A = -b11 / b21 / 2.0 + sqrt(qPow(b11 / 2.0 / b21, 2) + 1 / b21);
        return 1 / (A + b11 * A * B + 2 * b21 * A * A * B);
    }

    inline QString Format_BC50(const qreal logK21, const qreal logK11)
    {
        QString result = QString();
        QChar mu = QChar(956);

        qreal bc50 = BC50(logK21, logK11);

        result += QString("<p>BC50<sub>0</sub> = %1 %2M</p>").arg(bc50 * 1e6).arg(mu);

        qreal b21 = qPow(10, logK21 + logK11);
        qreal b11 = qPow(10, logK11);



        qreal B = 0, AB = 0, A2B = 0;

        QVector<qreal> parameter;
        parameter << b21 << b11;


        qreal upper = 0.9999;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BFunction;
        B = SimpsonIntegrate(0, upper, function, parameter);

        function = ABfunction;
        AB = SimpsonIntegrate(0, upper, function, parameter);

        function = A2Bfunction;
        A2B = SimpsonIntegrate(0, upper, function, parameter);

        function = BCfunction;
        qreal A = 1 / 2.0 / SimpsonIntegrate(0, upper, function, parameter);
        result += QString("<p>BC50<sub>0</sub> = %1</p>").arg(Print::printConcentration(A, 3));

        result += QString("<p>BC(A0)<sub>0</sub> = %1 </p>").arg(Print::printConcentration(A + AB + 2 * A2B, 3));

        function = A0Function;
        result += QString("<p>BC(A0)<sub>0</sub> = %1</p>").arg(Print::printConcentration(SimpsonIntegrate(0, 1.1, function, parameter, 1e-5)));

        result += QString("<p>BC(B0)<sub>0</sub> = %1</p>").arg(Print::printConcentration(B + AB + A2B, 3));

        result += QString("<p>BC(B)<sub>0</sub> = %1</p>").arg(Print::printConcentration(B, 3));
        result += QString("<p>BC(AB)<sub>0</sub> = %1</p>").arg(Print::printConcentration(AB));
        result += QString("<p>BC(A2B)<sub>0</sub> = %1</p>").arg(Print::printConcentration(A2B));
        result += QString("<p>CR(B)<sub>0</sub> = %1 </p>").arg(1- (b11*A/2.0 + b21*A*A/2.0));
        result += QString("<p>CR(AB)<sub>0</sub> = %1 </p>").arg(b11*A/2.0);
        result += QString("<p>CR(A2B)<sub>0</sub> = %1 </p>").arg(b21*A*A/2.0);

        return result;
    }
}

namespace ItoII {

    inline qreal BC50_Y(qreal x, const QVector<qreal>& parameter)
    {
        if (2 != parameter.size())
            return 0;
        qreal alpha = x / (1 - x);
        qreal b11 = parameter[0];
        qreal b12 = parameter[1];
        //qreal B = -b11 / (2.0 * b12) + sqrt(b11 * b11 / (b12 * b12 * 4.0) + alpha / b12);
        // qreal A = 1 / (b11 + 2.0 * b12 * B);
        return sqrt(b11 * b11 + 4 * b12 * alpha) / (1 + alpha);
    }

    inline qreal BC50(const qreal logK11, const qreal logK12)
    {
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_Y;
        qreal integ = SimpsonIntegrate(0, 1, function, parameter);
        return double(1) / double(2) / integ;
    }

    inline qreal BC50_A0_X(qreal x, const QVector<qreal>& parameter)
    {
        if (2 != parameter.size())
            return 0;
        qreal alpha = x / (1 - x);
        qreal b11 = parameter[0];
        qreal b12 = parameter[1];
        qreal B = -b11 / (2.0 * b12) + sqrt(b11 * b11 / (b12 * b12 * 4.0) + alpha / b12);

        qreal A = 1 / (b11 + 2.0 * b12 * B);
        return A + b11 * A * B + b12 * A * B * B;
    }

    inline qreal BC50_A0(const qreal logK11, const qreal logK12)
    {
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_A0_X;
        qreal integ = SimpsonIntegrate(0, 1, function, parameter);
        return integ;
    }

    inline qreal BC50_B0_X(qreal x, const QVector<qreal>& parameter)
    {
        if (2 != parameter.size())
            return 0;
        qreal alpha = x / (1 - x);
        qreal b11 = parameter[0];
        qreal b12 = parameter[1];

        qreal B = -b11 / (2.0 * b12) + sqrt(b11 * b11 / (b12 * b12 * 4.0) + alpha / b12);
        // qreal A = 1 / (b11 + 2.0 * b12 * B);
        return B;
    }

    inline qreal BC50_B0(const qreal logK11, const qreal logK12)
    {
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_B0_X;
        qreal integ = SimpsonIntegrate(0, 1, function, parameter, 1e-6);
        return integ;
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

    inline QString Format_BC50(const qreal logK11, const qreal logK12)
    {
        QString result = QString();
        /* this will be an interesting part ... */

        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b11 << b12;

        qreal A = 0;
        qreal B = 0, AB = 0, AB2 = 0;

        qreal B0 = BC50_B0(logK11, logK12) * 2;
        B = B0 / 2.0;
        A = 1 / (b11 + 2.0 * b12 * B);
        //qreal A0 = A + A * B * b11 + A * B * B * b12;

        AB = A * B * b11;
        AB2 = A * B * B * b12;

        result += QString("<p>BC50<sub>0</sub> =  %1 </p> ").arg(Print::printConcentration(BC50(logK11, logK12), 3));
        result += QString("<p>BC(A0)<sub>0</sub> = %1 </p>").arg(Print::printConcentration(A + AB + AB2, 3));
        result += QString("<p>BC(B0)<sub>0</sub> = %1</p>").arg(Print::printConcentration(B + AB + 2 * AB2, 3));
        result += QString("<p>BC(A)<sub>0</sub> = %1 </p>").arg(Print::printConcentration(A, 3));
        result += QString("<p>BC(B)<sub>0</sub> = %1</p>").arg(Print::printConcentration(B, 3));
        result += QString("<p>BC(AB)<sub>0</sub> = %1</p>").arg(Print::printConcentration(AB, 3));
        result += QString("<p>BC(AB2)<sub>0</sub> = %1</p>").arg(Print::printConcentration(AB2, 3));

        result += QString("<p>CR(B)<sub>0</sub> = %1 </p>").arg(B / B0);
        result += QString("<p>CR(AB)<sub>0</sub> = %1 </p>").arg(AB / B0);
        result += QString("<p>CR(AB2)<sub>0</sub> = %1 </p>").arg(2 * AB2 / B0);

        return result;
    }
}

namespace IItoII {

    inline qreal BC50_A0_X(qreal x, const QVector<qreal>& parameter)
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

        auto calc_b = [](double a, double b11, double b21, double b12, double alpha) {
            double x1 = b12;
            double x2 = 2 * b21 * a + b11;
            double x3 = -alpha;
            return MaxQuadraticRoot(x1, x2, x3);
        };
        qreal alpha = x / (1 - x);

        qreal A = x / 2;
        qreal B = 0;
        qreal a_1 = 0, b_1 = 0;
        int i;
        for (i = 0; i < 150; ++i) {
            a_1 = A;
            b_1 = B;
            B = calc_b(A, b11, b21, b12, alpha);
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


    inline qreal BC50_A0(const qreal logK21, const qreal logK11, const qreal logK12)
    {
        qreal b21 = qPow(10, logK21 + logK11);
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b21 << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_A0_X;
        qreal integ = SimpsonIntegrate(0, 1, function, parameter);
        return double(1) / double(2) / integ;
    }

    inline qreal BC50_B0_X(qreal x, const QVector<qreal>& parameter)
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

        auto calc_b = [](double a, double b11, double b21, double b12, double alpha) {
            double x1 = b12;
            double x2 = 2 * b21 * a + b11;
            double x3 = -alpha;
            return MaxQuadraticRoot(x1, x2, x3);
        };

        qreal A = x / 2;
        qreal alpha = x / (1 - x);

        qreal B = 0;
        qreal a_1 = 0, b_1 = 0;
        int i;
        for (i = 0; i < 150; ++i) {
            a_1 = A;
            b_1 = B;
            B = calc_b(A, b11, b21, b12, alpha);
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
        //  std::cout << B << " " << b11 * A * B + 2 * b12 * A * B * B + b21 * A * A * B << " " << B/(b11 * A * B + 2 * b12 * A * B * B + b21 * A * A * B) << std::endl;
        return B + b11 * A * B + 2 * b12 * A * B * B + b21 * A * A * B;
    }

    inline qreal BC50_B0(const qreal logK21, const qreal logK11, const qreal logK12)
    {
        qreal b21 = qPow(10, logK21 + logK11);
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b21 << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_B0_X;
        qreal integ = SimpsonIntegrate(0, 1, function, parameter);
        return integ;
    }


    inline QString Format_BC50(const qreal logK21, const qreal logK11, const qreal logK12)
    {
        QString result = QString();

        /* this will be an interesting part ... */

        qreal b21 = qPow(10, logK21 + logK11);
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b21 << b11 << b12;

        qreal A = 0, B = 0, AB = 0, A2B = 0, AB2 = 0;
        double B0 = BC50_B0(logK21, logK11, logK12);
        B = B0 / 2.0;
        qreal q = (b11 + 2 * b12 * B) / (2 * b21);
        A = -q + sqrt(q * q + 1 / b21);
        double A0 = A + A * B * b11 + 2 * b21 * A * A * B + b12 * A * B * B;


        AB = b11 * A * B;
        AB2 = b12 * A * B * B;
        A2B = b21 * A * A * B;
        qreal bc50 = BC50_A0(logK21, logK11, logK12);
        result += QString("<p>BC50<sub>0</sub> =  %1 </p> ").arg(Print::printConcentration(bc50, 3));

        result += QString("<p>BC(A)<sub>0</sub> = %1 </p>").arg(Print::printConcentration(A0, 3));
        result += QString("<p>BC(B)<sub>0</sub> = %1</p>").arg(Print::printConcentration(B0, 3));
        result += QString("<p>BC(A)<sub></sub> = %1 </p>").arg(Print::printConcentration(A, 3));
        result += QString("<p>BC(B)<sub></sub> = %1</p>").arg(Print::printConcentration(B, 3));
        result += QString("<p>BC(A2B)<sub></sub> = %1</p>").arg(Print::printConcentration(A2B, 3));
        result += QString("<p>BC(AB)<sub></sub> = %1</p>").arg(Print::printConcentration(AB, 3));
        result += QString("<p>BC(AB2)<sub></sub> = %1</p>").arg(Print::printConcentration(AB2, 3));

        result += QString("<p>CR(B)<sub>0</sub> = %1 </p>").arg(B / B0);
        result += QString("<p>CR(A2B)<sub>0</sub> = %1 </p>").arg(A2B / B0);
        result += QString("<p>CR(AB)<sub>0</sub> = %1 </p>").arg(AB / B0);
        result += QString("<p>CR(AB2)<sub>0</sub> = %1 </p>").arg(2 * AB2 / B0);

        return result;
    }
}
}
