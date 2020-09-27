/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QCollator>
#include <QtCore/QDebug>
#include <QtCore/QDirIterator>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QUuid>
#include <QtCore/QVector>

#include <src/core/models/dataclass.h>

#include <src/core/toolset.h>

#include "spectrahandler.h"

SpectraHandler::SpectraHandler(QObject* parent)
    : QObject(parent)
{
}

void SpectraHandler::addSpectrum(const QString& file)
{
    QPair<Vector, Vector> spec = ToolSet::LoadCSVile(file);
    QList<QPointF> xy;
    for (int i = 0; i < spec.first.size(); ++i)
        xy << (QPointF(spec.first[i], spec.second[i]));
    PeakPick::spectrum spectrum(spec.first, spec.second);
    Spectrum p;
    p.m_spectrum = spectrum;
    QFileInfo info(file);
    p.m_filename = info.fileName();
    p.m_path = info.path();
    p.m_xy = xy;
    QUuid uuid;
    QString id = uuid.createUuid().toString();
    m_spectra.insert(id, p);
    m_order << id;
}

void SpectraHandler::addDirectory(const QString& dir, const QString& suffix)
{
    QDirIterator it(dir, QDirIterator::NoIteratorFlags);
    QStringList files;
    while (it.hasNext()) {
        QString file = it.next();
        if (file.contains(suffix))
            files.append(file);
        //addSpectrum(file);
    }
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(
        files.begin(),
        files.end(),
        [&collator](const QString& file1, const QString& file2) {
            return collator.compare(file1, file2) < 0;
        });

    for (const auto& f : files)
        addSpectrum(f);
}

DataTable* SpectraHandler::CompileSimpleTable()
{
    std::sort(
        m_x.begin(),
        m_x.end());

    int cols = m_x.size();
    int rows = m_order.size();
    DataTable* table = new DataTable(cols, rows, this);
    for (int i = 0; i < m_x.size(); ++i) {
        double x = m_x[i];
        for (int j = 0; j < m_order.size(); ++j) {
            auto spec = m_spectra[m_order[j]];
            double val = 0;
            double diff = 1e10;
            for (const auto& p : spec.m_xy) {
                if (qAbs(x - p.x()) < diff) {
                    diff = qAbs(x - p.x());
                    val = p.y();
                }
            }
            table->data(i, j) = val;
        }
    }
    return table;
}
