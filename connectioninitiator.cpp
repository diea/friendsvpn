#include "connectioninitiator.h"
#include "controlplane/controlplaneclient.h"
#include "controlplane/controlplaneserver.h"
#include "controlplane/controlplaneconnection.h"

#include "dataplane/dataplaneserver.h"
#include "dataplane/dataplaneclient.h"
#include "dataplane/dataplaneconnection.h"

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
    server = new ControlPlaneServer(cert, key, QHostAddress::AnyIPv6, CONTROLPLANELISTENPORT, this);
    server->start();

    dpServer = DataPlaneServer::getInstance();
    dpServer->moveToThread(&dpServerThread);
    connect(&dpServerThread, SIGNAL(started()), dpServer, SLOT(start()));
    dpServerThread.start();
}

void ConnectionInitiator::startClients() {
    QList< User* > friends = qSql->getFriends();

    foreach(User* frien_d, friends) {
        ControlPlaneClient* c = new ControlPlaneClient(*(frien_d->cert), key, QHostAddress(*(frien_d->ipv6)),
                                                       CONTROLPLANELISTENPORT, this);
        c->run();
        clients.append(c);

        QThread* dcThread = new QThread(); // dataplane is threaded
        qDebug() << QHostAddress(*(frien_d->ipv6));

        DataPlaneConnection* con = this->getDpConnection(QString(*(frien_d->uid)));
        DataPlaneClient* dc = new DataPlaneClient(QHostAddress(*(frien_d->ipv6)), con);
        connect(dcThread, SIGNAL(started()), dc, SLOT(run()));
        connect(dcThread, SIGNAL(finished()), dcThread, SLOT(deleteLater()));
        connect(dc, SIGNAL(bufferReady(const char*, int)), con, SLOT(readBuffer(const char*, int)));

        dc->moveToThread(dcThread);

        /* we start the thread when the control plane connection is connected! */
        ControlPlaneConnection* controlPlane = this->getConnection(QString(*(frien_d->uid)));
        connect(controlPlane, SIGNAL(connected()), dcThread, SLOT(start()));

        // TODO delete user?
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

DataPlaneConnection* ConnectionInitiator::getDpConnection(QString uid) {
    static QMutex mutex;
    mutex.lock();
    QListIterator< DataPlaneConnection * > i(instance->dpConnections);
    while (i.hasNext()) {
        DataPlaneConnection* nxt = i.next();
        if (nxt->getUid() == uid) {
            mutex.unlock();
            return nxt;
        }
    }
    // doesn't exist, create a new one
    DataPlaneConnection* newCon = new DataPlaneConnection(uid);
    instance->dpConnections.append(newCon);
    mutex.unlock();
    return newCon;
}

void ConnectionInitiator::removeConnection(ControlPlaneConnection *con) {
    instance->connections.removeAll(con);
}
void ConnectionInitiator::removeConnection(DataPlaneConnection *con) {
    instance->dpConnections.removeAll(con);
}

QString ConnectionInitiator::getMyUid() {
    return instance->qSql->getLocalUid();
}















