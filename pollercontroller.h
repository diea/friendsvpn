#ifndef POLLERCONTROLLER_H
#define POLLERCONTROLLER_H

#include <QObject>
#include <QThread>
#include "poller.h"

class PollerController : public QObject
{
    Q_OBJECT
    QThread pollerThread;
public:
    explicit PollerController(BonjourSQL* qSql, QObject *parent = 0);

signals:
    void operate();
public slots:

};

#endif // POLLERCONTROLLER_H
