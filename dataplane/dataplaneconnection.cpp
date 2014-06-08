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

void DataPlaneConnection::readBuffer(const char* buf, int bufLen) {
    lastRcvdTimestamp = time(NULL); // we received a packet, update time
    struct dpHeader *header = (struct dpHeader*) buf;
    const char* packetBuf = buf + sizeof(struct dpHeader); // packet

    qDebug() << "Got buffer of size" << bufLen << "and header says it has" << ntohs(header->len) << "bytes";

    QByteArray hash(header->md5, 16);
    char srcIpc[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(header->srcIp), srcIpc, INET6_ADDRSTRLEN);
    QString srcIp(srcIpc);
    // get server Proxy and send through it!
    Proxy* prox = Proxy::getProxy(hash); // try and get server (hash)
    if (!prox) {
        // compute client proxy
        // read tcp or udp header to get src port

        // get source port
        quint16 srcPort = ntohs(header->srcPort);

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

    prox->sendBytes(packetBuf, ntohs(header->len), srcIp, header->fragType);
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
    memcpy(header.md5, hash.data(), sizeof(char) * 16); // 16 bytes
    inet_pton(AF_INET6, srcIp.toUtf8().data(), &(header.srcIp));

    memcpy(&(header.srcPort), buf, sizeof(quint16)); /* srcPort is in the first 16 bits of the transport header */

    qDebug() << "Comparing" << static_cast<unsigned long>(len) << "and" << IPV6_MIN_MTU - sizeof(struct ether_header) - sizeof(struct ipv6hdr);
    if (static_cast<unsigned long>(len) > IPV6_MIN_MTU - sizeof(struct ether_header) - sizeof(struct ipv6hdr)) {
        // packet will use more than the min MTU, we fragment it
        quint16 dataFieldLen = IPV6_MIN_MTU - sizeof(struct ether_header) - sizeof(struct ipv6hdr) - sizeof(struct fragHeader);

        qDebug() << "Frag data field is" << dataFieldLen << "bytes";

        quint16 offsetVal = dataFieldLen / 8;
        quint16 fragOffsetMult = 0;
        quint32 fragId = globalIdFrag++;
        quint32 pos = 0;
        while (len > 0) { // send frags while len is > 0
            struct fragHeader fhead;
            memset(&fhead, 0, sizeof(struct fragHeader));
            fhead.nextHeader = (sockType == SOCK_DGRAM) ? SOL_UDP : SOL_TCP;
            fhead.identification = htonl(fragId);
            quint16 offset = htons(fragOffsetMult * offsetVal);
            header.fragType = !offset ? 1 : 2;
            fhead.fragOffsetResAndM |= (offset << 3);
            quint16 mbit = 1 >> 7;
            fhead.fragOffsetResAndM |= mbit;
            fragOffsetMult++;

            qDebug() << "Frag off res and M" << printBits(fhead.fragOffsetResAndM);

            int payloadLen = len >= dataFieldLen ? dataFieldLen : len;
            header.len = htons(sizeof(struct fragHeader) + payloadLen);

            char* packet = static_cast<char*>(malloc(payloadLen + sizeof(struct dpHeader) +
                                                     + sizeof(struct fragHeader)));
            if (!packet) {
                qDebug() << "packet could not be allocated!";
                return;
            }
            qDebug() << "Got out of malloc";
            memcpy(packet, &header, sizeof(struct dpHeader));
            memcpy(packet + sizeof(struct dpHeader), &fhead, sizeof(struct fragHeader));
            memcpy(packet + sizeof(struct dpHeader) + sizeof(struct fragHeader), buf + pos, payloadLen);

            qDebug() << "Sending packet with offset" << ntohs(offset);
            sendPacket(packet, payloadLen);

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
