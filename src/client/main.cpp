/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/client/console.h"

#include "src/global.h"
#include "src/global_config.h"
#include "src/version.h"

#include <QDebug>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>

#include <QCoreApplication>

#include <iostream>

int main(int argc, char** argv)
{
    qInstallMessageHandler(myMessageOutput);
    
    
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    Version(&app, &parser);
    
    QCommandLineOption experiments(QStringList() << "e" << "experiments",
                                   QCoreApplication::translate("main", "Number of experiments"),
                                   QCoreApplication::translate("main", "number"),
                                   QCoreApplication::translate("main", "1000"));
    parser.addOption(experiments);
    
    QCommandLineOption threads(QStringList() << "t" << "threads",
                                   QCoreApplication::translate("main", "Number of threads"),
                                   QCoreApplication::translate("main", "threads"),
                                   QCoreApplication::translate("main", "4"));
    parser.addOption(threads);

    QCommandLineOption _std(QStringList() << "s" << "std",
                            QCoreApplication::translate("main", "Standard deviation for the data generation"),
                            QCoreApplication::translate("main", "standard deviation"),
                            QCoreApplication::translate("main", "0.001"));
    parser.addOption(_std);
    parser.addOption({{"u", "reduction"}, "Enable Reduction analysis."});
    parser.addOption({{"c", "crossvalidation"}, "Enable Cross Validation analysis."});
    parser.addOption({{"m", "montecarlo"}, "Enable Monte Carlo analysis."});
    parser.addOption({{"o", "modelcomparison"}, "Enable Model Comparison analysis."});
    parser.addOption({{"w", "weakendgrid"}, "Enable Weakend Grid Search analysis."});
    
    parser.process(app);
    
    const QStringList args = parser.positionalArguments();
    
    int exp = parser.value("e").toInt();
    qreal std = parser.value("s").toDouble();
    bool reduction = parser.isSet("u");
    bool crossvalidation = parser.isSet("c");
    bool montecarlo = parser.isSet("m");
    bool modelcomparison = parser.isSet("o");
    bool weakendgrid = parser.isSet("w");
    qApp->instance()->setProperty("threads", parser.value("t").toInt());
    std::cout << SupraFit::about().toStdString() << std::endl;
    std::cout << "No. Experiments to be simulated " << exp << std::endl;
    std::cout << "Standard Deviation to be used " << std << std::endl;
    std::cout << "Reduction Analysis is turn on: " << reduction << std::endl;
    std::cout << "Cross Validation is turn on: " << crossvalidation << std::endl;
    std::cout << "Monte Carlo Simulation is turn on: " << montecarlo << std::endl;
    std::cout << "Model Comparison is turn on: " << modelcomparison << std::endl;
    std::cout << "Weakend Grid Search is turn on: " << weakendgrid << std::endl;
    std::cout << "Number of threads: " << qApp->instance()->property("threads").toInt() << std::endl;

    #ifdef _DEBUG
    qDebug() << "Debug output enabled, good fun!";
    #endif
    
    int count = 0;  
    
    for(const QString &file : args)
    {
        if(!file.isEmpty() && !file.isNull())
        {
            Console console(exp, std);
            
            console.setReduction(reduction);
            console.setCrossValidation(crossvalidation);
            console.setModelComparison(modelcomparison);
            console.setWeakendGridSearch(weakendgrid);
            console.setMonteCarlo(montecarlo);
            
            if(console.LoadFile(file))
            {
                ++count;
                console.FullTest();
            }
        }
    }

    if(!count)
    {
        std::cout << "Nothing found to be done" << std::endl;
        return 0;
    }else
        return app.exec();
}
