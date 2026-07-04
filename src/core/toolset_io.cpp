/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

// Claude Generated: file-I/O split out of toolset.cpp (2026). Holds the ToolSet data-file
// loaders/exporters (ExportResults, FindFile, LoadTableFile, LoadCSVFile, LoadAbsorbFile,
// LoadXYFile, LoadITCFile); the namespace ToolSet and toolset.h facade are unchanged.

#include <boxwhisker.h>

#include "jsonhandler.h"
#include "libmath.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <Eigen/Dense>

#include <functional>

#include "toolset.h"

namespace ToolSet {

void ExportResults(const QString& filename, const QList<QJsonObject>& models)
{
    QJsonObject toplevel;
    int i = 0;
    for (const QJsonObject& obj : qAsConst(models)) {
        QJsonObject constants = obj["data"].toObject()["constants"].toObject();
        QStringList keys = constants.keys();
        bool valid = true;
        for (const QString& str : qAsConst(keys)) {
            double var = constants[str].toString().toDouble();
            valid = valid && (var > 0);
        }
        toplevel["model_" + QString::number(i++)] = obj;
    }
    JsonHandler::WriteJsonFile(toplevel, filename);
}
QString FindFile(const QString& start, const QString& path, const QString& hash, bool recursive)
{
    QString file = start;
    QFileInfo info(start);

    if (!info.exists()) {
        QFileInfo i2(path + QDir::separator() + info.fileName());
        if (i2.exists())
            file = path + QDir::separator() + info.fileName();
        else {
            file.clear();
        }
    }
    return file;
}

DataTable* LoadTableFile(const QString& filename)
{
    DataTable* table = new DataTable;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << file.errorString();
        return table;
    }
    QByteArray blob = file.readAll();

    QStringList filecontent;

    filecontent = QString(blob).split("\n");
    for (const QString& line : filecontent) {
        //qDebug() << line;
        auto vector = String2DoubleEigVec(QString(line).replace("\t", " ").replace(",", "."));
        //  std::cout << vector << std::endl;
        table->insertRow(vector);
    }

    return table;
}

QPair<Vector, Vector> LoadCSVFile(const QString& filename)
{
    Vector x, y;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << file.errorString();
        return QPair<Vector, Vector>(x, y);
    }
    QByteArray blob = file.readAll();

    QStringList filecontent;

    filecontent = QString(blob).split("\n");
    bool data = false;
    std::vector<double> entries_x, entries_y;
    for (const QString& str : filecontent) {
        //str.remove("\r");
        if (str.simplified().contains("XYDATA")) {
            data = true;
            continue;
        }
        if (data) {
            QStringList elements = str.simplified().split(";");
            if (elements.size() == 2) {
                entries_x.push_back(String2Double(elements[0]));
                entries_y.push_back(String2Double(elements[1]));
            } else
                data = false;
        }
    }

    x = Vector::Map(&entries_x[0], entries_x.size());
    y = Vector::Map(&entries_y[0], entries_y.size());

    return QPair<Vector, Vector>(x, y);
}

QPair<Vector, Vector> LoadAbsorbFile(const QString& filename)
{
    Vector x, y;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << file.errorString();
        return QPair<Vector, Vector>(x, y);
    }
    QByteArray blob = file.readAll();

    QStringList filecontent;

    filecontent = QString(blob).split("\n");
    bool data = false;
    std::vector<double> entries_x, entries_y;
    for (const QString& str : filecontent) {
        if (str.simplified().contains("Pixel") && str.simplified().contains("Wavelength")) {
            data = true;
            continue;
        }
        if (data) {
            QStringList elements = str.simplified().split(" ");
            if (elements.size() == 3) {
                entries_x.push_back(String2Double(elements[1]));
                entries_y.push_back(String2Double(elements[2]));
            } else
                data = false;
        }
    }

    x = Vector::Map(&entries_x[0], entries_x.size());
    y = Vector::Map(&entries_y[0], entries_y.size());

    return QPair<Vector, Vector>(x, y);
}

