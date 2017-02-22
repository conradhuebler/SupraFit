/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <src/core/models.h>
#include <src/core/jsonhandler.h>

#include <QtCore/QJsonObject>

#include <QtTest/QtTest>

class TestJson : public QObject
{
    Q_OBJECT


private slots:
    void ImportExport();

};


void TestJson::ImportExport
{
    QJsonObject toplevel;
    if(JsonHandler::ReadJsonFile(toplevel, "../data/samples/2_1_1_1_testcase.json"))
    {
        QPointer<DataClass > data = new DataClass(toplevel);
        if(data->DataPoints() != 0)
        {
            SetData(new DataClass(new DataClass(data)), str);
            m_model_dataholder->AddToWorkspace(toplevel);
            QPointer< AbstractTitrationModel > t = new ItoI_ItoII_Model(data);
            QJsonObject export_file = t->ExportJson();
            QCOMPARE(toplevel, export_file);   
        }
    }
}

QTEST_MAIN(TestJson)
#include "testjson.moc"
