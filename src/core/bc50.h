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
        QChar mu = QChar(956);

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

        /*
        auto ABfunctionA = [](qreal A, qreal B, const QVector<qreal> &parameter)->qreal {
            qreal b11 = parameter[1];
            return b11 * A * B;
        };

        auto A2BfunctionA = [](qreal A, qreal B, const QVector<qreal> &parameter)->qreal {
            qreal b21 = parameter[0];
            return b21 * A * A * B;
        };*/

        qreal B = 0, AB = 0, A2B = 0, bc50_num = 0;

        QVector<qreal> parameter;
        parameter << b21 << b11;
        // double lower = 0, upper = 0.9998;
        /*
        qreal delta = 1E-5;
        int increments = (upper - lower) / delta;

        for (int i = 0; i < increments - 1; ++i) {
            double x = lower + i / double(increments);
            qreal b = x + delta;
            qreal B_a = BFunction(x, bc50, parameter), B_ab =  BFunction((x +b)/2.0, bc50, parameter), B_b = BFunction(b, bc50, parameter);

            qreal B_ = (b - x) / 6 * (B_a + 4 *B_ab + B_b);
            qreal AB_ = (b - x) / 6 * (ABfunction(bc50, B_a, parameter) + 4 * ABfunction(bc50, B_ab, parameter) + ABfunction(bc50, B_b, parameter));
            qreal A2B_ = (b - x) / 6 * (A2Bfunction(bc50, B_a, parameter) + 4 * A2Bfunction(bc50, B_ab, parameter) + A2Bfunction(bc50, B_b, parameter));
            bc50_num += 1/(bc50 + AB_ + 2 * A2B);

            std::cout << x << " .. " << 1/2.0/bc50_num << ": " << B_ << " " << AB_ << " " << A2B_  << " " << qAbs(1/2.0/bc50_num - bc50) << std::endl;

            if(qAbs(1/2.0/bc50_num - bc50) < 1e-9)
                break;

            B += B_;
            AB +=  AB_;
            A2B += A2B_;
        }

        lower = upper;
        upper = 1;

        delta = 1E-11;
        increments = (upper - lower) / delta;

        for (int i = 0; i < increments - 1; ++i) {
            double x = lower + i / double(increments);
            if(x > upper)
                break;
            qreal b = x + delta;
            qreal B_a = BFunction(x, bc50, parameter), B_ab =  BFunction((x +b)/2.0, bc50, parameter), B_b = BFunction(b, bc50, parameter);

            qreal B_ = (b - x) / 6 * (B_a + 4 *B_ab + B_b);
            qreal AB_ = (b - x) / 6 * (ABfunction(bc50, B_a, parameter) + 4 * ABfunction(bc50, B_ab, parameter) + ABfunction(bc50, B_b, parameter));
            qreal A2B_ = (b - x) / 6 * (A2Bfunction(bc50, B_a, parameter) + 4 * A2Bfunction(bc50, B_ab, parameter) + A2Bfunction(bc50, B_b, parameter));
            bc50_num += 1/(bc50 + AB_ + 2 * A2B);

            std::cout << x << " ... " << 1/2.0/bc50_num << ": " << B_ << " " << AB_ << " " << A2B_  << " " << qAbs(1/2.0/bc50_num - bc50) << std::endl;

            if(qAbs(1/2.0/bc50_num - bc50) < 1e-9 || std::isinf(AB_))
                break;

            B += B_;
            AB +=  AB_;
            A2B += A2B_;
        }
        */

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
        result += QString("<p>BC(B0)<sub>0</sub> = %1</p>").arg(Print::printConcentration(B + AB + A2B, 3));

        result += QString("<p>BC(B)<sub>0</sub> = %1</p>").arg(Print::printConcentration(B, 3));
        result += QString("<p>BC(AB)<sub>0</sub> = %1</p>").arg(Print::printConcentration(AB));
        result += QString("<p>BC(A2B)<sub>0</sub> = %1</p>").arg(Print::printConcentration(A2B));
        result += QString("<p>CR(B)<sub>0</sub> = %1 </p>").arg(B / (B + AB + A2B));
        result += QString("<p>CR(AB)<sub>0</sub> = %1 </p>").arg(AB / (B + AB + A2B));
        result += QString("<p>CR(A2B)<sub>0</sub> = %1 </p>").arg(A2B / (B + AB + A2B));

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
        qreal B = -b11 / (2.0 * b12) + sqrt(b11 * b11 / (b12 * b12 * 4.0) + alpha / b12);

        qreal A = 1 / (b11 + 2.0 * b12 * B);

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

    inline qreal BC50_Y_B(qreal x, const QVector<qreal>& parameter)
    {
        if (2 != parameter.size())
            return 0;
        qreal alpha = x / (1 - x);
        qreal b11 = parameter[0];
        qreal b12 = parameter[1];

        qreal B = -b11 / (2.0 * b12) + sqrt(b11 * b11 / (b12 * b12 * 4.0) + alpha / b12);
        qreal A = 1 / (b11 + 2.0 * b12 * B);

        // std::cout << B << " " << b11 * A * B + 2 * b12 * A * B * B << " " << B/(b11 * A * B + 2 * b12 * A * B * B) << std::endl;

        return B + b11 * A * B + 2 * b12 * A * B * B;
    }

    inline qreal BC50_B0(const qreal logK11, const qreal logK12)
    {
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b11 << b12;
        std::function<qreal(qreal, const QVector<qreal>&)> function = BC50_Y_B;
        qreal integ = SimpsonIntegrate(0, 1, function, parameter);
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

    inline qreal BFunction(qreal x, const QVector<qreal>& parameter)
    {
        qreal b11 = parameter[0];
        qreal b12 = parameter[1];
        qreal alpha = x / (1.0 - x);
        return -b11 / (2.0 * b12) + sqrt(b11 * b11 / (b12 * b12 * 4.0) + alpha / b12);
    }

    inline qreal AFunction(qreal x, const QVector<qreal>& parameter)
    {
        qreal b11 = parameter[0];
        qreal b12 = parameter[1];
        qreal alpha = x / (1.0 - x);
        qreal B = -b11 / (2.0 * b12) + sqrt(b11 * b11 / (b12 * b12 * 4.0) + alpha / b12);
        return 1 / (b11 + 2.0 * b12 * B);
    }

    inline qreal ABFunction(qreal x, const QVector<qreal>& parameter)
    {
        qreal B = BFunction(x, parameter);
        qreal b11 = parameter[0];
        qreal b12 = parameter[1];
        qreal A = AFunction(x, parameter);
        return b11 * A * B;
    }

    inline qreal AB2Function(qreal x, const QVector<qreal>& parameter)
    {
        qreal B = BFunction(x, parameter);
        qreal b11 = parameter[0];
        qreal b12 = parameter[1];
        qreal A = AFunction(x, parameter);
        return b12 * A * B * B;
    }

    inline QPair<qreal, qreal> c12(qreal x, const QVector<qreal>& parameter)
    {
        if (2 != parameter.size())
            return QPair<qreal, qreal>(0, 0);
        qreal alpha = x / (1 - x);

        qreal b11 = parameter[0];
        qreal b12 = parameter[1];
        qreal B = BFunction(x, parameter);
        qreal A = 1 / (b11 + 2.0 * b12 * B);
        return QPair<qreal, qreal>(A, B);
    }

    inline QString Format_BC50(const qreal logK11, const qreal logK12)
    {
        QString result = QString();
        QChar mu = QChar(956);

        /* this will be an interesting part ... */

        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b11 << b12;

        qreal bc50 = 0, bc50_sf = 0;
        qreal lower = 0, upper = 1;

        qreal integ = 0, A = 0;
        qreal delta = 1E-5;
        int increments = (upper - lower) / delta;
        qreal B = 0, AB = 0, AB2 = 0, bc50_num = 0;

        qreal B0 = BC50_B0(logK11, logK12);
        B = B0 / 2.0;
        A = 1 / (b11 + 2.0 * b12 * B);
        qreal A0 = A + A * B * b11 + A * B * B * b12;
        qDebug() << A0 << B0;
        // A = ItoI_ItoII::HostConcentration(A0, B0, QList<qreal>() << b11 << b12/b11);
        // B =ItoI_ItoII::GuestConcentration(A0, B0, QList<qreal>() << b11 << b12/b11);
        AB = A * B * b11;
        AB2 = A * B * B * b12;
        qDebug() << AB / A / B << AB2 / AB / B;
        // A0 = A + AB + AB2;
        // B0 = B + AB + 2*AB2;
        qDebug() << B / B0;
        qDebug() << ItoI_ItoII::HostConcentration(A0, B0, QList<qreal>() << b11 << b12 / b11) << A << ItoI_ItoII::GuestConcentration(A0, B0, QList<qreal>() << b11 << b12 / b11) << B;
        //{
        /*
            auto bc50Function = [](const QPair<qreal, qreal>& pair, const QVector<qreal>& parameter) -> qreal {
                const qreal A = pair.first;
                const qreal B = pair.second;
                qreal b11 = parameter[0];
                qreal b12 = parameter[1];
                return 1.0 / (A + b11 * A * B + b12 * A * B * B);
            };

            auto ABFunction = [](const QPair<qreal, qreal>& pair, const QVector<qreal>& parameter) -> qreal {
                const qreal A = pair.first;
                const qreal B = pair.second;
                qreal b11 = parameter[0];
                return b11 * A * B;
            };

            auto AB2Function = [](const QPair<qreal, qreal>& pair, const QVector<qreal>& parameter) -> qreal {
                const qreal A = pair.first;
                const qreal B = pair.second;
                qreal b12 = parameter[1];
                return b12 * A * B * B;
            };


//#pragma omp parallel for reduction(+ \
//                                   : integ, AB, AB2, B, A)
        for (int i = 0; i < increments - 1; ++i) {
            double x = lower + i / double(increments);
            qreal b = x + delta;
            QPair<qreal, qreal> c = c12(x, parameter), cb = c12((x + b) / 2, parameter), d = c12(b, parameter);

            integ += (b - x) / 6 * (bc50Function(c, parameter) + 4 * bc50Function(cb, parameter) + bc50Function(d, parameter));
            qreal A_ = (b - x) / 6 * (c.first + 4 * cb.first + d.first);
            qreal B_ = (b - x) / 6 * (c.second + 4 * cb.second + d.second);
            qreal AB_ = b11 * A * B;
            qreal AB2_ = b12 * A * B * B;
            //qDebug() << (b - x) / 6 * (ABFunction(c, parameter) + 4 * ABFunction(cb, parameter) + ABFunction(d, parameter)) << (b - x) / 6 * (AB2Function(c, parameter) + 4 * AB2Function(cb, parameter) + AB2Function(d, parameter));
         /*   AB += 1/((b - x) / 6 * (ABFunction(c, parameter) + 4 * ABFunction(cb, parameter) + ABFunction(d, parameter)));
            AB2 += 1/((b - x) / 6 * (AB2Function(c, parameter) + 4 * AB2Function(cb, parameter) + AB2Function(d, parameter)));
            B += 1/((b - x) / 6 * (c.second + 4 * cb.second + d.second));

            A += 1/((b - x) / 6 * (c.first + 4 * cb.first + d.first));
            */
        //qDebug() << A_ << B_ << AB_ << AB2_;
        /*
            A    += A;
            B    += B;
            AB   += AB_;
            AB2  += AB2_;*(
          //  integ += 1.0/(A_ + B_ + AB_ + 2*AB2_);
        }   */
        //}
        /*
        bc50 = 1.0 / 2.0 / integ;

        //result += QString("<p>BC50<sub>0</sub> =  %1 %2M ... ").arg(BC50(logK11, logK12) * 1e6).arg(mu);
        //result += QString("BC50<sub>0</sub>(SF) = %1 %2M</p>").arg(ItoI_ItoII_BC50_SF(logK11, logK12) * 1e6).arg(mu);

        //delta = 1e-6;
        //upper = 1;
        /*
        std::vector< std::function<qreal(qreal, const QVector<qreal>&)> *> functions;
        std::function<qreal(qreal, const QVector<qreal>&)> functionB = BFunction;

        functions.push_back( &Bfunction );
        std::function<qreal(qreal, const QVector<qreal>&)> functionAB = ABFunction;

        functions.push_back( &ABFunction );

        std::function<qreal(qreal, const QVector<qreal>&)> functionAB2 = AB2Function;

        functions.push_back( &AB2Function );

        std::vector<double> integs = SimpsonIntegrate(0.0, upper, functions, parameter, delta);
        B = integs[0];
        AB = integs[1];
        AB2 = integs[2];*/
        /*
        qDebug() << A << B << AB << AB2;

        //qDebug() << A << B << 1.0 / 2.0 /AB << 1.0 / 2.0 /AB2;

        std::function<qreal(qreal, const QVector<qreal>&)> function = BFunction;
        B = SimpsonIntegrate(0, upper, function, parameter, delta);

        function = ABFunction;
        AB = SimpsonIntegrate(0, upper, function, parameter, delta);

        function = AB2Function;
        AB2 = SimpsonIntegrate(0, upper, function, parameter, delta);

        function = AFunction;
        A = SimpsonIntegrate(0, upper, function, parameter, delta);
        qDebug() << A << B << AB << AB2;

        qDebug() << A + AB + AB2 << B + AB + 2 * AB2;

        qDebug() << AB2 / AB / B << AB / A / B;
        */
        //std::cout << BFunction(1, parameter) << " " << ABFunction(1, parameter) << " " << AB2Function(1, parameter) <<std::endl;

        //function = BCfunction;

        //result += QString("<p>BC50<sub>0</sub> = %1</p>").arg(Print::printConcentration(1/2.0/SimpsonIntegrate(0, upper, function, parameter),3));

        result += QString("<p>BC50<sub>0</sub> =  %1 </p> ").arg(Print::printConcentration(bc50, 3));
        result += QString("<p>BC(A0)<sub>0</sub> = %1 </p>").arg(Print::printConcentration(A + AB + AB2, 3));
        result += QString("<p>BC(B0)<sub>0</sub> = %1</p>").arg(Print::printConcentration(B + AB + 2 * AB2, 3));
        result += QString("<p>BC(A)<sub>0</sub> = %1 </p>").arg(Print::printConcentration(A, 3));
        result += QString("<p>BC(B)<sub>0</sub> = %1</p>").arg(Print::printConcentration(B, 3));
        result += QString("<p>BC(AB)<sub>0</sub> = %1</p>").arg(Print::printConcentration(AB, 3));
        result += QString("<p>BC(AB2)<sub>0</sub> = %1</p>").arg(Print::printConcentration(AB2, 3));

        result += QString("<p>CR(B)<sub>0</sub> = %1 </p>").arg(B / B0);
        result += QString("<p>CR(AB)<sub>0</sub> = %1 </p>").arg(AB / B0);
        result += QString("<p>CR(AB2)<sub>0</sub> = %1 </p>").arg(2 * AB2 / B0);

        /*
        result += QString("<p>CR(AB,A)<sub>0</sub> = %1 </p>").arg(AB/A);
        result += QString("<p>CR(AB2,A)<sub>0</sub> = %1 </p>").arg(AB2/A);
        */
        return result;
    }
}

