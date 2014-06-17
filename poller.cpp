#include "poller.h"
#include "connectioninitiator.h"
#include "controlplane/controlplaneconnection.h"
#include <QMutex>

Poller::Poller(QObject *parent) :
    QObject(parent)
{
    this->qSql = DatabaseHandler::getInstance();
    server = new XmlRPCTextServer(this);
    server->addMethod("setUid", this, "setUid");
    server->addMethod("emitBonjourChanged", this, "emitBonjourChanged");
}

Poller* Poller::instance = NULL;

Poller* Poller::getInstance() {
    static QMutex mutex;
    mutex.lock();
    if (!instance)
        instance = new Poller();
    mutex.unlock();
    return instance;
}

void Poller::run() {
    QTimer::singleShot(2000, this, SLOT(fetchXmlRPC()));
}

void Poller::fetchXmlRPC() {
    //qDebug() << "Fetching XML RPC";
    QString fetched = qSql->fetchXmlRpc();
    if (fetched != NULL)
        server->newConnection(fetched);
    this->run(); // call has returned, call run again !
}

bool Poller::setUid(QString uid) {
    qSql->uid = uid;
    return true;
}

void Poller::emitBonjourChanged() {
    emit bonjourChanged();
}
