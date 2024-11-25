#include "MaxPlot.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QDebug>
#include <QPixmap>
#include <QDateTime>
#include <MQTTClient.h>
#include <iostream>
#include <QMessageBox>
#include <QTimer>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <QTimer>
#include <QDebug>
#include <QObject>
#include <QThread>
#include <QStringList>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

using namespace std;
using namespace nlohmann;

MaxPlot::MaxPlot(QWidget *parent, QRCodeGenerator *qrGenerator)
    : QMainWindow(parent), plot(new QCustomPlot(this)), logo(new QLabel(this)),
      sampleCount(0), windowSize(50), isTouching(false), qr(qrGenerator)
{
    Init_GUI_SHOW();
}

MaxPlot::~MaxPlot()
{
}

void MaxPlot::Init_GUI_SHOW()
{
    Setup_Background();

    Start_To_Read();

    End_All_Test();
}

void MaxPlot::Start_To_Read()
{
    connect(startButton, &QPushButton::clicked, this, &MaxPlot::onStartButtonClicked);
    connect(this, &MaxPlot::Finish_ALL, this, &MaxPlot::startTimerIfReady);
}

void MaxPlot::startTimerIfReady()
{
    stop_Read_Thread();
    stop_Plot_Timer();
    stop_Mqtt_Thread();
    disconnect(this, &MaxPlot::Finish_ALL, this, &MaxPlot::startTimerIfReady);

    QTimer *timer_start = new QTimer(this);

    // Wait 5 minutes to start
    timer_start->setInterval(300000);
    timer_start->start();

    connect(timer_start, &QTimer::timeout, this, &MaxPlot::Start_To_Read);
}

void MaxPlot::End_All_Test()
{
    connect(exitButton, &QPushButton::clicked, this, &MaxPlot::onExitButtonClicked);
}

void MaxPlot::Setup_Background()
{
    this->setStyleSheet("background-color:black;");

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color:black");
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QPixmap logoPixmap("/home/orangepi/Desktop/zjj/Qt_use/logo.png");
    QPixmap scaledLogo = logoPixmap.scaled(300, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    logo->setPixmap(scaledLogo);
    logo->setAlignment(Qt::AlignCenter);

    logo->setStyleSheet("background-color:black;");
    logo->setFixedHeight(120);

    setupPlot();
    setupSlider();
    setupGestures();

    // Get start time stamp
    Start_TimeStamp = time(nullptr);

    startButton = new QPushButton("Begin", this);
    startButton->setFixedSize(100, 40);
    startButton->setStyleSheet("background-color:white;");

    exitButton = new QPushButton("Exit", this);
    exitButton->setFixedSize(100, 40);
    exitButton->setStyleSheet("background-color:white;");

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(exitButton);
    topLayout->addWidget(startButton);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(slider);

    mainLayout->addWidget(logo, 0);
    mainLayout->addWidget(plot, 1);

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(bottomLayout, 0);

    setCentralWidget(centralWidget);

    startTime = QDateTime::currentDateTime();

    setAttribute(Qt::WA_AcceptTouchEvents);

    // showFullScreen();
}

void MaxPlot::onStartButtonClicked()
{
    Read_Data_Thread();
    Update_Plot_Thread();
    //Mqtt_Thread();
    //corrupted double-linked list
    //Aborted (core dumped)

}

void MaxPlot::Read_Data_Thread()
{
    max30102 = new MAX30102("/dev/i2c-4");
    worker = new MaxDataWorker(nullptr, max30102);

    workerThread = new QThread();
    worker->moveToThread(workerThread);

    connect(workerThread, &QThread::started, worker, &MaxDataWorker::doWork);

    connect(max30102, &MAX30102::dataReady, this, &MaxPlot::handleDataReady);
    connect(worker, &MaxDataWorker::finishRead, this, &MaxPlot::Http_Worker_Start);
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);

    qRegisterMetaType<MaxData>("MaxData");

    workerThread->start();
}

void MaxPlot::stop_Read_Thread()
{
    // To Do
    max30102->Quit();
    if (!max30102)
        delete max30102;

    workerThread->quit();
    workerThread->wait();

    if (!workerThread)
        delete workerThread;
}

