#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qcustomplot.h"
#include <QSlider>
#include <QTimer>
#include <QVector>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QTouchEvent>
#include <QLabel>
#include <QDateTime>
#include <QPushButton>
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

    QCustomPlot *plot;   
    QSlider *slider;
    QTimer *timer;
    QLabel *logo;

    QVector<QVector<double>> redData; // 用于存储每个传感器的红光数据
    QVector<QVector<double>> irData;  // 用于存储每个传感器的红外数据
    QVector<double> xData;  // 时间数据

    int sampleCount;
    int middle_sensor = 0;
    QDateTime startTime;
    int windowSize;

    QVector<uint8_t> channels; // 用于存储通道信息
    QPushButton *exitButton;
    QPushButton *button1;
    QPushButton *button2;

    QPoint lastTouchPos; // 记录触摸位置
    bool isTouching;
    MAX30102 max30102; // 传感器对象
};

#endif // MAINWINDOW_H
