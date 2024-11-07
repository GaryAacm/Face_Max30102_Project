#ifndef MAXDATAWORKER_H
#define MAXDATAWORKER_H

#include <QObject>
#include <QVector>
#include <QTimer>
#include <QQueue>
#include "max30102.h"
#include <chrono>

struct MaxData
{
    uint32_t middle_red;
    uint32_t middle_ir;
    uint32_t redData1, redData3, redData5, redData7; 
    uint32_t irData1, irData3, irData5, irData7;

};

Q_DECLARE_METATYPE(MaxData)

class MaxDataWorker : public QObject
{
    Q_OBJECT

public:
    explicit MaxDataWorker(QObject *parent = nullptr);
    ~MaxDataWorker();
    void startWork();
    void stopWork();
    

signals:
    void dataReady(const MaxData &data);

private slots :
    void doWork();
    

private:
    QTimer *timer;
    MAX30102 *max30102;
};

#endif // MAXDATAWORKER_H
