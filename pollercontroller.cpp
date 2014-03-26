#include "pollercontroller.h"

PollerController::PollerController(BonjourSQL* qSql, QObject *parent) :
    QObject(parent)
{
    Poller* poller = new Poller(qSql);
    poller->moveToThread(&pollerThread);
    connect(this, SIGNAL(operate()), poller, SLOT(run()));
    pollerThread.start();
}
