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
    header->len = ntohs(header->len);
    qDebug() << "Got buffer of size" << bufLen << "and header says it has" << header->len << "bytes";

    if (header->fragType != 0) { /* handle fragment */
        struct dpFragHeader* fragHead = (struct dpFragHeader*) (buf + sizeof(struct dpHeader));
        fragHead->fragId = ntohl(fragHead->fragId);
        fragHead->offset = ntohs(fragHead->offset);
        qDebug() << bufLen << "-" <<  sizeof(struct dpHeader) << "-" << sizeof(struct dpFragHeader);
        quint16 offsetLen = bufLen - sizeof(struct dpHeader) - sizeof(struct dpFragHeader);
        fragBufMut.lock();
        if (!fragmentBuffer.contains(fragHead->fragId)) { /* new frag */
            qDebug() << "Appending fragment with ID" << fragHead->fragId;
            if (fragmentBuffer.size() > FRAG_BUFFER_SIZE) {
                /* remove everything from QHash */
                QHash<quint32, struct fragment_local*>::iterator i;
                for (i = fragmentBuffer.begin(); i != fragmentBuffer.end(); ++i) {
                    struct fragment_local* frag = i.value();
                    qDebug() << "Removing ID" << i.key();
                    qDebug() << "Fragment is discarded, free";
                    free(frag->fragBuf);
                    qDebug() << "Buffer was freed, free structure";
                    free(frag);
                    qDebug() << "Remove call";
                    i = fragmentBuffer.erase(i);
                }
                qDebug() << "Fragment buffer QHash is now empty";
            }
            struct fragment_local* frag = static_cast<struct fragment_local*>(malloc(sizeof(struct fragment_local)));
            memset(frag, 0, sizeof(struct fragment_local));
            frag->fragBuf = static_cast<char*>(malloc(header->len));
            memset(frag->fragBuf, 0, header->len);
            frag->remainingBits = header->len;
            frag->totalSize = header->len;
            fragmentBuffer.insert(fragHead->fragId, frag);
            qDebug() << "Frag buffer size" << fragmentBuffer.size();
        }
        fragBufMut.unlock();
        struct fragment_local* frag = fragmentBuffer.value(fragHead->fragId);
        if (frag) {
            qDebug() << "Got fragment of offset" << fragHead->offset << "and len" << offsetLen;
            if (fragHead->offset + offsetLen <= frag->totalSize) {
                const char* frag_rcvd = buf + sizeof(struct dpHeader) + sizeof(struct dpFragHeader);
                memcpy(frag->fragBuf + fragHead->offset, frag_rcvd, offsetLen);
                remainingBitsMutex.lock();
                qDebug() << "Remaining bits" << frag->remainingBits << "-=" << offsetLen;
                frag->remainingBits -= offsetLen;
                qDebug() << "Remaining bytes for fragId" << fragHead->fragId << "are" << frag->remainingBits;
                if (!frag->remainingBits) { /* got to 0, packet is arrived */
                    qDebug() << "Fragment has been assembled";
                    packetBuf = frag->fragBuf;
                } else if (frag->remainingBits < 0) {
                    qDebug() << "Should not happen, remaining bits is < 0";
                }
                remainingBitsMutex.unlock();
            }
        }
    } else {
        qDebug() << "Not a fragment";
        packetBuf = buf + sizeof(struct dpHeader); // packet
    }

    if (!packetBuf) { return; } /* packet is not yet complete */

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

    prox->sendBytes(packetBuf, header->len, srcIp);

    if (header->fragType != 0) { /* free resources to assemble packet */
        struct dpFragHeader* fragHead = (struct dpFragHeader*) (buf + sizeof(struct dpHeader));
        qDebug() << "Removing ID" << fragHead->fragId;
        struct fragment_local* f = fragmentBuffer.value(fragHead->fragId);
        free(f->fragBuf);
        f->fragBuf = 0;
        free(f);
        fragmentBuffer.remove(fragHead->fragId);
    }
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
        quint16 dataFieldLen = maxPayloadLen - sizeof(struct dpFragHeader); //TODO remove dpHeader
        qDebug() << "Frag data field is" << dataFieldLen << "bytes";
        struct dpFragHeader dpFrag;
        memset(&dpFrag, 0, sizeof(struct dpFragHeader));

        globalIdMutex.lock();
        dpFrag.fragId = htonl(globalIdFrag++);
        globalIdMutex.unlock();

        header.fragType = 1;

        quint16 pos = 0;
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
            //dpFrag.offsetLen = htons(payloadLen); /* TODO maybe not needed as UDP contains length */

            memcpy(packet, &header, sizeof(struct dpHeader));
            memcpy(packet + sizeof(struct dpHeader), &dpFrag, sizeof(struct dpFragHeader));
            memcpy(packet + sizeof(struct dpHeader) + sizeof(struct dpFragHeader), buf + pos, payloadLen);

            qDebug() << "Sending packet with offset" << pos << "and with len" << payloadLen;
            qDebug() << payloadLen << "+" << sizeof(struct dpHeader) << "+" << sizeof(struct dpFragHeader);
            sendPacket(packet, payloadLen + sizeof(struct dpHeader) + sizeof(struct dpFragHeader));

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
    qDebug() << "Disconnect DataPlaneConnection";
    mutex.lock();
    if (curMode == Both) {
        client->stop();
        server->stop();
    } else {
        qDebug() << "Not in both mode";
        if (curMode == Receiving) {
            qDebug() << "Stopping server";
            server->stop();
        }
        if (curMode == Emitting) {
            qDebug() << "Stopping client";
            client->stop();
        }
    }
    curMode = Closed;
    mutex.unlock();
    qDebug() << "Deleting client proxies";
    while (!clientProxys.empty()) {
        Proxy* c = clientProxys.pop();
        if (c)
            delete c;
    }
    qDebug() << "Removing from ConnectionInitiator";
    ConnectionInitiator::getInstance()->removeConnection(this);
    qDebug() << "Delete self";
    this->deleteLater();
}
