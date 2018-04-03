/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QMap>

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QWidget>

class QVBoxLayout;
class QPushButton;

struct ModelHistoryElement {
    QJsonObject model;
    qreal error;
    qreal corr_coeff;
    QList<int> active_signals;
};

class ModelHistoryWidget : public QGroupBox {
    Q_OBJECT
public:
    ModelHistoryWidget(const QJsonObject* element, int active, int index, QWidget* parent = 0);
    inline ~ModelHistoryWidget() {}

private:
    const QJsonObject* m_json;
    QPushButton *m_add, *m_load, *m_remove;
    int m_index;

private slots:
    inline void AddModel() { emit AddJson(*m_json); }
    inline void LoadModel() { emit LoadJson(*m_json); }
    void remove();

signals:
    void LoadJson(const QJsonObject& json);
    void AddJson(const QJsonObject& json);
    void Remove(int index, QPointer<ModelHistoryWidget>);
};

class ModelHistory : public QWidget {
    Q_OBJECT
public:
    ModelHistory(QWidget* parent = 0);
    ~ModelHistory();
    void InsertElement(const QJsonObject& model, int active);
    void InsertElement(const QJsonObject& model);
    virtual QSize sizeHint() const { return QSize(210, 600); }

private:
    QMap<int, QJsonObject*> m_history;
    QWidget* m_mainwidget;
    QVBoxLayout* m_vlayout;
    int m_index;

private slots:
    void Remove(int index, QPointer<ModelHistoryWidget> element);

signals:
    void AddJson(const QJsonObject& json);
    void LoadJson(const QJsonObject& json);

private:
};
