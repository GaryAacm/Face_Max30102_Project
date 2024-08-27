#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QDebug>
#include <QPixmap>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), plotTop(new QCustomPlot(this)), plotBottom(new QCustomPlot(this)), logo(new QLabel(this)),
      sampleCount(0), windowSize(50), isTouching(false)
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
    // connect(button1, &QPushButton::pressed, this, &MainWindow::StopRun);
    // connect(button1, &QPushButton::released, this, &MainWindow::BeginRun);

    button2 = new QPushButton("fun2", this);
    button2->setFixedSize(100, 40);
    button2->setStyleSheet("background-color:white;");
    // connect(button2, &QPushButton::pressed, this, &MainWindow::onRightButtonClicked);
    // connect(button2, &QPushButton::released, this, &MainWindow::onRightButtonReleased);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(exitButton);
    topLayout->addWidget(button1);
    topLayout->addWidget(button2);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(slider);

    mainLayout->addWidget(logo, 0);
    mainLayout->addWidget(plotTop, 1);
    mainLayout->addWidget(plotBottom, 1);

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(bottomLayout, 0);

    setCentralWidget(centralWidget);

    channels = {1, 3};

    startTime = QDateTime::currentDateTime();
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updatePlot);
    timer->start(5);
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

// void MainWindow::onLeftButtonClicked() {
//     timer->stop();
//     int position = slider->value();
//     int step = 1;

//     if (position - step >= 0) {
//         position -= step;
//         slider->setValue(position);
//         plotTop->xAxis->setRange(position, position + windowSize);
//         plotBottom->xAxis->setRange(position, position + windowSize);
//         plotTop->replot();
//         plotBottom->replot();
//     }
// }

// void MainWindow::onRightButtonClicked() {
//     timer->stop();
//     int position = slider->value();
//     int step = 1;

//     if (position + step + windowSize <= xData.last()) {
//         position += step;
//         slider->setValue(position);
//         plotTop->xAxis->setRange(position, position + windowSize);
//         plotBottom->xAxis->setRange(position, position + windowSize);
//         plotTop->replot();
//         plotBottom->replot();
//     }
// }

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
    plotTop->addGraph();
    plotTop->graph(0)->setName("Red");
    plotTop->graph(0)->setPen(QPen(Qt::red));
    plotTop->graph(0)->setLineStyle(QCPGraph::lsLine);
    plotTop->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));

    plotTop->addGraph();
    plotTop->graph(1)->setName("IR");
    plotTop->graph(1)->setPen(QPen(Qt::green));
    plotTop->graph(1)->setLineStyle(QCPGraph::lsLine);
    plotTop->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));

    plotTop->xAxis->setLabel("Time (s)");
    plotTop->yAxis->setLabel("Amplitude");

    plotTop->xAxis->setRange(0, windowSize);
    plotTop->yAxis->setRange(0, 300000);

    plotTop->legend->setVisible(true);

    plotTop->setBackground(Qt::black);
    plotTop->xAxis->setBasePen(QPen(Qt::white));
    plotTop->yAxis->setBasePen(QPen(Qt::white));
    plotTop->xAxis->setTickPen(QPen(Qt::white));
    plotTop->yAxis->setTickPen(QPen(Qt::white));
    plotTop->xAxis->setSubTickPen(QPen(Qt::white));
    plotTop->yAxis->setSubTickPen(QPen(Qt::white));
    plotTop->xAxis->setLabelColor(Qt::white);
    plotTop->yAxis->setLabelColor(Qt::white);
    plotTop->xAxis->setTickLabelColor(Qt::white);
    plotTop->yAxis->setTickLabelColor(Qt::white);

    plotBottom->addGraph();
    plotBottom->graph(0)->setName("Red");
    plotBottom->graph(0)->setPen(QPen(Qt::red));
    plotBottom->graph(0)->setLineStyle(QCPGraph::lsLine);
    plotBottom->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));

    plotBottom->addGraph();
    plotBottom->graph(1)->setName("IR");
    plotBottom->graph(1)->setPen(QPen(Qt::green));
    plotBottom->graph(1)->setLineStyle(QCPGraph::lsLine);
    plotBottom->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));

    plotBottom->xAxis->setLabel("Time (s)");
    plotBottom->yAxis->setLabel("Amplitude");

    plotBottom->xAxis->setRange(0, windowSize);
    plotBottom->yAxis->setRange(0, 300000);

    plotBottom->legend->setVisible(true);
    plotBottom->setBackground(Qt::black);
    plotBottom->xAxis->setBasePen(QPen(Qt::white));
    plotBottom->yAxis->setBasePen(QPen(Qt::white));
    plotBottom->xAxis->setTickPen(QPen(Qt::white));
    plotBottom->yAxis->setTickPen(QPen(Qt::white));
    plotBottom->xAxis->setSubTickPen(QPen(Qt::white));
    plotBottom->yAxis->setSubTickPen(QPen(Qt::white));
    plotBottom->xAxis->setLabelColor(Qt::white);
    plotBottom->yAxis->setLabelColor(Qt::white);
    plotBottom->xAxis->setTickLabelColor(Qt::white);
    plotBottom->yAxis->setTickLabelColor(Qt::white);
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
            double xRangeSize = plotTop->xAxis->range().size();
            double xStep = dx * (xRangeSize / plotTop->width());

            plotTop->xAxis->moveRange(-xStep);
            plotBottom->xAxis->moveRange(-xStep);

            lastTouchPos = touchPoint.pos().toPoint();
            plotTop->replot();
            plotBottom->replot();
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

        plotTop->xAxis->scaleRange(scaleFactor, plotTop->xAxis->range().center());
        plotTop->yAxis->scaleRange(scaleFactor, plotTop->yAxis->range().center());

        plotBottom->xAxis->scaleRange(scaleFactor, plotBottom->xAxis->range().center());
        plotBottom->yAxis->scaleRange(scaleFactor, plotBottom->yAxis->range().center());

        plotTop->replot();
        plotBottom->replot();
    }
}

void MainWindow::updatePlot()
{
    uint32_t red, ir;
    const char *i2c_device = "/dev/i2c-4";
    MAX30102 max30102(i2c_device);
    max30102.PreJob();

    for (int j = 0; j < 30; j++)
    {
        max30102.DoJob(&red, &ir);
        usleep(5000);

        if (red < 3500 || ir < 3500)
            continue;

        redData.append(static_cast<double>(red));
        irData.append(static_cast<double>(ir));
        sampleCount++;

        double elapsedTime = startTime.msecsTo(QDateTime::currentDateTime()) / 1000.0;
        xData.append(elapsedTime);

        plotTop->graph(0)->setData(xData, redData);
        plotTop->graph(1)->setData(xData, irData);

        plotBottom->graph(0)->setData(xData, redData);
        plotBottom->graph(1)->setData(xData, irData);

        if (elapsedTime <= windowSize)
        {
            plotTop->xAxis->setRange(0, windowSize);
            plotBottom->xAxis->setRange(0, windowSize);
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
            plotTop->xAxis->setRange(elapsedTime - windowSize, elapsedTime);
            plotBottom->xAxis->setRange(elapsedTime - windowSize, elapsedTime);
        }
        plotTop->replot();
        plotBottom->replot();
    }
}

void MainWindow::onSliderMoved(int position)
{
    plotTop->xAxis->setRange(position, position + windowSize);
    plotBottom->xAxis->setRange(position, position + windowSize);
    plotTop->replot();
    plotBottom->replot();
}
