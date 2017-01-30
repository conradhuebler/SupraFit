/*
 * <one line to give the library's name and an idea of what it does.>
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

#ifndef MODELHISTORYWIDGET_H
#define MODELHISTORYWIDGET_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QGroupBox>

#include <QtWidgets/QScrollArea>
#include <QtCore/QMap>
#include <QtCore/QJsonObject>

class QVBoxLayout;
class QPushButton;

struct ModelHistoryElement
{
    QJsonObject model;
    qreal error;
    qreal corr_coeff;
    QVector<int > active_signals;
};

class ModelHistoryWidget : public QGroupBox
{
  Q_OBJECT
public:
    ModelHistoryWidget(const ModelHistoryElement *element,  QWidget *parent = 0);
    ~ModelHistoryWidget(){ };
    
private:
    const QJsonObject *m_json;
    QPushButton *m_add, *m_load, *m_remove;
private slots:
    inline void AddModel() { emit AddJson(*m_json); }
    inline void LoadModel() { emit LoadJson(*m_json); }
    void remove(); 
signals:
    void LoadJson(const QJsonObject &json);
    void AddJson(const QJsonObject &json);
    void Remove(const QJsonObject *json, QPointer<ModelHistoryWidget>);
};


class ModelHistory : public QWidget
{
    Q_OBJECT
public:
    ModelHistory(QMap<int, ModelHistoryElement> *history, QWidget *parent = 0);
    ~ModelHistory();
    void InsertElement(const ModelHistoryElement *elm);
    
private:
    QMap<int, ModelHistoryElement> *m_history;
    QWidget *m_mainwidget;
    QVBoxLayout *m_vlayout;
    
private slots:
    void Remove(const QJsonObject *json, QPointer<ModelHistoryWidget> element);
    
signals:
    void AddJson(const QJsonObject &json);
    void LoadJson(const QJsonObject &json);
private:
};

#endif // MODELHISTORYWIDGET_H
