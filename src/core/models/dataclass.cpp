/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global.h"
#include "src/global_config.h"

#include "src/core/models/datatable.h"
#include "src/core/toolset.h"

#include <Eigen/Dense>

#include <QtCore/QAbstractTableModel>
#include <QtCore/QCollator>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QPointer>
#include <QtCore/QReadWriteLock>
#include <QtCore/QString>
#include <QtCore/QUuid>
#include <QtCore/QtGlobal>

#include <cmath>
#include <iostream>
#include <random>

#include "dataclass.h"


DataClassPrivate::DataClassPrivate()
    : m_maxsize(0)
    , m_datatype(DataClassPrivate::Table)
    , m_info(new DataClassPrivateObject)
{
    m_independent_model = new DataTable;
    m_dependent_model = new DataTable;

    QUuid uuid;
    m_uuid = uuid.createUuid().toString();

    m_dependent_model->setCheckable(true);
    if (m_independent_model->columnCount() != m_scaling.size())
        for (int i = 0; i < m_independent_model->columnCount(); ++i)
            m_scaling << 1;
    m_independent_model->setHeaderData(0, Qt::Horizontal, ("Host"), Qt::DisplayPropertyRole);
    m_independent_model->setHeaderData(1, Qt::Horizontal, ("Guest"), Qt::DisplayPropertyRole);
    //  std::cout << "stepping upwards DataClassPrivate::DataClassPrivate(): " << m_ref_counter << this << std::endl;
}

DataClassPrivate::DataClassPrivate(int type)
    : m_maxsize(0)
    , m_datatype(DataClassPrivate::Table)
    , m_info(new DataClassPrivateObject)
{
    m_independent_model = new DataTable;
    m_dependent_model = new DataTable;
    m_dependent_model->setCheckable(true);

    QUuid uuid;
    m_uuid = uuid.createUuid().toString();

    if (m_independent_model->columnCount() != m_scaling.size())
        for (int i = 0; i < m_independent_model->columnCount(); ++i)
            m_scaling << 1;
    m_independent_model->setHeaderData(0, Qt::Horizontal, ("Host"), Qt::DisplayPropertyRole);
    m_independent_model->setHeaderData(1, Qt::Horizontal, ("Guest"), Qt::DisplayPropertyRole);
    //  std::cout << "stepping upwards DataClassPrivate::DataClassPrivate(): " << m_ref_counter << this  << std::endl;
}

DataClassPrivate::DataClassPrivate(const DataClassPrivate& other)
    : QSharedData(other)
    , m_info(other.m_info)
{
    m_independent_model = new DataTable(other.m_independent_model);
    m_systemObject = other.m_systemObject;
    m_uuid = other.m_uuid;
    m_scaling = other.m_scaling;
    m_dependent_model = new DataTable(other.m_dependent_model);
    m_raw_data = other.m_raw_data;
    m_system_parameter = other.m_system_parameter;
    m_datatype = other.m_datatype;
    m_title = other.m_title;
    m_root_dir = other.m_root_dir;
    m_ref_counter++;
}

DataClassPrivate::DataClassPrivate(const DataClassPrivate* other)
    : m_info(other->m_info)
{
    m_independent_model = new DataTable(other->m_independent_model);
    m_systemObject = other->m_systemObject;
    m_uuid = other->m_uuid;
    m_scaling = other->m_scaling;
    m_dependent_model = new DataTable(other->m_dependent_model);
    m_raw_data = other->m_raw_data;
    m_system_parameter = other->m_system_parameter;
    m_datatype = other->m_datatype;
    m_title = other->m_title;
    m_root_dir = other->m_root_dir;
    m_ref_counter++;
}

DataClassPrivate::~DataClassPrivate()
{
    --m_ref_counter;
    if (m_ref_counter < 1) {
        if (m_independent_model)
            delete m_independent_model;
        if (m_dependent_model)
            delete m_dependent_model;
        if (m_info)
            delete m_info;
#ifdef _DEBUG
        std::cout << "got away with it ..." << this << std::endl;
#endif
    }
#ifdef _DEBUG
    else
        std::cout << "still someone having data: " << m_ref_counter << this << std::endl;
#endif
}


void DataClassPrivate::check()
{
    std::cout << "Check of data " << std::endl;
    std::cout << "Concentration Table ## Row:" << m_independent_model->rowCount() << " Colums: " << m_independent_model->columnCount() << std::endl;
    std::cout << "Signal Table ## Row:" << m_dependent_model->rowCount() << " Colums: " << m_dependent_model->columnCount() << std::endl;
}

