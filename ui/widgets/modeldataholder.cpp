/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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




//#include "ui/widgets/modelwidget.h"
#include "ui/widgets/datawidget.h"


#include <QtWidgets/QGridLayout>


#include "modeldataholder.h"

ModelDataHolder::ModelDataHolder()
{
    QGridLayout *layout = new QGridLayout;
    
    setLayout(layout);
    
    
    m_datawidget = new DataWidget;
    
    layout->addWidget(m_datawidget);
    
}

ModelDataHolder::~ModelDataHolder()
{
}

#include "modeldataholder.moc"
