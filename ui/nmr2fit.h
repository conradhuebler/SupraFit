#ifndef nmr2fit_H
#define nmr2fit_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>
#include <QtCore/QPointer>
#include <QtCharts/QLineSeries>
class ModelDataHolder;
class ChartWidget;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    virtual ~MainWindow();
    
    
    
    
    
private:
    QPointer<QSplitter >m_mainsplitter;
    QPointer<ChartWidget > m_charts;
    QPointer<ModelDataHolder > m_model_dataholder;
    
    
private slots:
    void LoadData();
    void PlotData(QVector< QPointer< QtCharts::QLineSeries > > data);
};

#endif // nmr2fit_H