DataClass::DataClass(QObject* parent)
    : QObject(parent)
{
    d = new DataClassPrivate;

    connect(Info(), &DataClassPrivateObject::SystemParameterChanged, this, &DataClass::SystemParameterChanged);
    connect(Info(), &DataClassPrivateObject::Update, this, &DataClass::Update);
    connect(Info(), &DataClassPrivateObject::Message, this, &DataClass::Message);
    connect(Info(), &DataClassPrivateObject::Warning, this, &DataClass::Warning);
}

DataClass::DataClass(const QJsonObject& json, int type, QObject* parent)
    : QObject(parent)
{
    d = new DataClassPrivate();

    connect(Info(), &DataClassPrivateObject::SystemParameterChanged, this, &DataClass::SystemParameterChanged);
    connect(Info(), &DataClassPrivateObject::Update, this, &DataClass::Update);
    connect(Info(), &DataClassPrivateObject::Message, this, &DataClass::Message);
    connect(Info(), &DataClassPrivateObject::Warning, this, &DataClass::Warning);

    ImportData(json);
}

DataClass::DataClass(const DataClass& other)
    : QObject()
{
    d = other.d;
    connect(Info(), &DataClassPrivateObject::SystemParameterChanged, this, &DataClass::SystemParameterChanged);
    connect(Info(), &DataClassPrivateObject::Update, this, &DataClass::Update);
    connect(Info(), &DataClassPrivateObject::Message, this, &DataClass::Message);
    connect(Info(), &DataClassPrivateObject::Warning, this, &DataClass::Warning);
}

DataClass::DataClass(const DataClass* other)
    : QObject()
{
    d = other->d;
    connect(Info(), &DataClassPrivateObject::SystemParameterChanged, this, &DataClass::SystemParameterChanged);
    connect(Info(), &DataClassPrivateObject::Update, this, &DataClass::Update);
    connect(Info(), &DataClassPrivateObject::Message, this, &DataClass::Message);
    connect(Info(), &DataClassPrivateObject::Warning, this, &DataClass::Warning);
}

DataClass::~DataClass()
{
#ifdef _DEBUG
    std::cout << "delete dataclass" << std::endl;
    d->printReferenz();
#endif
}

void DataClass::NewUUID()
{
    QUuid uuid;
    d->m_uuid = uuid.createUuid().toString();
}

QList<double> DataClass::getSignals(QList<int> active_signal)
{
    if (active_signal.size() < SeriesCount())
        active_signal = QVector<int>(SeriesCount(), 1).toList();

    QList<double> x;
    for (int j = 0; j < SeriesCount(); ++j) {
        if (active_signal[j] != 1)
            continue;
        for (int i = 0; i < DataPoints(); ++i) {
            if (DependentModel()->isChecked(i, j))
                x.append(DependentModel()->data(i, j));
        }
    }
    return x;
}

