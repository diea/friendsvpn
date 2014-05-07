#include "dataplaneconnection.h"
#include "bonjoursql.h"

DataPlaneConnection::DataPlaneConnection(QString uid, AbstractPlaneConnection *parent) :
    AbstractPlaneConnection(uid, parent), friendUid(uid)
{
}

void DataPlaneConnection::removeConnection() {
    BonjourSQL* qSql = BonjourSQL::getInstance();

    qDebug() << "Removing conncetion!";
    mutex.lock();
    if (friendUid < qSql->getLocalUid()) { // friend is smaller, I am server
        client->disconnect();
        client = NULL;
        curMode = Receiving;
    } else {
        server->disconnect();
        server = NULL;
        curMode = Emitting;
    }
    mutex.unlock();
}

bool DataPlaneConnection::addMode(plane_mode mode, QObject* socket) {
    mutex.lock();
    if ((curMode == Both) || (curMode == mode)) {
        mutex.unlock();
        return false;
    }
    if (mode == Receiving) {
        qDebug() << "Adding receiving mode for dataplane for uid" << friendUid;
        ServerWorker* sslSocket = dynamic_cast<ServerWorker*>(socket);
        if (!sslSocket) {
            qDebug() << "NOT a server worker! returing false";
            mutex.unlock();
            return false;
        }
        server = sslSocket;
    }
    else {
        qDebug() << "Adding emitting mode for dataplane!";
        DataPlaneClient* sslSocket = dynamic_cast<DataPlaneClient*>(socket);
        if (!sslSocket) {
            mutex.unlock();
            return false;
        }
        client = sslSocket;
    }

    if (curMode == Closed)  {
        curMode = mode;
        emit connected();
    }
    else curMode = Both;

    if (curMode == Both)
        this->removeConnection();

    mutex.unlock();
    return true;
}

void DataPlaneConnection::readBuffer(const char* buf) {
    mutex.lock();
    qDebug() << "DataPlane buffer" << buf;

    // get client proxy and send data through it
    // TODO
    mutex.unlock();
}

void DataPlaneConnection::sendBytes(const char *buf, int len) {
    mutex.lock();
    if (curMode == Closed) {
        qWarning() << "Trying to sendBytes on Closed state for uid" << friendUid;
    } else if (curMode == Emitting) {
        qDebug() << "Send bytes as dataplane client";
        client->sendBytes(buf, len);
    } else if (curMode == Receiving) {
        qDebug() << "Send bytes as dataplane server";
        server->sendBytes(buf, len);
    } else {
        qWarning() << "Should not happen, trying to send bytes in Both mode for uid" << friendUid;
    }
    mutex.unlock();
}
