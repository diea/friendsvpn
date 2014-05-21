#include "dataplaneconnection.h"
#include "bonjoursql.h"
#include "proxyclient.h"
#include "controlplane/controlplaneconnection.h"
#include <QCryptographicHash>


DataPlaneConnection::DataPlaneConnection(QString uid, AbstractPlaneConnection *parent) :
    AbstractPlaneConnection(uid, parent), client(NULL), server(NULL), lastRcvdTimestamp(time(NULL))
{
    this->connect(this, SIGNAL(disconnected()), SLOT(disconnect()));
}

void DataPlaneConnection::removeConnection() {
    BonjourSQL* qSql = BonjourSQL::getInstance();

    qDebug() << "Removing connection!";
    qDebug() << friendUid.toULongLong() << "<" << qSql->getLocalUid().toULongLong();
    if (friendUid.toULongLong() < qSql->getLocalUid().toULongLong()) { // friend is smaller, I am server
        qDebug() << "friend is smaller!";
        if (client)
            client->stop();
        client = NULL;
        curMode = Receiving;
    } else {
        if (server)
            server->stop();
        server = NULL;
        curMode = Emitting;
    }
    qDebug() << "Connection removed!";
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
    qDebug() << "returing from addMode!";
    return true;
}

void DataPlaneConnection::readBuffer(const char* buf, int len) {
    lastRcvdTimestamp = time(NULL); // we received a packet, update time
    qDebug() << "sending packet of len" << len;
    struct dpHeader *header = (struct dpHeader*) buf;
    const char* packetBuf = buf + sizeof(struct dpHeader); // packet

    //qDebug() << "header srcIp" << header->srcIp << header->md5;
    // qDebug() << "packetBuf"
    QByteArray hash(header->md5, 16);
    char srcIpc[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(header->srcIp), srcIpc, INET6_ADDRSTRLEN);
    QString srcIp(srcIpc);
    // get server Proxy and send through it!
    Proxy* prox = Proxy::getProxy(hash); // try and get server (hash)

    if (!prox) {
        // compute client proxy
        // read tcp or udp header to get src port

        // the first 16 bits of UDP or TCP header are the src_port
        quint16* srcPort = static_cast<quint16*>(malloc(sizeof(quint16)));
        memcpy(srcPort, packetBuf, sizeof(quint16));
        *srcPort = ntohs(*srcPort);

        QByteArray clientHash = QCryptographicHash::hash(QString(hash + srcIp + QString::number(*srcPort)).toUtf8(), QCryptographicHash::Md5);
        prox = Proxy::getProxy(clientHash);
        if (!prox) {
            prox = new ProxyClient(clientHash, hash, srcIp, header->sockType, *srcPort, this);
            prox->run();
            clientProxys.push(dynamic_cast<ProxyClient*>(prox));
            free(srcPort);
        } else {
            qDebug() << "Found the client proxy ! :)";
        }
    }
    prox->sendBytes(packetBuf, ntohs(header->len), srcIp);
}

void DataPlaneConnection::sendBytes(const char *buf, int len, QByteArray& hash, int sockType, QString& srcIp) {
    if (curMode == Closed) {
        qWarning() << "Trying to sendBytes on Closed state for uid" << friendUid;
    }

    if (time(NULL) - lastRcvdTimestamp > 60) {
        // is distant host still alive ?
        // get controlplaneconnection and ask it
        qDebug() << "Checking if host is still alive";
        ConnectionInitiator* in = ConnectionInitiator::getInstance();
        ControlPlaneConnection* c = in->getConnection(friendUid);
        c->alive(); // we continue, the controlplane will close if needed
    }

    // make the DATA header
    struct dpHeader header;
    header.sockType = sockType;
    header.len = htons(qint16(len));
    memcpy(header.md5, hash, 16); // 16 bytes
    inet_pton(AF_INET6, srcIp.toUtf8().data(), &(header.srcIp));

    char* packet = static_cast<char*>(malloc(len + sizeof(struct dpHeader)));
    memcpy(packet, &header, sizeof(struct dpHeader));
    memcpy(packet + sizeof(struct dpHeader), buf, len);

    qDebug() << "sending packet of len" << len;
    sendPacket(packet, len + sizeof(struct dpHeader));
    free(packet);
}

void DataPlaneConnection::sendPacket(const char *buf, int len) {
    mutex.lock();
    if (curMode == Emitting) {
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

void DataPlaneConnection::disconnect() {
    qDebug() << "Disconnect dataplane!";
    if (curMode == Both) {
        client->stop();
        server->stop();
    } else {
        if (curMode == Receiving) {
            qDebug() << "killing server";
            server->stop();
        }
        if (curMode == Emitting) {
            qDebug() << "killing client";
            client->stop();
        }
    }

    qDebug() << "releasing client proxys";
    while (!clientProxys.empty()) {
        Proxy* c = clientProxys.pop();
        if (c)
            delete c;
    }

    curMode = Closed;
    ConnectionInitiator::getInstance()->removeConnection(this);
    this->deleteLater();
}