const QJsonObject DataClass::ExportData() const
{
    QJsonObject json;

    json["independent"] = d->m_independent_model->ExportTable(true);
    json["dependent"] = d->m_dependent_model->ExportTable(true);
    json["system"] = d->m_systemObject;
    json["DataType"] = d->m_datatype;
    json["SupraFit"] = qint_version;
    json["raw"] = d->m_raw_data;
    json["title"] = d->m_title;
    json["uuid"] = d->m_uuid;
    json["git_commit"] = git_commit_hash;
    json["content"] = d->m_content;
    json["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    json["xaxis"] = m_plot_x;
    return json;
}

bool DataClass::ImportData(const QJsonObject& topjson, bool forceUUID)
{
    int fileversion = topjson["SupraFit"].toInt();

    if (fileversion > qint_version) {
        emit Info()->Warning(QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2").arg(qint_version).arg(fileversion));
        qWarning() << QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2. Get the most recent version from <a href='https://github.com/conradhuebler/SupraFit'>https://github.com/conradhuebler/SupraFit</a>").arg(qint_version).arg(fileversion);
        return false;
    }

    d->m_systemObject = topjson["system"].toObject();

    d->m_independent_model->ImportTable(topjson["independent"].toObject());
    d->m_dependent_model->ImportTable(topjson["dependent"].toObject());

    d->m_datatype = DataClassPrivate::DataType(topjson["DataType"].toInt());
    d->m_raw_data = topjson["raw"].toObject();
    d->m_title = topjson["title"].toString();
    d->m_content = topjson["content"].toString();
    m_plot_x = topjson["xaxis"].toBool();

    if (forceUUID) {
        if (!topjson["uuid"].toString().isEmpty())
            d->m_uuid = topjson["uuid"].toString();
        else {
            QUuid uuid;
            d->m_uuid = uuid.createUuid().toString();
        }
    }

    if (d->m_independent_model->columnCount() != d->m_scaling.size())
        for (int i = 0; i < d->m_independent_model->columnCount(); ++i)
            d->m_scaling << 1;

    emit Info()->Update();
    return true;
}

bool DataClass::LegacyImportData(const QJsonObject& topjson, bool forceUUID)
{
    int fileversion = topjson["SupraFit"].toInt();

    if (fileversion == qint_version)
        return ImportData(topjson, forceUUID);

    if (fileversion > qint_version) {
        emit Info()->Warning(QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2").arg(qint_version).arg(fileversion));
        qWarning() << QString("One does not simply load this file. It appeared after Amon Hen!\nUpdating SupraFit to the latest version will fix this.\nCurrent fileversion is %1, version of saved file is %2").arg(qint_version).arg(fileversion);
        return false;
    }

    m_plot_x = topjson["xaxis"].toBool();

    d->m_systemObject = topjson["system"].toObject();

    if (fileversion >= 1601) {
        d->m_independent_model->ImportTable(topjson["independent"].toObject());
        d->m_dependent_model->ImportTable(topjson["dependent"].toObject());
    } else {
        QJsonObject concentrationObject, signalObject;
        concentrationObject = topjson["concentrations"].toObject();
        signalObject = topjson["signals"].toObject();
        if (concentrationObject.isEmpty() || signalObject.isEmpty())
            return false;

        QStringList keys = signalObject.keys();

        QCollator collator;
        collator.setNumericMode(true);
        std::sort(
            keys.begin(),
            keys.end(),
            [&collator](const QString& key1, const QString& key2) {
                return collator.compare(key1, key2) < 0;
            });
        if (DataPoints() == 0) {
            for (const QString& str : qAsConst(keys)) {
                QVector<qreal> concentrationsVector, signalVector;
                concentrationsVector = ToolSet::String2DoubleVec(concentrationObject[str].toString());
                signalVector = ToolSet::String2DoubleVec(signalObject[str].toString());
                d->m_independent_model->insertRow(concentrationsVector);
                d->m_dependent_model->insertRow(signalVector);
            }
            QStringList header = topjson["header"].toString().split("|");
            setHeader(header);
            return true;
        } else if (keys.size() != DataPoints()) {
            qWarning() << "table size doesn't fit to imported data";
            return false;
        }
        for (const QString& str : qAsConst(keys)) {
            QVector<qreal> concentrationsVector, signalVector;
            concentrationsVector = ToolSet::String2DoubleVec(concentrationObject[str].toString());
            signalVector = ToolSet::String2DoubleVec(signalObject[str].toString());
            int row = str.toInt();
            d->m_independent_model->setRow(concentrationsVector, row);
            d->m_dependent_model->setRow(signalVector, row);
        }

        QStringList header = topjson["header"].toString().split("|");
        setHeader(header);
    }
    if (fileversion > 1602) {
        d->m_datatype = DataClassPrivate::DataType(topjson["DataType"].toInt());
        d->m_raw_data = topjson["raw"].toObject();
        d->m_title = topjson["title"].toString();
    }
    if (forceUUID) {
        if (fileversion > 1603) {
            if (!topjson["uuid"].toString().isEmpty())
                d->m_uuid = topjson["uuid"].toString();
            else {
                QUuid uuid;
                d->m_uuid = uuid.createUuid().toString();
            }
        } else {
            QUuid uuid;
            d->m_uuid = uuid.createUuid().toString();
        }
    }

    if (d->m_independent_model->columnCount() != d->m_scaling.size())
        for (int i = 0; i < d->m_independent_model->columnCount(); ++i)
            d->m_scaling << 1;

    return true;
}

void DataClass::LoadSystemParameter()
{
    for (int index : getSystemParameterList()) {
        if (d->m_systemObject[QString::number(index)].toString().isEmpty())
            continue;
        setSystemParameterValue(index, d->m_systemObject[QString::number(index)].toVariant());
    }

    emit Info()->SystemParameterLoaded();
}

void DataClass::setSystemObject(const QJsonObject& object)
{
    d->m_systemObject = object;
}

void DataClass::setHeader(const QStringList& strlist)
{
    if (strlist.size() == (d->m_independent_model->columnCount() + d->m_dependent_model->columnCount())) {
        for (int i = 0; i < strlist.size(); ++i) {
            if (i < d->m_independent_model->columnCount())
                d->m_independent_model->setHeaderData(i, Qt::Horizontal, (strlist[i]), Qt::DisplayRole);
            else
                d->m_dependent_model->setHeaderData(i - d->m_independent_model->columnCount(), Qt::Horizontal, (strlist[i]), Qt::DisplayRole);
        }
    }
}

void DataClass::OverrideInDependentTable(DataTable* table)
{
    d.detach();
    d->m_independent_model = table;
    IndependentModelOverride();
}

void DataClass::OverrideDependentTable(DataTable* table)
{
    d.detach();
    table->setCheckedTable(d->m_dependent_model->CheckedTable());
    d->m_dependent_model = table;
    DependentModelOverride();
}

void DataClass::OverrideCheckedTable(DataTable* table)
{
    d.detach();
    d->m_dependent_model->setCheckedTable(table->CheckedTable());
    CheckedModelOverride();
    DependentModelOverride();
}

void DataClass::addSystemParameter(int index, const QString& str, const QString& description, SystemParameter::Type type)
{
    if (d->m_system_parameter.contains(index))
        return;
    SystemParameter parameter(index, str, description, type);
    d->m_system_parameter.insert(index, parameter);
}

SystemParameter DataClass::getSystemParameter(int index) const
{
    return d->m_system_parameter.value(index);
}

QList<int> DataClass::getSystemParameterList() const
{
    return d->m_system_parameter.keys();
}

void DataClass::setSystemParameterValue(int index, const QVariant& value)
{
    if (!value.isValid())
        return;

    SystemParameter parameter = getSystemParameter(index);
    parameter.setValue(value);
    d->m_system_parameter[index] = parameter;
}

void DataClass::setSystemParameterList(int index, const QStringList& value)
{
    SystemParameter parameter = getSystemParameter(index);
    if (!parameter.isList())
        return;
    parameter.setList(value);
    d->m_system_parameter[index] = parameter;
}

void DataClass::setSystemParameter(const SystemParameter& parameter)
{
    int index = parameter.Index();
    if (d->m_system_parameter.contains(index))
        d->m_system_parameter[index] = parameter;
    emit Info()->SystemParameterChanged();
}

void DataClass::WriteSystemParameter()
{
    QJsonObject systemObject;
    for (const int index : getSystemParameterList()) {
        systemObject[QString::number(index)] = getSystemParameter(index).value().toString();
    }
    d->m_systemObject = systemObject;
}

QJsonObject DataClass::ExportChildren(bool statistics, bool locked)
{
    QJsonObject models;
    if (SFModel() == SupraFit::Data) {
        for (int i = 0; i < d->m_children.size(); ++i) {
            if (d->m_children[i]) {
                if (qobject_cast<const AbstractModel*>(d->m_children[i])) {
                    QPointer<AbstractModel> data = const_cast<AbstractModel*>(qobject_cast<const AbstractModel*>(d->m_children[i]));
                    if (data)
                        models[tr("model_%1").arg(i)] = data->ExportModel(statistics, locked);
                }
            }
        }
    } else {
        AbstractModel* data = qobject_cast<AbstractModel*>(this);
        models[tr("model_0")] = data->ExportModel(statistics, locked);
    }

    return models;
}

void DataClass::AddChildren(QPointer<DataClass> data)
{
    d->m_children << data;
    connect(data, &DataClass::Deleted, this, [this, data]() {
        d->m_children.removeAll(data);
#ifdef _DEBUG
        qDebug() << d->m_children.size();
#endif
    });
}

QString DataClass::Data2Text() const
{
    QString text;
    text += "#### Begin of Data Description ####\n";
    text += "Independent Data Structure :   " + QString::number(DataPoints()) + "\n";
    for (int i = 0; i < IndependentModel()->columnCount(); ++i)
        text += " " + IndependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += IndependentModel()->ExportAsString();
    text += "\n";
    text += "Dependent Data Structure :          " + QString::number(SeriesCount()) + "\n";
    for (int i = 0; i < DependentModel()->columnCount(); ++i)
        text += " " + DependentModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\t";
    text += "\n";
    text += DependentModel()->ExportAsString();
    text += "\n";
    text += Data2Text_Private();
    text += "#### End of Data Description #####\n";
    text += "******************************************************************************************************\n";
    return text;
}
