#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qcustomplot.h"
#include <QSlider>
#include <QTimer>
#include <QVector>
#include<QGestureEvent>
#include<QPinchGesture>
#include<QTouchEvent>
#include<QLabel>
#include<QDateTime>
#include<QTimer>
#include<QPushButton>
#include "max30102.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool event(QEvent *event) override;
    void pinchTriggered(QPinchGesture *gesture);
    bool gestureEvent(QGestureEvent *event);
    void touchEvent(QTouchEvent *event);

private slots:
    void updatePlot();
    void onSliderMoved(int position);
    void onExitButtonClicked();

private:
    void setupPlot();
    void setupSlider();
    void setupGestures();
    void StopRun();
    void BeginRun();
    // void onLeftButtonReleased();
    // void onRightButtonReleased();

    QCustomPlot *plotTop;   
    QCustomPlot *plotBottom; 
    QSlider *slider;
    QTimer *timer;
    QLabel *logo;
    QVector<double> redData, irData;
    QVector<double> xData;
    
    int sampleCount;
    QDateTime startTime;
    int windowSize;

    QVector<uint8_t> channels;
    QPushButton *exitButton;
    QPushButton *button1;
    QPushButton *button2;

    QPoint lastTouchPos;
    bool isTouching;
};

#endif // MAINWINDOW_H

