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

#include <QObject>

class SupraFitCli : public QObject {
    Q_OBJECT
public:
    explicit SupraFitCli();
    ~SupraFitCli();

    inline void setInFile(const QString& file)
    {
        m_infile = file;
        m_outfile = file;
    }
    inline void setOutFile(const QString& file)
    {
        m_outfile = file;
        m_outfile.contains(".suprafit") ? m_extension = ".suprafit" : m_extension = ".json";
        m_outfile.remove(".json").remove(".suprafit");
    }

    inline void setReduction(bool val) { m_reduction = val; }
    inline void setCrossValidation(bool val) { m_crossvalidation = val; }
    inline void setMonteCarlo(bool val) { m_montecarlo = val; }
    inline void setModelComparison(bool val) { m_modelcomparison = val; }
    inline void setWeakendGridSearch(bool val) { m_weakendgrid = val; }

signals:

public slots:

protected:
    bool m_reduction = false, m_crossvalidation = false, m_montecarlo = false, m_modelcomparison = false, m_weakendgrid = false;
    QString m_infile = QString(), m_outfile = QString(), m_extension = ".suprafit";
};