QPair<Vector, Vector> LoadXYFile(const QString& filename)
{
    Vector x, y;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << file.errorString();
        return QPair<Vector, Vector>(x, y);
    }
    QByteArray blob = file.readAll();

    QStringList filecontent;

    filecontent = QString(blob).split("\n");

    std::vector<double> entries_x(filecontent.size()), entries_y(filecontent.size());
    int index = 0;
    for (const QString& str : filecontent) {
        if (!str.contains("#")) {
            QStringList elements = str.simplified().split(" ");
            if (elements.size() == 2) {
                entries_x[index] = (String2Double(elements[0]));
                entries_y[index] = (String2Double(elements[1]));
                index++;
            }
        }
    }

    x = Vector::Map(&entries_x[0], index);
    y = Vector::Map(&entries_y[0], index);

    return QPair<Vector, Vector>(x, y);
}

QPair<PeakPick::spectrum, QJsonObject> LoadITCFile(QString& filename, std::vector<PeakPick::Peak>* peaks, qreal& offset, qreal& freq, QVector<qreal>& inject)
{
    QJsonObject systemparameter;

    Vector x, y;

    QFileInfo info(filename);
    if (!info.exists()) {
        QStringList list = filename.split("/");
        QString file = list.last();
        QString lastdir = qApp->instance()->property("lastdir").toString();
        QFileInfo inf(lastdir + "/" + file);
        if (inf.exists())
            filename = lastdir + "/" + file;
        else {
            qDebug() << QString(lastdir + "/" + file) << "doesnt work";
            throw 404;
        }
    }
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << file.errorString();
        throw 404;
    }

    QStringList filecontent = QString(file.readAll()).split("\n");

    std::vector<double> entries_x, entries_y;
    bool start_peak = false, skip = false;
    qreal last_x = 0;
    offset = 0;
    PeakPick::Peak floating_peak;
    int i_offset = 0, sytemcounter = 0, index = -1;
    for (const QString& str : filecontent) {
        if (str.contains("$") || str.contains("#") || str.contains("?") || str.contains("%")) {
            if (str.contains("%")) {
            }
            if (str.contains("$")) {
                if (str.simplified().split(" ").size() == 8) {
                    double val = str.simplified().split(" ").last().toDouble();
                    freq = val;
                }
            }
            if (str.contains("#")) {
                double val = QString(str).remove("#").toDouble();
                if (qFuzzyCompare(val, 0)) {
                    sytemcounter++;
                    continue;
                }
                if (sytemcounter == 4)
                    systemparameter[QString::number(AbstractItcModel::Temperature)] = QString::number(val + 273.15);
                if (sytemcounter == 2)
                    systemparameter[QString::number(AbstractItcModel::CellConcentration)] = QString::number(val);
                if (sytemcounter == 1)
                    systemparameter[QString::number(AbstractItcModel::SyringeConcentration)] = QString::number(val);
                if (sytemcounter == 3)
                    systemparameter[QString::number(AbstractItcModel::CellVolume)] = QString::number(val * 1000);

                sytemcounter++;
            }
        } else if (str.contains("@")) {
            skip = (str.contains("@0"));
            if (skip)
                continue;
            start_peak = true;
            floating_peak.setPeakEnd(index);
            if (last_x && floating_peak.start)
                peaks->push_back(floating_peak);
            inject << str.split(",")[1].toDouble();
        } else {
            QStringList elements = str.simplified().split(",");
            if (elements.size() > 2) {
                if (skip) {
                    offset += elements[1].toDouble();
                    i_offset++;
                    last_x = elements[0].toDouble() / freq;
                }
                entries_x.push_back(elements[0].toDouble());
                entries_y.push_back(elements[1].toDouble());
                index++;
                if (start_peak) {
                    // floating_peak.setPeakStart(elements[0].toDouble() / freq);
                    floating_peak.setPeakStart(index);
                    start_peak = false;
                }
                last_x = elements[0].toDouble() / freq;
            }
        }
    }

    //floating_peak.setPeakEnd(last_x);
    floating_peak.setPeakEnd(index);
    peaks->push_back(floating_peak);

    offset /= double(i_offset);
    if (entries_x.size() < 1 || entries_y.size() < 1 || entries_x.size() != entries_y.size())
        throw 101;

    x = Vector::Map(&entries_x[0], entries_x.size());
    y = Vector::Map(&entries_y[0], entries_y.size());

    return QPair<PeakPick::spectrum, QJsonObject>(PeakPick::spectrum(x, y), systemparameter);
}

}