namespace IItoII {

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
        //  std::cout << B << " " << b11 * A * B + 2 * b12 * A * B * B + b21 * A * A * B << " " << B/(b11 * A * B + 2 * b12 * A * B * B + b21 * A * A * B) << std::endl;
        return B + b11 * A * B + 2 * b12 * A * B * B + b21 * A * A * B;
    }

    inline qreal BC50(const qreal logK21, const qreal logK11, const qreal logK12)
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

    inline qreal AFunction(qreal x, const QVector<qreal>& parameter)
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

    inline qreal BFunction(qreal x, const QVector<qreal>& parameter)
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
        return B;
    }

    inline qreal ABFunction(qreal x, const QVector<qreal>& parameter)
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
        return b11 * A * B;
    }

    inline qreal A2BFunction(qreal x, const QVector<qreal>& parameter)
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
        return b21 * A * A * B;
    }

    inline qreal AB2Function(qreal x, const QVector<qreal>& parameter)
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
        return b12 * A * B * B;
    }

    inline QPair<qreal, qreal> concentrations_211112(qreal x, const QVector<qreal>& parameter)
    {
        if (3 != parameter.size())
            return QPair<qreal, qreal>(0, 0);
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
        //ifdef _DEBUG
        qreal AB = b11 * A * B;
        qreal AB2 = b12 * A * B * B;
        qreal A2B = b21 * A * A * B;
        //std::cout << a_1 << " " << b_1 << " " << b11 * a_1 * b_1 << " " << b21 * a_1 * a_1 * b_1 << " " << b12 * a_1 * b_1 * b_1 << std::endl;
        //std::cout << A << " " << B << " " << b11 * A * B << " " << b21 * A * A * B << " " << b12 * A * B * B << std::endl;
        //std::cout << "last Change: " << qAbs(b21 * a_1 * a_1 * b_1 - b21 * A * A * B) << " " << qAbs(b12 * a_1 * b_1 * b_1 - b12 * A * B * B) << " " << qAbs(b11 * a_1 * b_1 - b11 * A * B) << std::endl;
        //std::cout << "A0=" << A + AB + AB2 + 2*A2B << " ....... B0=" << A + AB + 2*AB2 + A2B << std::endl;

        //        std::cout << AB2/AB/B << " " << " " << A2B/AB/A << " " << AB/B/A << std::endl;
        //std::cout << "Guess A: " << x / 2 << " .. Final A: " << A << " .. Iterations:" << i << std::endl;
        //std::cout << std::endl;
        //endif
        return QPair<qreal, qreal>(A, B);
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

    inline QString Format_BC50(const qreal logK21, const qreal logK11, const qreal logK12)
    {
        QString result = QString();
        QChar mu = QChar(956);

        /* this will be an interesting part ... */

        qreal b21 = qPow(10, logK21 + logK11);
        qreal b11 = qPow(10, logK11);
        qreal b12 = qPow(10, logK11 + logK12);

        QVector<qreal> parameter;
        parameter << b21 << b11 << b12;

        qreal bc50 = 0;

        qreal lower = 0, upper = 0.9999;

        qreal integ = 0, A = 0, B = 0, AB = 0, A2B = 0, AB2 = 0;
        qreal delta = 1E-5;
        double B0 = BC50_B0(logK21, logK11, logK12);
        B = B0 / 2.0;
        qreal q = (b11 + 2 * b12 * B) / (2 * b21);
        A = -q + sqrt(q * q + 1 / b21);
        double A0 = A + A * B * b11 + 2 * b21 * A * A * B + b12 * A * B * B;

        /*
        //std::cout << A0 << " " << B0 << std::endl;
        qreal ok;
        {

            qreal a0 = A0;
            qreal b0 = B0;
            auto calc_a = [](double a0, long double b, double b11, double b21, double b12) {
                long double x1 = 2 * b21 * b;
                long double x2 = b12 * b * b + b11 * b + 1;
                long double x3 = -a0;
                long double a = MaxQuadraticRoot(x1, x2, x3);
                if (a < a0)
                    return a;
                else {
        #ifdef _DEBUG
                    std::cout << "a: " << a << " a0 " << a0 << std::endl;
        #endif
                    return MinQuadraticRoot(x1, x2, x3);
                }
            };

            auto calc_b = [](double b0, long double a, double b11, double b21, double b12) {
                long double x1 = 2 * b12 * a;
                long double x2 = b21 * a * a + b11 * a + 1;
                long double x3 = -b0;
                long double b = MaxQuadraticRoot(x1, x2, x3);
                if (b < b0)
                    return b;
                else {
        #ifdef _DEBUG
                    std::cout << "b: " << b << " b0: " << b0 << std::endl;
        #endif
                    return MinQuadraticRoot(x1, x2, x3);
                }
            };
            long double epsilon = 1e-14;
            long double a = qMin(a0, b0) / b11 * 10;
            long double b = 0;
            long double a_1 = 0, b_1 = 0;
            int i = 0;
            for (i = 0; i < 1000; ++i) {
                a_1 = a;
                b_1 = b;
                b = calc_b(b0, a, b11, b21, b12);
                if (b < 0)
                    b *= -1;

                a = calc_a(a0, b, b11, b21, b12);
                if (a < 0)
                    a *= -1;
                if (qAbs(b21 * a_1 * a_1 * b_1 - b21 * a * a * b) < epsilon && qAbs(b12 * a_1 * b_1 * b_1 - b12 * a * b * b) < epsilon && qAbs(b11 * a_1 * b_1 - b11 * a * b) < epsilon)
                    break;
            }
        #ifdef _DEBUG
            std::cout << a_1 << " " << b_1 << " " << K11 * a_1 * b_1 << " " << b21 * a_1 * a_1 * b_1 << " " << b12 * a_1 * b_1 * b_1 << std::endl;
            std::cout << a << " " << b << " " << K11 * a * b << " " << b21 * a * a * b << " " << b12 * a * b * b << std::endl;
            std::cout << "Guess A: " << qMin(a0, b0) / K11 * 100 << " .. Final A: " << a << " .. Iterations:" << i << std::endl;
        #endif
            ok = (a < A0) && (b < B0) && (a > 0) && (b > 0) && i < 1000;

            A = a;
            B = b;

        }*/
        //qDebug() << ok;
        /*
        IItoI_ItoI_ItoII_Solver *solver = new IItoI_ItoI_ItoII_Solver;
        solver->setConstants(QList<double> () << logK21 << logK11 << logK12);
        solver->setInput(A0, B0);
        solver->run();
        QPair<double, double> pair = solver->Concentrations();
        delete solver;
        A = pair.first;
        B = pair.second;*/
        qDebug() << b11 << A << B;
        AB = b11 * A * B;
        qDebug() << AB << AB / A / B;
        AB2 = b12 * A * B * B;

        A2B = b21 * A * A * B;

        //  qDebug() << A << B << A2B << AB << AB2;
        int increments = (upper - lower) / delta;
        //#pragma omp parallel for reduction(+ \
//                                   : integ, AB, AB2, B, A, A2B)
        /*
        for (int i = 0; i < increments - 1; ++i) {
            double x = lower + i / double(increments);
            qreal b = x + delta;
            QPair<qreal, qreal> c = concentrations_211112(x, parameter), cb = concentrations_211112((x + b) / 2.0, parameter), d = concentrations_211112(b, parameter);

            //   integ += (b - x) / 6 * (bc50Function(c, parameter) + 4 * bc50Function(cb, parameter) + bc50Function(d, parameter));
            qreal A_ = (b - x) / 6.0 * (c.first + 4 * cb.first + d.first);
            qreal B_ = (b - x) / 6.0 * (c.second + 4 * cb.second + d.second);
            // std::cout << c.first << " " << c.second << " " << A_ << std::endl;

            A += A_;
            B += B_;

            qreal AB_ = b11 * A_ * B_;
            qreal AB2_ = b12 * A_ * B_ * B_;
            qreal A2B_ = b21 * A_ * A_ * B_;
            /*
            qreal AB_ = b11*A_*B_;
            qreal A2B_ = b21*A_*A_*B_;
            qreal AB2_ = b12*A_*B_*B_;*/
        /*
            A2B += (b - x) / 6 * (A2BFunction(c, parameter) + 4 * A2BFunction(cb, parameter) + A2BFunction(d, parameter));
            AB += (b - x) / 6 * (ABFunction(c, parameter) + 4 * ABFunction(cb, parameter) + ABFunction(d, parameter));
            AB2 += (b - x) / 6 * (AB2Function(c, parameter) + 4 * AB2Function(cb, parameter) + AB2Function(d, parameter));
            */

        //   AB += AB_;
        //   A2B += A2B_;
        //  AB2 += AB2_;
        // integ += 1 / (A_ + AB_ + AB2_ + 2 * A2B_);
        //std::cout << AB2/AB/B << " " << " " << A2B/AB/A << " " << AB/B/A << std::endl;
        // std::cout << "A0(" << x << ")=" << A_ + AB_ + AB2_ + 2*A2B_ << " ... B0(" << x << ")=" << A_ + AB_ + 2*AB2_ + A2B_ << " " << AB2_/AB_/B_ << " " << A2B_/AB_/A_ << " " << AB_/B_/A_  << std::endl;

        //std::cout << log10(AB2/AB/B) << " " << logK12 << " " << log10(A2B/AB/A) << " " << logK21 << " " << log10(AB/B/A) << " " << logK11 << std::endl;
        //}
        //bc50 = 1.0 / 2.0 / integ;
        /*
        std::function<qreal(qreal, const QVector<qreal>&)> function = BFunction;
        B = SimpsonIntegrate(0, upper, function, parameter, delta);

        function = ABFunction;
        AB = SimpsonIntegrate(0, upper, function, parameter, delta);

        function = AB2Function;
        AB2 = SimpsonIntegrate(0, upper, function, parameter, delta);

        function = AFunction;
        A = SimpsonIntegrate(0, upper, function, parameter, delta);

        function = A2BFunction;
        A2B = SimpsonIntegrate(0, upper, function, parameter, delta);
        qDebug() << A << B << A2B <<AB << AB2;*/

        //    qDebug() << AB/A/B << AB2/AB/B << A2B/A/AB;
        //    qDebug() << A + AB + AB2+ 2*A2B;
        //    qDebug() << B + AB + 2*AB2 + A2B;
        result += QString("<p>BC50<sub>0</sub> =  %1 </p> ").arg(Print::printConcentration(bc50, 3));
        result += QString("<p>BC50<sub>0</sub> =  %1 </p> ").arg(Print::printConcentration(BC50(logK21, logK11, logK12), 3));

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

        /*
        result += QString("<p>CR(A2B,A)<sub>0</sub> = %1 </p>").arg(2*A2B/A);
        result += QString("<p>CR(AB,A)<sub>0</sub> = %1 </p>").arg(AB/A);
        result += QString("<p>CR(AB2,A)<sub>0</sub> = %1 </p>").arg(AB2/A);*/

        return result;
    }
}
}
