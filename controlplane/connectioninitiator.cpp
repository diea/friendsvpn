#include "connectioninitiator.h"
#include "controlplaneclient.h"
#include "controlplaneserver.h"

ConnectionInitiator* ConnectionInitiator::instance = NULL;

ConnectionInitiator::ConnectionInitiator(QObject *parent) :
    QObject(parent)
{
    this->qSql = BonjourSQL::getInstance();
}

ConnectionInitiator* ConnectionInitiator::getInstance() {
    static QMutex mutex;
    mutex.lock();
    if (instance == NULL) {
        instance = new ConnectionInitiator();
    }
    mutex.unlock();
    return instance;
}

void ConnectionInitiator::run() {
    cert = qSql->getLocalCert();
    key = qSql->getMyKey();

    // start the server
    this->startServer();
    // start the clients
    this->startClients();
}

void ConnectionInitiator::startServer() {
    server = new ControlPlaneServer(cert, key, QHostAddress::AnyIPv6, 61323, this);
    server->start();
    //threadedServ = new ThreadedCpServer(cert, key, QHostAddress::AnyIPv6, 61323, this);
    //threadedServ->start();
}

void ConnectionInitiator::startClients() {
    QList< User* > friends = qSql->getFriends();

    foreach(User* frien_d, friends) {
        ControlPlaneClient* c = new ControlPlaneClient(*(frien_d->cert), key, QHostAddress(*(frien_d->ipv6)),
                                                       61323, this);
        // TODO delete user?
        c->run();
        clients.append(c);
    }
}

ControlPlaneConnection* ConnectionInitiator::getConnection(QString uid) {
    static QMutex mutex;
    mutex.lock();
    QListIterator< ControlPlaneConnection* > i(instance->connections);
    while (i.hasNext()) {
        ControlPlaneConnection* nxt = i.next();
        if (nxt->getUid() == uid) {
            mutex.unlock();
            return nxt;
        }
    }
    // doesn't exist, create a new one
    ControlPlaneConnection* newCon = new ControlPlaneConnection(uid);
    instance->connections.append(newCon);
    mutex.unlock();
    return newCon;
}

QString ConnectionInitiator::getMyUid() {
    return instance->qSql->getLocalUid();
}