void MaxPlot::Update_Plot_Thread()
{

    connect(max30102, &MAX30102::dataReady, this, &MaxPlot::updatePlot);

    connect(timerThread, &QThread::finished, timer, &QObject::deleteLater);
}

void MaxPlot::Mqtt_Thread()
{
    mqttThread = new QThread();
    mqttWorker = new MQTTWorker(ADDRESS, CLIENTID, TOPIC, QOS, TIMEOUT);
    mqttWorker->moveToThread(mqttThread);

    connect(this, &MaxPlot::sendMQTTMessage, mqttWorker, &MQTTWorker::publishMessage);
    mqttThread->start();

    connect(max30102, &MAX30102::dataReady, this, &MaxPlot::Get_Mqtt_Message);
    // QObject::connect(QThread, Unknown) : invalid nullptr parameter
    //                                          Start to Plot
    //                                              MQTT READY
    //                                                  datacount is : 399
}

void MaxPlot::stop_Plot_Timer()
{
    if (timer && timer->isActive())
    {
        timer->stop();
    }

    timerThread->quit();
    timerThread->wait();
    if (!timerThread)
        delete timerThread;
}

void MaxPlot::stop_Mqtt_Thread()
{
    if (Mqtt_timer && Mqtt_timer->isActive())
    {
        Mqtt_timer->stop();
    }

    mqttWorker->stop();
    delete mqttWorker;

    mqttThread->quit();
    mqttThread->wait();
    if (!mqttThread)
        delete mqttThread;
}

void Send_Message(const std::string &Start_Unix, const int *Channel_ID, const int *Red_Data, const int *IR_Data, const std::string &sample_id, const std::string &uuid, const int fre)
{
    CURL *curl = curl_easy_init();
    if (curl)
    {
        int Data_Size = 45 * 800;
        json jsonData;
        jsonData["Start_Unix"] = Start_Unix;

        jsonData["channel_id"] = std::vector<int>(Channel_ID, Channel_ID + Data_Size);
        jsonData["data"]["ir"] = std::vector<int>(IR_Data, IR_Data + Data_Size);
        jsonData["data"]["reds"] = std::vector<int>(Red_Data, Red_Data + Data_Size);

        jsonData["sample_id"] = sample_id;
        jsonData["user_uuid"] = uuid;

        jsonData["frequency"] = fre;

        string jsonString = jsonData.dump();

        std::string url = "http://sp.grifcc.top:8080/collect/data";

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // POST DATA
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            std::cerr << "curl_easy_perform() 失败: " << curl_easy_strerror(res) << std::endl;

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    else
    {
        std::cerr << "curl_easy_init() 失败" << std::endl;
    }
}

void MaxPlot::Http_Worker_Start()
{
    // cout << "Collect data is:" << count_data << endl;

    // Init HTTP
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK)
    {
        std::cout << "curl_global_init() 失败: " << curl_easy_strerror(res) << std::endl;
    }

    // Send ID
    string userdata = qr->user_message;
    size_t pos = userdata.find(',');
    string sample_id = userdata.substr(0, pos);
    string uuid = userdata.substr(pos + 1);

    // Send Time
    long ts_start = static_cast<long>(Start_TimeStamp);
    string Start_string = to_string(ts_start);

    std::thread sendThread(Send_Message, Start_string, channel_send_id, red_send_data, ir_send_data, sample_id, uuid, 100);
    sendThread.detach();
    curl_global_cleanup();
}

