/*
 * This file handles all optimization functions
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

#ifndef MINIMIZER_H
#define MINIMIZER_H
#include "src/core/AbstractModel.h"
#include "src/ui/widgets/modelhistorywidget.h"
#include <QtCore/QSharedPointer>
#include <QtCore/QJsonObject>

class AbstractTitrationModel;

class Minimizer : public QObject
{
    Q_OBJECT
public:
    Minimizer(QObject *parent = 0);
    ~Minimizer();
    void setModel(const QSharedPointer<AbstractTitrationModel> model);
    int Minimize();
    void setOptimizerConfig(const OptimizerConfig &config) 
    { 
        m_opt_config = config;
        m_inform_config_changed = true;
    }
    inline OptimizerConfig getOptimizerConfig() const { return m_opt_config; }
    void addToHistory();
    QJsonObject Parameter() const;
    
private:
    QString OptPara2String() const;
    QSharedPointer<AbstractTitrationModel> m_model;
    OptimizerConfig m_opt_config;
    bool m_inform_config_changed;
    
    qreal ModelError() const;
signals:
    void Message(const QString &str, int priority);
    void Warning(const QString &str, int priority);
    void RequestCrashFile();
    void RequestRemoveCrashFile();
    void InsertModel(const ModelHistoryElement &element);
};

#endif // MINIMIZER_H
