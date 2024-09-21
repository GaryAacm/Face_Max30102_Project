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

    generateQRButton->setFixedSize(200, 50);
    startButton->setFixedSize(200, 50);
    exitButton->setFixedSize(200, 50);

    // mainLayout->addStretch();

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(generateQRButton);
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(exitButton);

    buttonLayout->setSpacing(20);
    buttonLayout->setContentsMargins(20, 10, 20, 20);
    mainLayout->addLayout(buttonLayout);

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
    QProcess *pythonProcess = new QProcess(this);
    QString pythonScript = "QRcode.py";
    QStringList arguments;
    arguments << pythonScript;

    pythonProcess->start("python3", arguments);
}

void MainWindow::RunMax30102(const std::string &command)
{
    int result = system(command.c_str());
    if (result == 0)
    {
        std::cout << "Command executed successfully." << std::endl;
    }
    else
    {
        std::cerr << "Command execution failed with error code: " << result << std::endl;
    }
}

void MainWindow::onStartClicked()
{
    RunMax30102("./read_max30102");
}

void MainWindow::onExitClicked()
{
    sendExitMessage();
    close();
}
