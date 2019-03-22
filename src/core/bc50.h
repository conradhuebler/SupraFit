/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

    inline QPair<qreal, qreal> ABPair(qreal x, const QVector<qreal>& parameter)
    {
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];
        qreal quot = b11 / 2.0 / b21;
        qreal A = -b11 / b21 / 2.0 + sqrt(quot * quot + 1 / b21);
        qreal alpha = x / (1 - x);

        qreal B = alpha / (b11 + 2.0 * b21 * A);
        return QPair<qreal, qreal>(A,B);
    }

    inline qreal AFunction(qreal x, const QVector<qreal>& parameter)
    {
        QPair<qreal, qreal> AB = ABPair(x, parameter);
        return AB.first;
    }

    inline qreal BFunction(qreal x, const QVector<qreal>& parameter)
    {
        QPair<qreal, qreal> AB = ABPair(x, parameter);
        return AB.second;
    }

    inline qreal A0Function(qreal x, const QVector<qreal>& parameter)
    {
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];

        QPair<qreal, qreal> AB = ABPair(x, parameter);

        qreal A  = AB.first;
        qreal B  = AB.second;

        return A + b11 * A * B + 2 * b21 * A * A * B;
    }

    inline qreal B0Function(qreal x, const QVector<qreal>& parameter)
    {
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];

        QPair<qreal, qreal> AB = ABPair(x, parameter);

        qreal A  = AB.first;
        qreal B  = AB.second;

        return B + b11 * A * B + b21 * A * A * B;
    }

    inline qreal ABfunction(qreal x, const QVector<qreal>& parameter)
    {
        qreal b11 = parameter[1];

        QPair<qreal, qreal> AB = ABPair(x, parameter);

        qreal A  = AB.first;
        qreal B  = AB.second;

        return b11 * A * B;
    }

    inline qreal A2Bfunction(qreal x, const QVector<qreal>& parameter)
    {
        qreal b21 = parameter[0];

        QPair<qreal, qreal> AB = ABPair(x, parameter);

        qreal A  = AB.first;
        qreal B  = AB.second;

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

        /* the next line is outdate with still included, historical stuff */
//        result += QString("<p>BC50<sub>0</sub> = %1 %2M</p>").arg(bc50 * 1e6).arg(mu);

        qreal b21 = qPow(10, logK21 + logK11);
        qreal b11 = qPow(10, logK11);


        qreal A = 0, B = 0, AB = 0, A2B = 0, A0 = 0, B0 = 0;
        qreal rCD_B = 0, rCD_A2B = 0, rCD_AB = 0;
        QVector<qreal> parameter;
        parameter << b21 << b11;


        qreal upper = 1;
        qreal prec = 1e-4;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BFunction;
        B = SimpsonIntegrate(0, upper, function, parameter, prec);

        function = AFunction;
        A = SimpsonIntegrate(0, upper, function, parameter, prec);

        function = ABfunction;
        AB = SimpsonIntegrate(0, upper, function, parameter, prec);

        function = A2Bfunction;
        A2B = SimpsonIntegrate(0, upper, function, parameter, prec);

#ifdef conservative
        function = A0Function;
        A0 = SimpsonIntegrate(0, upper, function, parameter, prec);

        function = B0Function;
        B0 = SimpsonIntegrate(0, upper, function, parameter, prec);

        rCD_B = B/B0;
        rCD_A2B = A2B/B0;
        rCD_AB = AB/B0;
#else
        A0 = A + 2*A2B + AB;
        B0 = B + A2B + AB;

        rCD_B = 1-(b11*A/2.0 + b21*A*A/2.0);
        rCD_A2B = b21*A*A/2.0;
        rCD_AB = b11*A/2.0;
#endif


        result += QString("<p>BC50<sub>0</sub> = %1</p>").arg(Print::printConcentration(A, 3));
        result += QString("<p>BC(A0)<sub>0</sub> = %1</p>").arg(Print::printConcentration(A0));
        result += QString("<p>BC(B0)<sub>0</sub> = %1</p>").arg(Print::printConcentration(B0, 3));
        result += QString("<p>BC(A)<sub>0</sub> = %1</p>").arg(Print::printConcentration(A, 3));
        result += QString("<p>BC(B)<sub>0</sub> = %1</p>").arg(Print::printConcentration(B, 3));
        result += QString("<p>BC(A2B)<sub>0</sub> = %1</p>").arg(Print::printConcentration(A2B));
        result += QString("<p>BC(AB)<sub>0</sub> = %1</p>").arg(Print::printConcentration(AB));
        result += QString("<p>CR(B)<sub>0</sub> = %1 </p>").arg(rCD_B);
        result += QString("<p>CR(A2B)<sub>0</sub> = %1 </p>").arg(rCD_A2B);
        result += QString("<p>CR(AB)<sub>0</sub> = %1 </p>").arg(rCD_AB);

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
/*
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

    inline qreal BC50_A_X(qreal x, const QVector<qreal>& parameter)
    {
        if (2 != parameter.size())
            return 0;

        qreal b11 = parameter[0];
        qreal b12 = parameter[1];
        qreal K = 4*b12/b11/b11;
        qreal alpha = x / (1 - x);

        // qreal A = sqrt(x-1)/(sqrt(b11*b11*(1-x)+4*b12*x));
        qreal B = -b11 / (2.0 * b12) + sqrt(b11 * b11 / (b12 * b12 * 4.0) + alpha / b12);

        qreal A = 1 / (b11 + 2.0 * b12 * B);
        return A;
    }

    inline qreal BC50_A(const qreal logK11, const qreal logK12)
    {
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_A_X;
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
*/
    inline QPair<qreal, qreal> ABPair(qreal x, const QVector<qreal>& parameter)
    {
        qreal b11 = parameter[0];
        qreal b12 = parameter[1];

        qreal alpha = x/(1.0-x);
        qreal B = -b11 / (2.0 * b12) + sqrt(b11 * b11 / (b12 * b12 * 4.0) + alpha / b12);
        qreal A = 1/(b11+2*b12*B);
        return QPair<qreal, qreal>(A,B);
    }

    inline qreal BFunction(qreal x, const QVector<qreal>& parameter)
    {
        return ABPair(x, parameter).second;
    }

    inline qreal AFunction(qreal x, const QVector<qreal>& parameter)
    {
        return ABPair(x, parameter).first;
    }

    inline qreal ABFunction(qreal x, const QVector<qreal>& parameter)
    {
        QPair<qreal, qreal> pair = ABPair(x, parameter);

        qreal b11 = parameter[0];

        qreal A = pair.first;
        qreal B = pair.second;

        return A*B*b11;
    }

    inline qreal AB2Function(qreal x, const QVector<qreal>& parameter)
    {
        qreal b12 = parameter[1];

        QPair<qreal, qreal> pair = ABPair(x, parameter);

        qreal A = pair.first;
        qreal B = pair.second;

        return A*B*B*b12;
    }

    inline qreal A0Function(qreal x, const QVector<qreal>& parameter)
    {
        qreal b12 = parameter[1];
        qreal b11 = parameter[0];

        QPair<qreal, qreal> pair = ABPair(x, parameter);

        qreal A = pair.first;
        qreal B = pair.second;

        return A + A*B*b11 + A*B*B*b12;
    }

    inline qreal B0Function(qreal x, const QVector<qreal>& parameter)
    {
        qreal b12 = parameter[1];
        qreal b11 = parameter[0];

        QPair<qreal, qreal> pair = ABPair(x, parameter);

        qreal A = pair.first;
        qreal B = pair.second;

        return B + A*B*b11 + 2*A*B*B*b12;
    }

    inline QString Format_BC50(const qreal logK11, const qreal logK12)
    {
        QString result = QString();
        /* this will be an interesting part ... */

        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b11 << b12;

        qreal A = 0, B = 0, AB = 0, AB2 = 0, A0 = 0, B0 = 0;

        qreal upper = 1;
        qreal prec = 1e-5;
        std::function<qreal(qreal, const QVector<qreal>&)> function = AFunction;
        A = SimpsonIntegrate(0, upper, function, parameter, prec);
        function = BFunction;
        B = SimpsonIntegrate(0, upper, function, parameter, prec);
        function = ABFunction;

        AB = SimpsonIntegrate(0, upper, function, parameter, prec);
        function = AB2Function;

        AB2 =SimpsonIntegrate(0, upper, function, parameter, prec);

#ifdef conservative
        function = A0Function;
        A0 = SimpsonIntegrate(0, upper, function, parameter, prec);

        function = B0Function;
        B0 = SimpsonIntegrate(0, upper, function, parameter, prec);
#else
        A0 = A + AB + AB2;
        B0 = B + AB +2*AB2;
#endif
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
/*
        std::cout << a_1 << " " << b_1 << " " << b11 * a_1 * b_1 << " " << b21 * a_1 * a_1 * b_1 << " " << b12 * a_1 * b_1 * b_1 << std::endl;
        std::cout << A << " " << B << " " << b11 * A * B << " " << b21 * A * A * B << " " << b12 * A * B * B << std::endl;
        std::cout << "last Change: " << qAbs(b21 * a_1 * a_1 * b_1 - b21 * A * A * B) << " " << qAbs(b12 * a_1 * b_1 * b_1 - b12 * A * B * B) << " " << qAbs(b11 * a_1 * b_1 * b11 * A * B) << std::endl;
        std::cout << "Guess A: " << x / 2 << " .. Final A: " << A << " .. Iterations:" << i << std::endl;
*/
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


    inline qreal BC50_A_X(qreal x, const QVector<qreal>& parameter)
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
/*
        std::cout << a_1 << " " << b_1 << " " << b11 * a_1 * b_1 << " " << b21 * a_1 * a_1 * b_1 << " " << b12 * a_1 * b_1 * b_1 << std::endl;
        std::cout << A << " " << B << " " << b11 * A * B << " " << b21 * A * A * B << " " << b12 * A * B * B << std::endl;
        std::cout << "last Change: " << qAbs(b21 * a_1 * a_1 * b_1 - b21 * A * A * B) << " " << qAbs(b12 * a_1 * b_1 * b_1 - b12 * A * B * B) << " " << qAbs(b11 * a_1 * b_1 * b11 * A * B) << std::endl;
        std::cout << "Guess A: " << x / 2 << " .. Final A: " << A << " .. Iterations:" << i << std::endl;
*/
#endif
        return A;
    }

    inline QPair<qreal, qreal> ABConcentration(qreal x, const QVector<qreal>& parameter)
    {
        if (3 != parameter.size())
            return QPair<qreal, qreal>(0,0);
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
        for (i = 0; i < 350; ++i) {
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
/*
        std::cout << a_1 << " " << b_1 << " " << b11 * a_1 * b_1 << " " << b21 * a_1 * a_1 * b_1 << " " << b12 * a_1 * b_1 * b_1 << std::endl;
        std::cout << A << " " << B << " " << b11 * A * B << " " << b21 * A * A * B << " " << b12 * A * B * B << std::endl;
        std::cout << "last Change: " << qAbs(b21 * a_1 * a_1 * b_1 - b21 * A * A * B) << " " << qAbs(b12 * a_1 * b_1 * b_1 - b12 * A * B * B) << " " << qAbs(b11 * a_1 * b_1 * b11 * A * B) << std::endl;
        std::cout << "Guess A: " << x / 2 << " .. Final A: " << A << " .. Iterations:" << i << std::endl;
*/
#endif
        return QPair<qreal, qreal>(A, B);
    }



    inline qreal BC50_A(const qreal logK21, const qreal logK11, const qreal logK12)
    {
        qreal b21 = qPow(10, logK21 + logK11);
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b21 << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_A_X;
        qreal integ = SimpsonIntegrate(0, 1, function, parameter);
        return integ;
    }

    inline qreal BC50_B0_X(qreal x, const QVector<qreal>& parameter)
    {
        if (3 != parameter.size())
            return 0;
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];
        qreal b12 = parameter[2];

        qreal epsilon = 1e-14;

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
/*
        std::cout << a_1 << " " << b_1 << " " << b11 * a_1 * b_1 << " " << b21 * a_1 * a_1 * b_1 << " " << b12 * a_1 * b_1 * b_1 << std::endl;
        std::cout << A << " " << B << " " << b11 * A * B << " " << b21 * A * A * B << " " << b12 * A * B * B << std::endl;
        std::cout << "last Change: " << qAbs(b21 * a_1 * a_1 * b_1 - b21 * A * A * B) << " " << qAbs(b12 * a_1 * b_1 * b_1 - b12 * A * B * B) << " " << qAbs(b11 * a_1 * b_1 * b11 * A * B) << std::endl;
        std::cout << "Guess A: " << x / 2 << " .. Final A: " << A << " .. Iterations:" << i << std::endl;
*/
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

    inline qreal BFunction(qreal x, const QVector<qreal>& parameter)
    {
        QPair<qreal, qreal> pair = ABConcentration(x, parameter);
        return pair.second;
    }

    inline qreal AFunction(qreal x, const QVector<qreal>& parameter)
    {
        QPair<qreal, qreal> pair = ABConcentration(x, parameter);
        return pair.first;
    }


    inline qreal A2BFunction(qreal x, const QVector<qreal>& parameter)
    {
        qreal b21 = parameter[0];
        QPair<qreal, qreal> pair = ABConcentration(x, parameter);
        qreal A = pair.first;
        qreal B = pair.second;

        return A*A*B*b21;
    }

    inline qreal ABFunction(qreal x, const QVector<qreal>& parameter)
    {
        qreal b11 = parameter[1];
        QPair<qreal, qreal> pair = ABConcentration(x, parameter);
        qreal A = pair.first;
        qreal B = pair.second;

        return A*B*b11;
    }


    inline qreal AB2Function(qreal x, const QVector<qreal>& parameter)
    {
        qreal b12 = parameter[2];
        QPair<qreal, qreal> pair = ABConcentration(x, parameter);
        qreal A = pair.first;
        qreal B = pair.second;

        return A*B*B*b12;
    }

    inline qreal A0Function(qreal x, const QVector<qreal>& parameter)
    {
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];
        qreal b12 = parameter[2];
        QPair<qreal, qreal> pair = ABConcentration(x, parameter);
        qreal A = pair.first;
        qreal B = pair.second;

        return A + 2*A*A*B*b21 +  A*B*b11 + A*B*B*b12;
    }

    inline qreal B0Function(qreal x, const QVector<qreal>& parameter)
    {
        qreal b21 = parameter[0];
        qreal b11 = parameter[1];
        qreal b12 = parameter[2];
        QPair<qreal, qreal> pair = ABConcentration(x, parameter);
        qreal A = pair.first;
        qreal B = pair.second;

        return B + A*A*B*b21 +  A*B*b11 + 2*A*B*B*b12;
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

        qreal A = 0, B = 0, AB = 0, A2B = 0, AB2 = 0, A0 = 0, B0 = 0;

        qreal upper = 1;
        qreal delta = 1e-5;

        qreal integ = 0;
        int increments = (upper - 0) / delta + 1;
#ifndef conservative
        omp_set_num_threads(qApp->instance()->property("threads").toInt());
    #pragma omp parallel for reduction(+ \
                                       : A, B, AB, A2B, AB2, A0, B0)
        for (int i = 0; i < increments - 1; ++i)
        {
            double x = 0 + i / double(increments);
            qreal b = x + delta;
            const qreal quot = (b - x) / 6.0 ;
            QPair<qreal, qreal> x0 = ABConcentration(x, parameter);
            QPair<qreal, qreal> xy = ABConcentration((x + b)/2, parameter);
            QPair<qreal, qreal> y0 = ABConcentration(b, parameter);
            qreal c_a = (x0.first + 4 * xy.first + y0.first);
            qreal c_b = (x0.second + 4 * xy.second + y0.second);

            A += quot * c_a;
            B +=  quot * c_b;
            AB += quot/6.0 * c_a*c_b*b11;
            A2B += quot *quot/6.0 * c_a * c_a*c_b*b21/delta;
            AB2 += quot * quot/6.0*c_a*c_b*c_b*b12/delta;

            A0 += quot * (c_a + c_a*c_b*b11/6.0 + 2*c_a * quot/6.0*c_a*c_b*b21/delta +   quot/6.0/delta*c_a*c_b*c_b*b12);
            B0 += quot * (c_b + c_a*c_b*b11/6.0 +   c_a * quot/6.0*c_a*c_b*b21/delta + 2*quot/6.0/delta*c_a*c_b*c_b*b12);

        }

#else
        /* this block contains the slower integration of each single species - this results are nearly identical to the above */
        std::function<qreal(qreal, const QVector<qreal>&)> function = AFunction;
        A = SimpsonIntegrate(0, upper, function, parameter, delta);

        function = BFunction;
        B = SimpsonIntegrate(0, upper, function, parameter, delta);

        function = A2BFunction;
        A2B = SimpsonIntegrate(0, upper, function, parameter, delta);

        function = ABFunction;
        AB = SimpsonIntegrate(0, upper, function, parameter, delta);

        function = AB2Function;
        AB2 =SimpsonIntegrate(0, upper, function, parameter, delta);

        function = A0Function;
        A0 =SimpsonIntegrate(0, upper, function, parameter, delta);

        function = B0Function;
        B0 =SimpsonIntegrate(0, upper, function, parameter, delta);
#endif

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
