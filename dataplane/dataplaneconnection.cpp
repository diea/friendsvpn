#include "dataplaneconnection.h"
#include "bonjoursql.h"
#include "proxyclient.h"
#include <QCryptographicHash>

DataPlaneConnection::DataPlaneConnection(QString uid, AbstractPlaneConnection *parent) :
    AbstractPlaneConnection(uid, parent), client(NULL), server(NULL)
{
    this->connect(this, SIGNAL(disconnected()), SLOT(disconnect()));
}

void DataPlaneConnection::removeConnection() {
    BonjourSQL* qSql = BonjourSQL::getInstance();

    qDebug() << "Removing connection!";
    if (friendUid.toInt() < qSql->getLocalUid().toInt()) { // friend is smaller, I am server
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
    qDebug() << "sending packet of len" << len;
    struct dpHeader *header = (struct dpHeader*) buf;
    const char* packetBuf = buf + sizeof(struct dpHeader); // packet

    qDebug() << "header srcIp" << header->srcIp << header->md5;
    // qDebug() << "packetBuf"
    QString hash(header->md5);
    QString srcIp(header->srcIp);
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
    prox->sendBytes(packetBuf, header->len, srcIp);
}

void DataPlaneConnection::sendBytes(const char *buf, int len, QString& hash, int sockType, QString& srcIp) {
    if (curMode == Closed) {
        qWarning() << "Trying to sendBytes on Closed state for uid" << friendUid;
    }

    // make the DATA header
    struct dpHeader header;
    header.sockType = sockType;
    qDebug() << "length" << len;
    header.len = qint16(len);
    qDebug() << "header contains length in 16bit" << header.len;
    qDebug() << "hash length" << hash.length();
    strcpy(header.md5, hash.toUtf8().data());
    strcpy(header.srcIp, srcIp.toUtf8().data());

    char* packet = static_cast<char*>(malloc(len + sizeof(struct dpHeader)));
    memcpy(packet, &header, sizeof(struct dpHeader));
    memcpy(packet + sizeof(struct dpHeader), buf, len);

    qDebug() << "sending packet of len" << len;

    if (curMode == Emitting) {
        qDebug() << "Send bytes as dataplane client";
        client->sendBytes(packet, len + sizeof(struct dpHeader));
        // TODO Free packet in sendBytes
    } else if (curMode == Receiving) {
        qDebug() << "Send bytes as dataplane server";
        server->sendBytes(packet, len + sizeof(struct dpHeader));
        // TODO Free packet in sendBytes
    } else {
        qWarning() << "Should not happen, trying to send bytes in Both mode for uid" << friendUid;
    }
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
