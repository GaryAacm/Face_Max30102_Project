#ifndef MAXPLOT_H
#define MAXPLOT_H

#include <QMainWindow>
#include "qcustomplot.h"
#include <QSlider>
#include <QTimer>
#include <QVector>
#include <QList>
#include <QThread>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QTouchEvent>
#include <QLabel>
#include <QDateTime>
#include <QTimer>
#include <QPushButton>
#include "max30102.h"
#include "QRCodeGenerator.h"
#include <ctime>
#include <QTimer>
#include <QDebug>
#include <QObject>
#include <QThread>
#include "MaxDataWorker.h"
#include<QQueue>

using namespace std;
const int DATA_NUM = 200005;

class MaxPlot : public QMainWindow
{
    Q_OBJECT

public:
    MaxPlot(QWidget *parent, QRCodeGenerator *qrGenerator);
    ~MaxPlot();

signals:
    void windowClosed();

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    void pinchTriggered(QPinchGesture *gesture);
    bool gestureEvent(QGestureEvent *event);
    // void touchEvent(QTouchEvent *event);
    // void Send_Message(const std::string &Start_Unix, const std::string &End_Unix,const int *Red_Data, const int *IR_Data, size_t Data_Size,const std::string &userdata);

private slots:
    void updatePlot();
    void onSliderMoved(int position);
    void onExitButtonClicked();

    void onSliderPressed();
    void onSliderReleased();

public:
    void setupPlot();
    void setupSlider();
    void setupGestures();
    void handleDataReady(const MaxData &data);

    void Push_data(QQueue<uint32_t> &red_queue, QQueue<uint32_t> &ir_queue, QVector<double> &redVector, QVector<double> &irVector,
                        int &send_red, int &send_ir, int send_channel);

    int red_send_data[DATA_NUM], ir_send_data[DATA_NUM], channel_send_id[DATA_NUM];

    QCustomPlot *plot;
    QSlider *slider;
    QTimer *timer;
    QLabel *logo;

    QVector<double> redData_middle, irData_middle, red0, red1, red2, red3, ir1, ir2, ir3, ir0;
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
    QRCodeGenerator *qr;
    time_t Start_TimeStamp;
    time_t End_TimeStamp;
    uint32_t count_data = 0;

    MaxDataWorker *worker;
    QThread *workerThread;
};

#endif // MAINWINDOW_H
