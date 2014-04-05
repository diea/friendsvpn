#include "connectioninitiator.h"
#include "controlplaneclient.h"
#include "controlplaneserver.h"

ConnectionInitiator* ConnectionInitiator::instance = NULL;

ConnectionInitiator::ConnectionInitiator(BonjourSQL* qSql, QObject *parent) :
    QObject(parent)
{
    this->qSql = qSql;
}

ConnectionInitiator* ConnectionInitiator::getInstance(BonjourSQL* qSql = 0) {
    if ((instance == NULL) && (qSql != 0)) {
        return new ConnectionInitiator(qSql);
    } else {
        return NULL;
    }
}

ConnectionInitiator* ConnectionInitiator::getInstance() {
    if (instance != NULL)
        return instance;
    else
        return NULL;
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

void ConnectionInitiator::addControlPlaneConnection(ControlPlaneConnection *con) {
    connections.append(con);
}

ControlPlaneConnection* ConnectionInitiator::getConnection(QString uid) {
    QListIterator< ControlPlaneConnection* > i(connections);
    while (i.hasNext()) {
        ControlPlaneConnection* nxt = i.next();
        if (nxt->getUid() == uid) {
            return nxt;
        }
    }
    return NULL;
}

QString ConnectionInitiator::getMyUid() {
    return qSql->getLocalUid();
}















