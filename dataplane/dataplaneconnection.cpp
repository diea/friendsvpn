#include "dataplaneconnection.h"
#include "bonjoursql.h"

DataPlaneConnection::DataPlaneConnection(QString uid, AbstractPlaneConnection *parent) :
    AbstractPlaneConnection(uid, parent), friendUid(uid)
{
}

void DataPlaneConnection::removeConnection() {
    BonjourSQL* qSql = BonjourSQL::getInstance();
    if (friendUid < qSql->getLocalUid()) { // friend is smaller, I am server
        client->disconnect();
        client = NULL;
        curMode = Receiving;
    } else {
        server->disconnect();
        server = NULL;
        curMode = Emitting;
    }
}

bool DataPlaneConnection::addMode(plane_mode mode, QObject* socket) {
    if ((curMode == Both) || (curMode == mode)) return false;
    if (mode == Receiving) {
        ServerWorker* sslSocket = dynamic_cast<ServerWorker*>(socket);
        if (!sslSocket) return false;
        server = sslSocket;
    }
    else {
        DataPlaneClient* sslSocket = dynamic_cast<DataPlaneClient*>(socket);
        if (!sslSocket) return false;
        client = sslSocket;
    }

    if (curMode == Closed)  {
        curMode = mode;
        emit connected();
    }
    else curMode = Both;

    if (curMode == Both)
        this->removeConnection();

    return true;
}

void DataPlaneConnection::readBuffer(const char* buf) {
    qDebug() << "DataPlane buffer" << buf;

    // get client proxy and send data through it
    // TODO
}

void DataPlaneConnection::sendBytes(const char *buf, int len) {
    static QMutex mutex;
    mutex.lock();
    if (curMode == Closed) {
        qWarning() << "Trying to sendBytes on Closed state for uid" << friendUid;
    } else if (curMode == Emitting) {
        client->sendBytes(buf, len);
    } else if (curMode == Receiving) {
        server->sendBytes(buf, len);
    } else {
        qWarning() << "Should not happen, trying to send bytes in Both mode for uid" << friendUid;
    }
    mutex.unlock();
}
