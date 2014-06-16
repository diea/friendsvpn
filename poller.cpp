#include "poller.h"

Poller::Poller(QObject *parent) :
    QObject(parent)
{
    this->qSql = DatabaseHandler::getInstance();
    server = new XmlRPCTextServer(this);
    server->addMethod("setUid", this, "setUid");
}

void Poller::run() {
    QTimer::singleShot(2000, this, SLOT(fetchXmlRPC()));
}

void Poller::fetchXmlRPC() {
    QString fetched = qSql->fetchXmlRpc();
    if (fetched != NULL)
        server->newConnection(fetched);
    this->run(); // call has returned, call run again !
}

bool Poller::setUid(QString uid) {
    qSql->uid = uid;
    return true;
}

