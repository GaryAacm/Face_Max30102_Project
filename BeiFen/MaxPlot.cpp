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
#include <QQueue>
using namespace std;
using namespace nlohmann;


MaxPlot::MaxPlot(QWidget *parent, QRCodeGenerator *qrGenerator)
    : QMainWindow(parent), plot(new QCustomPlot(this)), logo(new QLabel(this)),
      sampleCount(0), windowSize(50), isTouching(false), qr(qrGenerator)
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

    exitButton = new QPushButton("Exit", this);
    exitButton->setFixedSize(100, 40);
    exitButton->setStyleSheet("background-color:white;");
    connect(exitButton, &QPushButton::clicked, this, &MaxPlot::onExitButtonClicked);

    button1 = new QPushButton("fun1", this);
    button1->setFixedSize(100, 40);
    button1->setStyleSheet("background-color:white;");

    button2 = new QPushButton("fun2", this);
    button2->setFixedSize(100, 40);
    button2->setStyleSheet("background-color:white;");

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(exitButton);
    topLayout->addWidget(button1);
    topLayout->addWidget(button2);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(slider);

    mainLayout->addWidget(logo, 0);
    mainLayout->addWidget(plot, 1);

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(bottomLayout, 0);

    setCentralWidget(centralWidget);

    startTime = QDateTime::currentDateTime();

    worker = new MaxDataWorker();
    workerThread = new QThread();

    worker->moveToThread(workerThread);

    // 连接信号和槽
    connect(workerThread, &QThread::started, worker, &MaxDataWorker::startWork);
    connect(worker, &MaxDataWorker::dataReady, this, &MaxPlot::handleDataReady);
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);

    qRegisterMetaType<MaxData>("MaxData");

    // 启动线程
    workerThread->start();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MaxPlot::updatePlot);
    timer->start(33);

    setAttribute(Qt::WA_AcceptTouchEvents);

    // Get start time stamp
    Start_TimeStamp = time(nullptr);

    showFullScreen();
}

MaxPlot::~MaxPlot()
{
}

void MaxPlot::onExitButtonClicked()
{
    exitButton->setEnabled(false);

    if (timer->isActive())
    {
        timer->stop();
    }
    // Stop the worker safely
    QMetaObject::invokeMethod(worker, "stopWork", Qt::BlockingQueuedConnection);

    // Clean up the worker thread
    workerThread->quit();
    workerThread->wait();
    delete workerThread;

    close();
}

void Send_Message(const std::string &Start_Unix, const std::string &End_Unix, const int *Channel_ID, const int *Red_Data, const int *IR_Data, size_t Data_Size, const std::string &sample_id, const std::string &uuid)
{
    CURL *curl = curl_easy_init();
    if (curl)
    {
        // 构建 JSON 对象
        json jsonData;
        jsonData["Start_Unix"] = Start_Unix;
        jsonData["End_Unix"] = End_Unix;

        jsonData["channel_id"] = std::vector<int>(Channel_ID, Channel_ID + Data_Size);
        jsonData["data"]["ir"] = std::vector<int>(IR_Data, IR_Data + Data_Size);
        jsonData["data"]["reds"] = std::vector<int>(Red_Data, Red_Data + Data_Size);

        cout << "Data size is :" << Data_Size;

        jsonData["sample_id"] = sample_id;
        jsonData["user_uuid"] = uuid;

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


void MaxPlot::handleDataReady(const MaxData &data)
{
    double elapsedTime = startTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;


    redData_middle.append(static_cast<double>(data.middle_red));
    irData_middle.append(static_cast<double>(data.middle_ir));
    channel_send_id[count_data] = 0;
    red_send_data[count_data] = data.middle_red;
    red_send_data[count_data++] = data.middle_ir;

    red0.append(static_cast<double>(data.redData1));
    ir0.append(static_cast<double>(data.irData1));
    channel_send_id[count_data] = 1;
    red_send_data[count_data] = data.redData1;
    red_send_data[count_data++] = data.irData1;

    red1.append(static_cast<double>(data.redData3));
    ir1.append(static_cast<double>(data.irData3));
    channel_send_id[count_data] = 3;
    red_send_data[count_data] = data.redData3;
    red_send_data[count_data++] = data.irData3;

    red2.append(static_cast<double>(data.redData5));
    ir2.append(static_cast<double>(data.irData5));
    channel_send_id[count_data] = 5;
    red_send_data[count_data] = data.redData5;
    red_send_data[count_data++] = data.irData5;

    red3.append(static_cast<double>(data.redData7));
    ir3.append(static_cast<double>(data.irData7));
    channel_send_id[count_data] = 7;
    red_send_data[count_data] = data.redData7;
    red_send_data[count_data++] = data.irData7;

    //count_data += 8;
    xData.append(elapsedTime);

    End_TimeStamp = time(nullptr);
    if (End_TimeStamp - Start_TimeStamp >= 1)
    {
        // Send Http
        cout<<"Collect data is:"<<count_data<<endl;
        CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (res != CURLE_OK)
        {
            std::cout << "curl_global_init() 失败: " << curl_easy_strerror(res) << std::endl;
        }

        string userdata = qr->user_message;
        size_t pos = userdata.find(',');
        string sample_id = userdata.substr(0, pos);
        string uuid = userdata.substr(pos + 1);

        long ts_start = static_cast<long>(Start_TimeStamp);
        long ts_end = static_cast<long>(End_TimeStamp);
        string Start_string = to_string(ts_start);
        string End_string = to_string(ts_end);

        std::thread sendThread(Send_Message, Start_string, End_string, channel_send_id, red_send_data, ir_send_data, count_data, sample_id, uuid);
        sendThread.detach();
        curl_global_cleanup();

        Start_TimeStamp = End_TimeStamp;
        count_data = 0;
    }
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
    plot->yAxis->setRange(0, 200000);

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

    // xData.clear();
    // redData_middle.clear();
    // irData_middle.clear();
    // red0.clear();
    // ir0.clear();
    // red1.clear();
    // ir1.clear();
    // red2.clear();
    // ir2.clear();
    // red3.clear();
    // ir3.clear();

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