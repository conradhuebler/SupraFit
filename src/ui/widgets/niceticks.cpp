/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

/*
 * Source taken from here
 * https://stackoverflow.com/questions/8506881/nice-label-algorithm-for-charts-with-minimum-ticks
 *
 * Thanks a lot to stackoverflow
 *
 */

#include "math.h"

#include "niceticks.h"

using namespace std;

/**
* Calculate and update values for tick spacing and nice
* minimum and maximum data points on the axis.
*/
void NiceScale::calculate()
{
    range = niceNum(maxPoint - minPoint, false);
    tickSpacing = niceNum(range / (maxTicks - 1), true);
    niceMin = floor(minPoint / tickSpacing) * tickSpacing;
    niceMax = ceil(maxPoint / tickSpacing) * tickSpacing;
}

/**
* Returns a "nice" number approximately equal to range
  Rounds the number if round = true Takes the ceiling if round = false.
*
* @param range the data range
* @param round whether to round the result
* @return a "nice" number to be used for the data range
*/
float NiceScale::niceNum(float range, bool round)
{   float exponent; /** exponent of range */
    float fraction; /** fractional part of range */
    float niceFraction; /** nice, rounded fraction */

    exponent = floor(log10(range));
    fraction = range / pow(10.f, exponent);

    if (round)
    {   if (fraction < 1.5)
            niceFraction = 1;
        else if (fraction < 3)
            niceFraction = 2;
        else if (fraction < 7)
            niceFraction = 5;
        else
            niceFraction = 10;
    }
    else
    {   if (fraction <= 1)
            niceFraction = 1;
        else if (fraction <= 2)
            niceFraction = 2;
        else if (fraction <= 5)
            niceFraction = 5;
        else
            niceFraction = 10;
    }

    return niceFraction * pow(10, exponent);
}

/**
* Sets the minimum and maximum data points for the axis.
*
* @param minPoint the minimum data point on the axis
* @param maxPoint the maximum data point on the axis
*/
void NiceScale::setMinMaxPoints(float minPoint, float maxPoint)
{
    this->minPoint = minPoint;
    this->maxPoint = maxPoint;
    calculate();
}

/**
* Sets maximum number of tick marks we're comfortable with
*
* @param maxTicks the maximum number of tick marks for the axis
*/
void NiceScale::setMaxTicks(float maxTicks)
{
    this->maxTicks = maxTicks;
    calculate();
}

// minimum number of decimals in tick labels
// use in sprintf statement:
// sprintf(buf, "%.*f", decimals(), tickValue);
int NiceScale::decimals(void)
{
    float logTickX = log10(tickSpacing);
    if(logTickX >= 0)
        return 0;
    return (int)(abs(floor(logTickX)));
}
