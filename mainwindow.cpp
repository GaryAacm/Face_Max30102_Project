#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QDebug>
#include <QPixmap>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), plot(new QCustomPlot(this)), logo(new QLabel(this)),
      sampleCount(0), windowSize(50), isTouching(false), max30102("/dev/i2c-4")
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
    connect(exitButton, &QPushButton::clicked, this, &MainWindow::onExitButtonClicked);

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
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updatePlot);
    timer->start(33);
    setAttribute(Qt::WA_AcceptTouchEvents);

    showFullScreen();
}

MainWindow::~MainWindow()
{
}

void MainWindow::onExitButtonClicked()
{
    close();
}

void MainWindow::BeginRun()
{
    timer->start(5);
}

void MainWindow::StopRun()
{
    timer->stop();
}

void MainWindow::setupPlot()
{

    for (int i = 0; i < 10; ++i)
    {
        plot->addGraph();
        QString graphName = (i % 2 == 0) ? QString("Sensor %1 Red").arg(i / 2 + 1) : QString("Sensor %1 IR").arg(i / 2 + 1);
        plot->graph(i)->setName(graphName);

        QPen pen;
        pen.setColor(QColor::fromHsv((i * 36) % 360, 255, 255)); // 根据HSV色调变化，设置不同颜色
        plot->graph(i)->setPen(pen);
        plot->graph(i)->setLineStyle(QCPGraph::lsLine);
        plot->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));

    }

    plot->xAxis->setLabel("Time (s)");
    plot->yAxis->setLabel("Amplitude");

    plot->xAxis->setRange(0, windowSize);
    plot->yAxis->setRange(0, 300000);

    plot->legend->setVisible(false); // 关闭默认图例

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

void MainWindow::setupGestures()
{
    grabGesture(Qt::PinchGesture);
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
    {
        return gestureEvent(static_cast<QGestureEvent *>(event));
    }
    else if (event->type() == QEvent::TouchBegin || event->type() == QEvent::TouchUpdate || event->type() == QEvent::TouchEnd)
    {
        touchEvent(static_cast<QTouchEvent *>(event));
        return true;
    }
    return QMainWindow::event(event);
}

bool MainWindow::gestureEvent(QGestureEvent *event)
{
    if (QGesture *pinch = event->gesture(Qt::PinchGesture))
    {
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    }
    return true;
}

void MainWindow::touchEvent(QTouchEvent *event)
{
    if (event->touchPoints().count() == 1)
    {
        QTouchEvent::TouchPoint touchPoint = event->touchPoints().first();

        if (event->type() == QEvent::TouchBegin)
        {
            lastTouchPos = touchPoint.pos().toPoint();
            isTouching = true;
        }
        else if (event->type() == QEvent::TouchUpdate && isTouching)
        {
            int dx = touchPoint.pos().x() - lastTouchPos.x();
            double xRangeSize = plot->xAxis->range().size();
            double xStep = dx * (xRangeSize / plot->width());

            plot->xAxis->moveRange(-xStep);

            lastTouchPos = touchPoint.pos().toPoint();
            plot->replot();
        }
        else if (event->type() == QEvent::TouchEnd)
        {
            isTouching = false;
        }
    }
}

void MainWindow::setupSlider()
{
    slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(0, 0);
    slider->setEnabled(false);
    slider->setStyleSheet("background-color:white;");
    connect(slider, &QSlider::valueChanged, this, &MainWindow::onSliderMoved);
}

void MainWindow::pinchTriggered(QPinchGesture *gesture)
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

void MainWindow::updatePlot()
{
    uint32_t red, ir;
    max30102.get_middle_data(&red, &ir);
    redData[middle_sensor].append(static_cast<double>(red));
    irData[middle_sensor].append(static_cast<double>(ir));

    for (int sensorIndex = 1; sensorIndex < 5; ++sensorIndex)
    {
        max30102.get_branch_data(&red, &ir);
        redData[sensorIndex].append(static_cast<double>(red));
        irData[sensorIndex].append(static_cast<double>(ir));
    }

    sampleCount++;

    double elapsedTime = startTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    xData.append(elapsedTime);

    // 更新10条曲线的数据
    for (int i = 0; i < 5; ++i)
    {
        plot->graph(2 * i)->setData(xData, redData[i]);    // 红光数据
        plot->graph(2 * i + 1)->setData(xData, irData[i]); // 红外数据
    }

    if (elapsedTime <= windowSize)
    {
        plot->xAxis->setRange(0, windowSize);
        slider->setRange(0, windowSize);
    }
    else
    {
        if (!slider->isSliderDown())
        {
            int maxSliderValue = elapsedTime;
            slider->setRange(0, 10);
            slider->setValue(maxSliderValue - windowSize);
        }
        plot->xAxis->setRange(elapsedTime - windowSize, elapsedTime);
    }
    plot->replot();
}

void MainWindow::onSliderMoved(int position)
{
    plot->xAxis->setRange(position, position + windowSize);
    plot->replot();
}