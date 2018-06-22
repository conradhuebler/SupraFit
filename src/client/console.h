/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "src/core/models.h"

class DataClass;

/**
 * @todo write docs
 */
class Console : public QObject {
    Q_OBJECT

public:
    Console(int runs, double std);
    ~Console();

    bool FullTest();
    bool LoadFile(const QString& file);

    inline void setReduction(bool val) { m_reduction = val; }
    inline void setCrossValidation(bool val) { m_crossvalidation = val; }
    inline void setMonteCarlo(bool val) { m_montecarlo = val; }
    inline void setModelComparison(bool val) { m_modelcomparison = val; }
    inline void setWeakendGridSearch(bool val) { m_weakendgrid = val; }

private:
    QJsonObject m_toplevel;
    QString m_file;
    int m_runs;
    double m_std;

    bool m_reduction, m_crossvalidation, m_montecarlo, m_modelcomparison, m_weakendgrid;

    inline QSharedPointer<AbstractModel> Test11Model(QPointer<DataClass> data)
    {
        QSharedPointer<AbstractModel> model = QSharedPointer<ItoI_Model>(new ItoI_Model(data), &QObject::deleteLater);
        return model;
    }
    inline QSharedPointer<AbstractModel> Test2111Model(QPointer<DataClass> data)
    {
        QSharedPointer<AbstractModel> model = QSharedPointer<IItoI_ItoI_Model>(new IItoI_ItoI_Model(data), &QObject::deleteLater);
        return model;
    }
    inline QSharedPointer<AbstractModel> Test1112Model(QPointer<DataClass> data)
    {
        QSharedPointer<AbstractModel> model = QSharedPointer<ItoI_ItoII_Model>(new ItoI_ItoII_Model(data), &QObject::deleteLater);
        return model;
    }

    inline QSharedPointer<AbstractModel> Test22Model(QPointer<DataClass> data)
    {
        QSharedPointer<AbstractModel> model = QSharedPointer<IItoI_ItoI_ItoII_Model>(new IItoI_ItoI_ItoII_Model(data), &QObject::deleteLater);
        return model;
    }

    void Test(QSharedPointer<AbstractModel> model);

    QPointer<const DataClass> m_data;

    QJsonObject MonteCarlo(QSharedPointer<AbstractModel> model);
    QJsonObject MoCoAnalyse(QSharedPointer<AbstractModel> model);
    QJsonObject Reduction(QSharedPointer<AbstractModel> model);
    QJsonObject CrossValidation(QSharedPointer<AbstractModel> model);
    QJsonObject GridSearch(QSharedPointer<AbstractModel> model);

    void PrintStatistic(const QJsonObject& object, QSharedPointer<AbstractModel> model);
};
