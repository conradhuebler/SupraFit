/*
 * < Performe cluster analysis with in pure C++ >
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

typedef std::vector<int> CxxClusterIndices;
typedef std::vector<double> CxxClusterPosition;
typedef std::pair<CxxClusterIndices, CxxClusterPosition> CxxClusterElement;
typedef std::vector<CxxClusterElement> CxxClusterMatrix;

struct CxxClusterMinimalDistance {
    CxxClusterIndices A;
    CxxClusterIndices B;
    double distance = 1e27;
};

class CxxCluster {
public:
    CxxCluster() = default;
    ~CxxCluster() = default;

    inline void PrintMatrix(const CxxClusterMatrix& matrix)
    {
        for (const auto& row : matrix) {
            std::cout << "[ ";
            for (auto i : row.first)
                std::cout << i << " ";
            std::cout << "] ";
            for (int i = 0; i < 10; ++i)
                std::cout << " ";
            std::cout << "{";
            for (auto d : row.second)
                std::cout << d << " ";
            std::cout << "}\n";
        }
    }
    void Run(const CxxClusterMatrix& matrix)
    {
        CxxClusterMatrix temp = matrix;
        m_storage.push_back(temp);
        while (temp.size() > 1) {
            temp = Link(temp, FindShortesDistance(temp));
            m_storage.push_back(temp);
        }
    }
    std::vector<CxxClusterMatrix> Storage() const { return m_storage; }

private:
    CxxClusterMinimalDistance FindShortesDistance(const CxxClusterMatrix& matrix)
    {
        CxxClusterMinimalDistance result;
        for (int i = 0; i < matrix.size(); ++i) {
            for (int j = 0; j < i; ++j) {
                double distance = Euclidiean(matrix[i].second, matrix[j].second);
                if (distance < result.distance) {
                    result.A = matrix[i].first;
                    result.B = matrix[j].first;
                    result.distance = distance;
                }
            }
        }
        return result;
    }

    CxxClusterMatrix Link(const CxxClusterMatrix& matrix, const CxxClusterMinimalDistance& distance)
    {
        CxxClusterMatrix result;
        CxxClusterElement A, B;
        for (int i = 0; i < matrix.size(); ++i) {
            if (matrix[i].first == distance.A)
                A = matrix[i];
            else if (matrix[i].first == distance.B)
                B = matrix[i];
            else
                result.push_back(matrix[i]);
        }

        CxxClusterIndices indicies;
        CxxClusterPosition position(A.second.size(), 0);
        for (int a = 0; a < position.size(); ++a)
            position[a] += (A.second[a] + B.second[a]) * 0.5;

        indicies.insert(indicies.end(), distance.A.begin(), distance.A.end());
        indicies.insert(indicies.end(), distance.B.begin(), distance.B.end());

        CxxClusterElement element(indicies, position);
        result.push_back(element);
        return result;
    }
    double Euclidiean(const CxxClusterPosition& A, const CxxClusterPosition& B)
    {
        double distance = 0.0;
        for (int i = 0; i < A.size(); ++i) {
            distance += (A[i] - B[i]) * (A[i] - B[i]);
        }
        return sqrt(distance);
    }

    std::vector<CxxClusterMatrix> m_storage;
};
