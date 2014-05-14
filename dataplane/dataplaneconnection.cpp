#include "dataplaneconnection.h"
#include "bonjoursql.h"
#include "proxyclient.h"
#include <QCryptographicHash>

DataPlaneConnection::DataPlaneConnection(QString uid, AbstractPlaneConnection *parent) :
    AbstractPlaneConnection(uid, parent), friendUid(uid)
{
}

void DataPlaneConnection::removeConnection() {
    BonjourSQL* qSql = BonjourSQL::getInstance();

    qDebug() << "Removing connection!";
    if (friendUid.toInt() < qSql->getLocalUid().toInt()) { // friend is smaller, I am server
        //client->disconnect();
        client->stop();
        client = NULL;
        curMode = Receiving;
    } else {
        //server->disconnect();
        server = NULL;
        curMode = Emitting;
    }
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

void DataPlaneConnection::readBuffer(const char* buf, int len) {
    struct dpHeader *header = (struct dpHeader*) buf;
    const char* packetBuf = buf + sizeof(struct dpHeader*); // packet

    // get server Proxy and send through it!
    Proxy* prox = Proxy::getProxy(hash); // try and get server (hash)

    if (!prox) {
        // compute client proxy
        // read tcp or udp header to get src port

        // get index of ]
        int accIndex = 0;
        while ((packetBuf[++accIndex] != ']') && (accIndex < length)) { }

        if (accIndex == length) {
            qFatal("dataplane client wrong packet format received");
            return;
        }

        // the first 16 bits of UDP or TCP header are the src_port
        quint16* srcPort = static_cast<quint16*>(malloc(sizeof(quint16)));
        memcpy(srcPort, packetBuf + accIndex + 1, sizeof(quint16));
        *srcPort = ntohs(*srcPort);
        QFile test("sourcePort" + QString::number(*srcPort));
        test.open(QIODevice::WriteOnly);
        test.write(packetBuf, length);

        QString hash(header->md5);
        QString srcIp(header->srcIp);
        QByteArray clientHash = QCryptographicHash::hash(QString(hash + srcIp + QString::number(*srcPort)).toUtf8(), QCryptographicHash::Md5);
        prox = Proxy::getProxy(clientHash);
        if (!prox) {
            prox = new ProxyClient(clientHash, hash, srcIp, sockType, *srcPort, this);
            prox->run();
            free(srcPort);
        } else {
            qDebug() << "Found the client proxy ! :)";
        }
    }
    prox->sendBytes(packetBuf, length, srcIp);
}

void DataPlaneConnection::sendBytes(const char *buf, int len, QString& hash, int sockType, QString& srcIp) {
    if (curMode == Closed) {
        qWarning() << "Trying to sendBytes on Closed state for uid" << friendUid;
    }

    // make the DATA header
    struct dpHeader header;
    header.sockType = sockType;
    header.length = len;
    header.md5 = hash.toUtf8().data();
    header.srcIp = srcIp.toUtf8().data();

    void* packet = malloc(len + sizeof(struct dpHeader));
    memcpy(packet, &header, sizeof(struct dpHeader));
    memcpy(packet + sizeof(struct dpHeader), buf, len);

    if (curMode == Emitting) {
        qDebug() << "Send bytes as dataplane client";
        client->sendBytes(packet, len + sizeof(struct dpHeader));
    } else if (curMode == Receiving) {
        qDebug() << "Send bytes as dataplane server";
        server->sendBytes(packet, len + sizeof(struct dpHeader));
    } else {
        qWarning() << "Should not happen, trying to send bytes in Both mode for uid" << friendUid;
    }
}
