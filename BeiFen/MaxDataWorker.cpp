#include "MaxDataWorker.h"
#include "max30102.h"
#include <iostream>

using namespace std;

MaxDataWorker::MaxDataWorker(QObject *parent) : QObject(parent)
{
    max30102 = new MAX30102("/dev/i2c-4");

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MaxDataWorker::doWork);
}

MaxDataWorker::~MaxDataWorker()
{
    max30102->Quit();
    delete max30102;
}

void MaxDataWorker::startWork()
{
    timer->start(0);
}

void MaxDataWorker::stopWork()
{
    timer->stop();
}

void MaxDataWorker::doWork()
{
    MaxData data;
    uint32_t red[8] = {0}, ir[8] = {0};
    uint32_t middle_red_data = 0, middle_ir_data = 0;
    // 数据采集函数
    max30102->get_middle_data(&middle_red_data, &middle_ir_data);
    max30102->get_branch_data(red, ir);
    data.middle_red = middle_red_data;
    data.middle_ir = middle_ir_data;

    data.redData1 = red[1];
    data.redData3 = red[3];
    data.redData5 = red[5];
    data.redData7 = red[7];

    data.irData1 = ir[1];
    data.irData3 = ir[3];
    data.irData5 = ir[5];
    data.irData7 = ir[7];
    
    emit dataReady(data);
}
