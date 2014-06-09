#include "dataplaneconnection.h"
#include "databasehandler.h"
#include "proxyclient.h"
#include "controlplane/controlplaneconnection.h"
#include <QCryptographicHash>


DataPlaneConnection::DataPlaneConnection(QString uid, AbstractPlaneConnection *parent) :
    AbstractPlaneConnection(uid, parent), client(NULL), server(NULL), lastRcvdTimestamp(time(NULL))
{
    this->connect(this, SIGNAL(disconnected()), SLOT(disconnect()));
    globalIdFrag = 0;
}

void DataPlaneConnection::removeConnection() {
    DatabaseHandler* qSql = DatabaseHandler::getInstance();

    if (friendUid.toULongLong() < qSql->getLocalUid().toULongLong()) { // friend is smaller, I am server
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
}

bool DataPlaneConnection::addMode(plane_mode mode, QObject* socket) {
    mutex.lock();
    if ((curMode == Both) || (curMode == mode)) {
        mutex.unlock();
        return false;
    }
    if (mode == Receiving) {
        ServerWorker* sslSocket = dynamic_cast<ServerWorker*>(socket);
        if (!sslSocket) {
            mutex.unlock();
            return false;
        }
        server = sslSocket;
    }
    else {
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

void DataPlaneConnection::readBuffer(char* buf, int bufLen) {
    lastRcvdTimestamp = time(NULL); // we received a packet, update time
    struct dpHeader *header = (struct dpHeader*) buf;
    char* packetBuf = NULL;
    qDebug() << "Got buffer of size" << bufLen << "and header says it has" << ntohs(header->len) << "bytes";

    if (header->fragType != 0) { /* handle fragment */
        struct dpFragHeader* fragHead = (struct dpFragHeader*) (buf + sizeof(struct dpHeader));
        if (!remainingBits.contains(fragHead->fragId)) { /* new frag */
            remainingBits.insert(fragHead->fragId, header->len);
            fragmentBuffer.insert(fragHead->fragId, static_cast<char*>(malloc(header->len)));
            totalSize.insert(fragHead->fragId, header->len);
        }
        if (fragHead->offset + fragHead->offsetLen <= totalSize.value(fragHead->fragId)) {
            const char* frag = buf + sizeof(dpHeader) + sizeof(fragHeader);
            memcpy(fragmentBuffer.value(fragHead->fragId), frag, fragHead->offsetLen);
            remainingBits[fragHead->fragId] -= fragHead->offsetLen;
            if (!remainingBits[fragHead->fragId]) { /* got to 0, packet is arrived */
                qDebug() << "Fragment has been assembled";
                packetBuf = fragmentBuffer.value(fragHead->fragId);
            }
        }
    } else {
        qDebug() << "Not a fragment";
        packetBuf = buf + sizeof(struct dpHeader); // packet
    }

    if (!packetBuf) { return; } /* packet is not yet complete */

    /* TODO, check TCP checksum ? */

    QByteArray hash(header->md5, 16);
    qDebug() << "Hash is" << hash.toHex();
    char srcIpc[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(header->srcIp), srcIpc, INET6_ADDRSTRLEN);
    QString srcIp(srcIpc);
    // get server Proxy and send through it!
    Proxy* prox = Proxy::getProxy(hash); // try and get server (hash)
    if (!prox) {
        // compute client proxy
        // read tcp or udp header to get src port

        // get source port
        quint16 srcPort;
        memcpy(&srcPort, packetBuf, sizeof(quint16)); /* src port is 16 first bits of trans header */
        srcPort = ntohs(srcPort);

        qDebug() << "Source port of packet is " << srcPort;

        QByteArray clientHash = QCryptographicHash::hash(QString(hash + srcIp + QString::number(srcPort)).toUtf8(), QCryptographicHash::Md5);
        qDebug() << "Get proxy";
        prox = Proxy::getProxy(clientHash);
        if (!prox) {
            qDebug() << "New client";
            prox = new ProxyClient(clientHash, hash, srcIp, header->sockType, srcPort, this);
            qDebug() << "Constructor OK, run";
            prox->run();
            qDebug() << "Run OK";
            clientProxys.push(dynamic_cast<ProxyClient*>(prox));
            qDebug() << "Client OK";
        }
    }

    prox->sendBytes(packetBuf, ntohs(header->len), srcIp);
}

char* printBits(quint16 x)
{
   char* bits = static_cast<char*>(malloc(16));
   int i;
   int cnt = 0;
   for(i=(sizeof(quint16)*8)-1; i>=0; i--) {
       (x & (1 << i) ) ? sprintf(bits + cnt,"1") : sprintf(bits + cnt, "0");
       cnt++;
   }
   bits[16] = '\0';
   return bits;
}

/*quint16 DataPlaneConnection::initMaxPayloadLen() {
    quint16 maxPayload = IPV6_MIN_MTU - sizeof(struct ether_header) - sizeof(struct ipv6hdr);
    while (maxPayload % 8 != 0) {
        maxPayload--;
    }
    return maxPayload;
}*/

quint16 DataPlaneConnection::maxPayloadLen = IPV6_MIN_MTU - sizeof(struct dpHeader);

void DataPlaneConnection::sendBytes(const char *buf, int len, QByteArray& hash, int sockType, QString& srcIp) {
    if (time(NULL) - lastRcvdTimestamp > TIMEOUT_DELAY) {
        qDebug() << "Testing ALIVE";
        // is distant host still alive ?
        // get controlplaneconnection and ask it
        ConnectionInitiator* in = ConnectionInitiator::getInstance();
        ControlPlaneConnection* c = in->getConnection(friendUid);
        c->alive(); // we continue, the controlplane will close if needed
    }

    // make the DATA header
    struct dpHeader header;
    memset(&header, 0, sizeof(struct dpHeader));
    header.sockType = sockType;
    header.len = htons(qint16(len));
    qDebug() << "Putting md5" << hash.toHex() << "in header";
    memcpy(header.md5, hash.data(), sizeof(char) * 16); // 16 bytes
    qDebug() << "Header has md5" << QByteArray(header.md5, 16).toHex();
    inet_pton(AF_INET6, srcIp.toUtf8().data(), &(header.srcIp));

    qDebug() << "Comparing" << static_cast<unsigned long>(len) << "and" << maxPayloadLen;
    if (static_cast<unsigned long>(len) > maxPayloadLen) {
        // packet will use more than the min MTU, we fragment it
        quint16 dataFieldLen = maxPayloadLen - sizeof(struct dpHeader) - sizeof(struct dpFragHeader);
        qDebug() << "Frag data field is" << dataFieldLen << "bytes";
        struct dpFragHeader dpFrag;
        memset(&dpFrag, 0, sizeof(struct dpFragHeader));

        globalIdMutex.lock();
        dpFrag.fragId = htons(globalIdFrag++);
        globalIdMutex.unlock();

        header.fragType = 1;

        quint32 pos = 0;
        while (len > 0) { // send frags while len is > 0
            int payloadLen = len >= dataFieldLen ? dataFieldLen : len;
            if (payloadLen == len) {
                header.fragType = 3; // last frag
            }

            char* packet = static_cast<char*>(malloc(maxPayloadLen));
            if (!packet) {
                qDebug() << "packet could not be allocated!";
                return;
            }
            qDebug() << "Got out of malloc";
            dpFrag.offset = htons(pos);
            dpFrag.offsetLen = htons(payloadLen);

            memcpy(packet, &header, sizeof(struct dpHeader));
            memcpy(packet + sizeof(struct dpHeader), &dpFrag, sizeof(struct dpFragHeader));
            memcpy(packet + sizeof(struct dpHeader) + sizeof(struct dpFragHeader), buf + pos, payloadLen);

            qDebug() << "Sending packet with offset" << pos;
            sendPacket(packet, maxPayloadLen);

            header.fragType = 2; // first frag was sent

            pos += payloadLen;
            len -= payloadLen;
            free(packet);
        }
    } else {
        int packetLen = len + sizeof(struct dpHeader);
        qDebug() << "Going in packet malloc" << packetLen;
        char* packet = static_cast<char*>(malloc(packetLen));
        if (!packet) {
            qDebug() << "packet could not be allocated!";
            return;
        }
        qDebug() << "Got out of malloc";
        memcpy(packet, &header, sizeof(struct dpHeader));
        memcpy(packet + sizeof(struct dpHeader), buf, len);

        sendPacket(packet, packetLen);
        free(packet);
    }
}

void DataPlaneConnection::sendPacket(const char *buf, int len) {
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

void DataPlaneConnection::disconnect() {
    mutex.lock();
    if (curMode == Both) {
        client->stop();
        server->stop();
    } else {
        if (curMode == Receiving) {
            server->stop();
        }
        if (curMode == Emitting) {
            client->stop();
        }
    }
    mutex.unlock();
    while (!clientProxys.empty()) {
        Proxy* c = clientProxys.pop();
        if (c)
            delete c;
    }

    curMode = Closed;
    ConnectionInitiator::getInstance()->removeConnection(this);
    this->deleteLater();
}
