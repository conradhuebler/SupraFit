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

#ifndef MODELHISTORYDIALOG_H
#define MODELHISTORYDIALOG_H

#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>

#include <QtCore/QMap>
#include <QtCore/QJsonObject>

class QVBoxLayout;

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
    ModelHistoryWidget(const ModelHistoryElement *element, QWidget *parent = 0);
    ~ModelHistoryWidget();
    
private:
    const QJsonObject *m_json;
    QPushButton *m_add, *m_load;
    
private slots:
    inline void AddModel() { emit AddJson(*m_json); }
    inline void LoadModel() { emit LoadJson(*m_json); }
signals:
    void LoadJson(const QJsonObject &json);
    void AddJson(const QJsonObject &json);
};

class ModelHistoryDialog : public QDialog
{
    Q_OBJECT
public:
    ModelHistoryDialog(QMap<int, ModelHistoryElement> *history, QWidget *parent = 0);
    ~ModelHistoryDialog();
    
    virtual void show();
private:
    QMap<int, ModelHistoryElement> *m_history;
    void MakeList();
    QWidget *m_mainwidget;
    QVBoxLayout *m_vlayout;
signals:
    void AddModel(const QJsonObject &json);
    void LoadModel(const QJsonObject &json);
};

#endif // MODELHISTORYDIALOG_H
