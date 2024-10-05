#include "mainwindow.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QUrl>
#include <QMessageBox>
#include <QDebug>
#include <QApplication>
#include <QPainter>
#include <QDir>
#include <iostream>
#include <string>
#include <curl/curl.h>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      generateQRButton(new QPushButton("生成二维码")),
      startButton(new QPushButton("开始")),
      exitButton(new QPushButton("退出"))
{
    setWindowTitle("皮瓣移植工具");

    QVBoxLayout *mainLayout = new QVBoxLayout;

    generateQRButton->setStyleSheet("background:transparent;color:black;border:none;font-weight:bold;font-size:50px;");
    startButton->setStyleSheet("background:transparent;color:black;border:none;font-weight:bold;font-size:50px;");
    exitButton->setStyleSheet("background:transparent;color:black;border:none;font-size:50px;");

    maxPlotWindow = nullptr;

    generateQRButton->setFixedSize(300, 100);
    startButton->setFixedSize(300, 100);
    exitButton->setFixedSize(300, 100);

    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(generateQRButton);
    buttonLayout->addSpacing(50);
    buttonLayout->addWidget(startButton);
    buttonLayout->addSpacing(50);
    buttonLayout->addWidget(exitButton);

    buttonLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *buttonWidget = new QWidget;
    buttonWidget->setLayout(buttonLayout);

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addStretch(5); 
    topLayout->addWidget(buttonWidget);
    topLayout->addStretch(2);


    mainLayout->addStretch(3);
    mainLayout->addLayout(topLayout);
    mainLayout->addStretch(7); 

    setLayout(mainLayout);

    QString imagePath = "/home/orangepi/Desktop/zjj/background.png";
    if (!backgroundPixmap.load(imagePath))
    {
        qDebug() << "无法加载背景图片:" << imagePath;
    }

    connect(generateQRButton, &QPushButton::clicked, this, &MainWindow::onGenerateQRClicked);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(exitButton, &QPushButton::clicked, this, &MainWindow::onExitClicked);

    showFullScreen();
}

MainWindow::~MainWindow()
{
    if (maxPlotWindow)
    {
        delete maxPlotWindow;
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if (!backgroundPixmap.isNull())
    {
        painter.drawPixmap(0, 0, width(), height(), backgroundPixmap);
    }
    QWidget::paintEvent(event);
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

void MainWindow::sendExitMessage()
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        std::string base_url = "http://example.com/api";

        // 要发送的字符串数据
        std::string data = "Exit";

        char *encoded_data = curl_easy_escape(curl, data.c_str(), data.length());
        if (encoded_data)
        {
            std::string url = base_url + "?data=" + encoded_data;

            curl_free(encoded_data);

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK)
            {
                std::cerr << "curl_easy_perform() 失败: " << curl_easy_strerror(res) << std::endl;
            }
            else
            {

                std::cout << "服务器返回的数据: " << readBuffer << std::endl;
            }
        }
        else
        {
            std::cerr << "URL 编码失败" << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    else
    {
        std::cerr << "初始化 libcurl 失败" << std::endl;
    }
}

void MainWindow::onGenerateQRClicked()
{
    User_Message = qr.generateAndSendUserMessage();
}


void MainWindow::onStartClicked()
{
    if (!maxPlotWindow)
    {
        maxPlotWindow = new MaxPlot();

        // 连接 MaxPlot 的关闭信号到 MainWindow 的槽函数
        connect(maxPlotWindow, &MaxPlot::windowClosed, this, [this]()
                {
            // 在窗口关闭后，重置指针
            maxPlotWindow = nullptr; });

        maxPlotWindow->show();
    }
    else
    {
        maxPlotWindow->show();
        maxPlotWindow->raise();
        maxPlotWindow->activateWindow();
    }
}

void MainWindow::onExitClicked()
{
    sendExitMessage();
    close();
}