void MaxPlot::Get_Mqtt_Message()
{
    // To do ke cha jie
    cout << "MQTT READY" << endl;
    uint32_t red_temp[8] = {0}, ir_temp[8] = {0}, channel_temp[8] = {0};

    for (int i = 0; i < 8; i++)
    {
        red_temp[i] = Queue_Mqtt[i].front();
        Queue_Mqtt[i].pop();

        ir_temp[i] = Queue_Mqtt[i].front();
        Queue_Mqtt[i].pop();
        channel_temp[i] = i;
    }

    QJsonArray redTempArray;
    QJsonArray irTempArray;
    QJsonArray channelArray;

    for (int i = 0; i < 8; i++)
    {
        redTempArray.append(static_cast<int>(red_temp[i]));
        irTempArray.append(static_cast<int>(ir_temp[i]));
        channelArray.append(static_cast<int>(channel_temp[i]));
    }

    // 构建 QJsonObject
    QJsonObject payloadObj;
    payloadObj.insert("channel", channelArray);
    payloadObj.insert("ir", irTempArray);
    payloadObj.insert("red", redTempArray);

    // 转换为 JSON 字符串
    QJsonDocument doc(payloadObj);
    QString payload = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    emit sendMQTTMessage(payload);

    if (Queue_Mqtt[7].size() == 0)
        emit Finish_ALL();

    //{"channel":[0,1,2,3,4,5,6,7],"ir":[10,20,30,40,50,60,70,80],"red":[15,25,35,45,55,65,75,85]}
}

void MaxPlot::onExitButtonClicked()
{
    exitButton->setEnabled(false);

    if (timer->isActive())
    {
        timer->stop();
    }
    if (Mqtt_timer && Mqtt_timer->isActive())
    {
        Mqtt_timer->stop();
    }

    stop_Read_Thread();
    stop_Plot_Timer();
    stop_Mqtt_Thread();

    close();
}

void MaxPlot::handleDataReady(const MaxData &data)
{
    uint32_t temp_red = 0, temp_ir = 0;
    for (int i = 0; i < 8; i++)
    {
        Queue_Mqtt[i].push(data.redData[i]);
        Queue_Mqtt[i].push(data.irData[i]);

        channel_send_id[count_data] = i;
        red_send_data[count_data] = data.redData[i];
        red_send_data[count_data++] = data.irData[i];
        if (i == 0 || i == 2 || i == 4 || i == 6)
        {
            temp_red += data.redData[i];
            temp_ir += data.irData[i];
        }
    }

    temp_red /= 4;
    temp_ir /= 4;
    // cout<<temp_red<<" "<<temp_ir<<endl;
    cout<<"receiving data"<<endl;

    redData_middle.append(static_cast<double>(temp_red));
    irData_middle.append(static_cast<double>(temp_ir));

    red0.append(static_cast<double>(data.redData[1]));
    ir0.append(static_cast<double>(data.irData[1]));

    red1.append(static_cast<double>(data.redData[3]));
    ir1.append(static_cast<double>(data.irData[3]));

    red2.append(static_cast<double>(data.redData[5]));
    ir2.append(static_cast<double>(data.irData[5]));

    red3.append(static_cast<double>(data.redData[7]));
    ir3.append(static_cast<double>(data.irData[7]));

    double elapsedTime = startTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    xData.append(elapsedTime);
}

void MaxPlot::setupPlot()
{
    plot->addGraph();
    plot->graph(0)->setName("Middle_RED");
    plot->graph(0)->setPen(QPen(Qt::red));
    plot->graph(0)->setLineStyle(QCPGraph::lsLine);
    plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));

    plot->addGraph();
    plot->graph(1)->setName("Middle_IR");
    plot->graph(1)->setPen(QPen(Qt::green));
    plot->graph(1)->setLineStyle(QCPGraph::lsLine);
    plot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));

    QList<QColor> colors = {Qt::gray, Qt::blue, Qt::gray, Qt::blue, Qt::cyan, Qt::darkBlue, Qt::yellow, Qt::darkYellow, Qt::magenta, Qt::darkGreen};
    for (int i = 2; i < 10; ++i)
    {
        plot->addGraph();
        if (i % 2 == 0)
        {
            plot->graph(i)->setName(QString("RED_%1").arg(i / 2));
        }
        else
        {
            plot->graph(i)->setName(QString("IR_%1").arg(i / 2));
        }

        plot->graph(i)->setPen(QPen(colors[i % colors.size()]));

        plot->graph(i)->setLineStyle(QCPGraph::lsLine);
        plot->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));
    }

    plot->xAxis->setLabel("Time (s)");
    plot->yAxis->setLabel("Amplitude");

    plot->xAxis->setRange(0, windowSize);
    plot->yAxis->setRange(0, 500000);

    plot->legend->setVisible(true);

    plot->setBackground(Qt::black);
    plot->xAxis->setBasePen(QPen(Qt::white));
    plot->yAxis->setBasePen(QPen(Qt::white));
    plot->xAxis->setTickPen(QPen(Qt::white));
    plot->yAxis->setTickPen(QPen(Qt::white));
    plot->xAxis->setSubTickPen(QPen(Qt::white));
    plot->yAxis->setSubTickPen(QPen(Qt::white));
    plot->xAxis->setLabelColor(Qt::white);
    plot->yAxis->setLabelColor(Qt::white);
    plot->xAxis->setTickLabelColor(Qt::white);
    plot->yAxis->setTickLabelColor(Qt::white);
}

