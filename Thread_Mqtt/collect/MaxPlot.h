#ifndef MAXPLOT_H
#define MAXPLOT_H

#include <QMainWindow>
#include "qcustomplot.h"
#include <QSlider>
#include <QTimer>
#include <QVector>
#include <QList>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QTouchEvent>
#include <QLabel>
#include <QDateTime>
#include <QTimer>
#include <QPushButton>
#include "max30102.h"


class MaxPlot : public QMainWindow
{
    Q_OBJECT

public:
    MaxPlot(QWidget *parent = nullptr);
    ~MaxPlot();

signals:
    void windowClosed();

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    
    void pinchTriggered(QPinchGesture *gesture);
    bool gestureEvent(QGestureEvent *event);
    //void touchEvent(QTouchEvent *event);

private slots:
    void updatePlot();
    void onSliderMoved(int position);
    void onExitButtonClicked();
    
    void onSliderPressed();
    void onSliderReleased();

private:
    void setupPlot();
    void setupSlider();
    void setupGestures();

    QCustomPlot *plot;
    QSlider *slider;
    QTimer *timer;
    QLabel *logo;
    QVector<double> redData_middle, irData_middle,red0,red1,red2,red3,ir1,ir2,ir3,ir0;
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
    bool isView;
    MAX30102 *max30102;
};

#endif // MAINWINDOW_H
