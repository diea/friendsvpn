#include "poller.h"

Poller::Poller(BonjourSQL* qSql, QObject *parent) :
    QObject(parent)
{
    this->qSql = qSql;
    server = new XmlRPCTextServer(this);
    server->addMethod("setUid", this, "setUid");
}

void Poller::run() {
    //QTimer *timer = new QTimer(this);
    //connect(timer, SIGNAL(timeout()), this, SLOT(fetchXmlRPC()));
    QTimer::singleShot(2000, this, SLOT(fetchXmlRPC()));
    //timer->start(500);
    //fetchXmlRPC();
    qDebug() << QThread::currentThreadId();
}

void Poller::fetchXmlRPC() {
    qDebug() << QThread::currentThreadId();

    qDebug() << "fetching and current time : " << QDateTime::currentMSecsSinceEpoch();
    QString fetched = qSql->fetchXmlRpc();
    if (fetched != NULL)
        server->newConnection(fetched);
    this->run();
}

bool Poller::setUid(QString uid) {
    qDebug() << "uid is " << uid;
    qSql->uid = uid;
    return true;
}
