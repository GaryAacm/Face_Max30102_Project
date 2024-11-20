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
#include "MQTTWorker.h"
#include <queue>
#include <QJsonObject>
#include <QJsonDocument>

#define ADDRESS "tcp://localhost:1883"
#define CLIENTID "backend-client"
#define TOPIC "sensor/data"
#define QOS 1
#define TIMEOUT 10000L


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
    void sendMQTTMessage(const QString &message);
    void Finish_ALL();

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
    void onStartButtonClicked();
    void Read_Data_Thread();
    void Update_Plot_Thread();
    void Mqtt_Thread();
    void Get_Mqtt_Message();

    void onSliderPressed();
    void onSliderReleased();

    //void start_Plot_Timer();
    void Http_Worker_Start();
    void startTimerIfReady();

    void stop_Plot_Timer();
    void stop_Mqtt_Thread();
    void stop_Read_Thread();

public:
    void Init_GUI_SHOW();
    void Setup_Background();

    void setupPlot();
    void setupSlider();
    void setupGestures();
    void handleDataReady(const MaxData &data);
    void Start_To_Read();
    void End_All_Test();

    int red_send_data[DATA_NUM], ir_send_data[DATA_NUM], channel_send_id[DATA_NUM];

    QCustomPlot *plot;
    QSlider *slider;
    QTimer *timer, *Mqtt_timer;
    QLabel *logo;

    QVector<double> redData_middle, irData_middle, red0, red1, red2, red3, ir1, ir2, ir3, ir0;
    QVector<double> xData;

    int sampleCount;
    QDateTime startTime;
    int windowSize;

    QVector<uint8_t> channels;
    QPushButton *exitButton;
    QPushButton *startButton;

    queue<uint32_t> Queue_Plot[8], Queue_Mqtt[8];

    QPoint lastTouchPos;

    bool isTouching;
    bool isView;
    bool Thread_Running_Plot = true;
    bool Thread_Running_Mqtt = true;
    bool receivedFinishPlot = false;
    bool receivedFinishMqtt = false;

    QRCodeGenerator *qr;
    time_t Start_TimeStamp;
    time_t End_TimeStamp;
    uint32_t count_data = 0;

    MaxDataWorker *worker;
    QThread *workerThread;
    QThread *timerThread;
    QThread *mqttThread;
    MQTTWorker *mqttWorker;
    MAX30102 *max30102;
};

#endif // MAINWINDOW_H