void MaxPlot::setupGestures()
{
    grabGesture(Qt::PinchGesture);
}

bool MaxPlot::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
    {
        return gestureEvent(static_cast<QGestureEvent *>(event));
    }

    return QMainWindow::event(event);
}

bool MaxPlot::gestureEvent(QGestureEvent *event)
{
    if (QGesture *pinch = event->gesture(Qt::PinchGesture))
    {
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    }
    return true;
}

void MaxPlot::setupSlider()
{
    slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(0, 0);
    slider->setEnabled(false);
    slider->setStyleSheet("background-color:white;");
    connect(slider, &QSlider::valueChanged, this, &MaxPlot::onSliderMoved);
    connect(slider, &QSlider::sliderPressed, this, &MaxPlot::onSliderPressed);
    connect(slider, &QSlider::sliderReleased, this, &MaxPlot::onSliderReleased);
    isView = false;
}

void MaxPlot::onSliderPressed()
{
    isView = true;
}

void MaxPlot::onSliderReleased()
{
    int sliderMax = slider->maximum();
    int position = slider->value();
    isView = false;
}

void MaxPlot::onSliderMoved(int position)
{
    if (isView)
    {
        plot->xAxis->setRange(position, position + windowSize);
        plot->replot();
    }
}

void MaxPlot::pinchTriggered(QPinchGesture *gesture)
{
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    if (changeFlags & QPinchGesture::ScaleFactorChanged)
    {
        qreal scaleFactor = gesture->scaleFactor();

        plot->xAxis->scaleRange(scaleFactor, plot->xAxis->range().center());
        plot->yAxis->scaleRange(scaleFactor, plot->yAxis->range().center());

        plot->replot();
    }
}

void MaxPlot::updatePlot()
{
    cout << "Start to Plot" << endl;
    double elapsedTime = startTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;

    plot->graph(0)->setData(xData, redData_middle);
    plot->graph(1)->setData(xData, irData_middle);

    plot->graph(2)->setData(xData, red0);
    plot->graph(3)->setData(xData, ir0);

    plot->graph(4)->setData(xData, red1);
    plot->graph(5)->setData(xData, ir1);

    plot->graph(6)->setData(xData, red2);
    plot->graph(7)->setData(xData, ir2);

    plot->graph(8)->setData(xData, red3);
    plot->graph(9)->setData(xData, ir3);

    sampleCount++;

    if (elapsedTime <= windowSize)
    {
        plot->xAxis->setRange(0, windowSize);
        slider->setRange(0, windowSize);
        slider->setEnabled(false);
    }
    else
    {
        if (!slider->isEnabled())
        {
            slider->setEnabled(true);
        }
        double maxSliderValue = elapsedTime - windowSize;

        if (!slider->isSliderDown())
        {
            slider->setRange(0, static_cast<int>(elapsedTime - windowSize));

            if (!isView)
            {

                slider->setValue(static_cast<int>(maxSliderValue));
                plot->xAxis->setRange(elapsedTime - windowSize, elapsedTime);
            }
            plot->xAxis->setRange(elapsedTime - windowSize, elapsedTime);
        }
    }
    plot->replot();
}

void MaxPlot::closeEvent(QCloseEvent *event)
{
    emit windowClosed();
    QMainWindow::closeEvent(event);
}