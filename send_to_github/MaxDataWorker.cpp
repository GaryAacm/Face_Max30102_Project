#include "MaxDataWorker.h"
#include "max30102.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

using namespace std;
using namespace chrono;

void timer_handler(int sig, siginfo_t *si, void *uc)
{

    MAX30102 *data = (MAX30102 *)si->si_value.sival_ptr;
    data->get_data();
}

MaxDataWorker::MaxDataWorker(QObject *parent, MAX30102 *sensor)
    : QObject(parent), max30102(sensor)
{
}

MaxDataWorker::~MaxDataWorker()
{
}

void MaxDataWorker::doWork()
{
    struct sigevent sev;
    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGRTMIN, &sa, NULL);

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = max30102;

    timer_t timerid;
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
    {
        perror("timer_create");
    }

    struct itimerspec its;
    its.it_value.tv_sec = 0;        
    its.it_value.tv_nsec = 10000000; 
    its.it_interval.tv_sec = 0;      
    its.it_interval.tv_nsec = 10000000;

    if (timer_settime(timerid, 0, &its, NULL) == -1)
    {
        perror("timer_settime");
    }

    sleep(5);

    timer_delete(max30102);

    emit finishRead();
}
