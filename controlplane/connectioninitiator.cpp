#include "connectioninitiator.h"
ConnectionInitiator::ConnectionInitiator(BonjourSQL* qSql, QObject *parent) :
    QObject(parent)
{
    this->qSql = qSql;
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
