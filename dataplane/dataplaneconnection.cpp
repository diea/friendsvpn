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
    mutex.lock();
    qDebug() << "DataPlane buffer length" << len << "and buffer" << buf;

    int bufferPosition = 0; // to know where to start reading in the buffer (useful when there are multiple packets)
    int nbLoops = 4;
    while (len > 0) {
        nbLoops--;
        QString packet(buf + bufferPosition);
        qDebug() << "We have LEN " << len << "remaining ";

        /* /!\ buffer can be multiple packets */
        QStringList list = packet.split("\r\n");
        qDebug() << "packet list " << list;
        QString header;
        if (list.at(0) == "DATA") {
            QString hash;
            QString srcIp;
            int sockType;
            int length;
            const char* packetBuf; // packet
            header = header % "DATA\r\n";
            for (int i = 1; i < list.length() - 1 ; i++) {
                header = header % list.at(i) % "\r\n";
                if (list.at(i).isEmpty()) {
                    if (!list.at(i + 1).isEmpty()) {
                        QByteArray headerBytes = header.toUtf8();
                        packetBuf = buf + headerBytes.length();
                        len -= headerBytes.length(); // we read the header remove it from len
                        bufferPosition += headerBytes.length();
                    }
                } else {
                    QStringList keyValuePair = list.at(i).split("=");
                    QString key = keyValuePair.at(0);
                    if (key == "Hash") {
                        hash = keyValuePair.at(1);
                    } else if (key == "Length") {
                        length = keyValuePair.at(1).toInt();
                    } else if (key == "Trans") {
                        if (keyValuePair.at(1) == "tcp") {
                            sockType = SOCK_STREAM;
                        } else {
                            sockType = SOCK_DGRAM;
                        }
                    } else if (key == "SrcIP") {
                        srcIp = keyValuePair.at(1);
                    }
                }
            }

            // get server Proxy and send through it!
            Proxy* prox = Proxy::getProxy(hash); // try and get server (hash)

            if (!prox) {
                // compute client proxy
                // read tcp or udp header to get src port
                QFile tcpPacket("packetBuf");
                tcpPacket.open(QIODevice::WriteOnly);
                tcpPacket.write(packetBuf, length);

                // get index of ]
                int accIndex = 0;
                while ((packetBuf[++accIndex] != ']') && (accIndex < length)) { }

                if (accIndex == length) {
                    qDebug() << "dataplane client wrong packet format received";
                    return;
                }

                // the first 16 bits of UDP or TCP header are the src_port
                quint16* srcPort = static_cast<quint16*>(malloc(sizeof(quint16)));
                memcpy(srcPort, packetBuf + accIndex + 1, sizeof(quint16));
                *srcPort = ntohs(*srcPort);
                QFile test("sourcePort" + QString::number(*srcPort));
                test.open(QIODevice::WriteOnly);
                test.write(packetBuf, length);

                QByteArray clientHash = QCryptographicHash::hash(QString(hash + srcIp + QString::number(*srcPort)).toUtf8(), QCryptographicHash::Md5);
                prox = Proxy::getProxy(clientHash);
                if (!prox) {
        #if 0
                    qDebug() << "sending the bloody packet!";
                    QProcess* raw = new QProcess();
                    QStringList sendRawArgs;
                    sendRawArgs.append(QString::number(sockType));
                    sendRawArgs.append(QString::number(IPPROTO_TCP));
                    sendRawArgs.append("::1");
                    raw->start(QString(HELPERPATH) + "sendRaw", sendRawArgs);
                    qDebug() << "sendRawArgs!" << sendRawArgs;
                    QFile tcpPacket("tcpPacket");
                    tcpPacket.open(QIODevice::WriteOnly);
                    tcpPacket.write(packetBuf, length);

                    raw->write(packetBuf, length);
                    qDebug() << "raw has written";
                    raw->waitForReadyRead(2000);
                    qDebug() << raw->readAll();
        #endif
                    qDebug() << "new client proxy thread!";
                    QThread* proxyThread = new QThread();

                    // compute the proxyClient's unique hash
                    // = md5(hash + srcPort + srcIp)

                    prox = new ProxyClient(clientHash, hash, srcIp, sockType, *srcPort, this);
                    //prox->moveToThread(proxyThread);
                    //connect(proxyThread, SIGNAL(started()), prox, SLOT(run()));
                    //connect(proxyThread, SIGNAL(finished()), proxyThread, SLOT(deleteLater()));
                    // TODO delete proxy client also on finished() ?
                    //proxyThread->start();
                    prox->run();

                    free(srcPort);
                } else {
                    qDebug() << "Found the client proxy ! :)";
                }
            }
            prox->sendBytes(packetBuf, length, srcIp);
            // we read the packet, remove from len
            len -= length;
            qDebug() << "We had LEN" << len << "and length" << length;
            bufferPosition += length;
        } else {
            qDebug() << "PACKET DID NOT START WITH PROXY";
            len--;
        }
    }
    mutex.unlock();
}

void DataPlaneConnection::sendBytes(const char *buf, int len, QString& hash, int sockType, QString& srcIp) {
    mutex.lock();
    if (curMode == Closed) {
        qWarning() << "Trying to sendBytes on Closed state for uid" << friendUid;
    }

    // make the DATA header
    QString header;
    QString trans = (sockType == SOCK_DGRAM) ? "udp" : "tcp";
    header = header  % "DATA\r\n"
            % "Hash=" % hash % "\r\n"
            % "Trans=" % trans % "\r\n"
            % "Length=" % QString::number(len) % "\r\n"
            % "SrcIP=" % srcIp % "\r\n"
            % "\r\n";
    QByteArray headerBytes = header.toUtf8();
    int headerLen = headerBytes.length();

    int packetLen = len + headerLen;
    char dataPacket[packetLen];
    char* headerC = headerBytes.data();
    strncpy(dataPacket, headerC, headerLen);
    memcpy(dataPacket + headerLen, buf, len);

    QFile tcpPacket("tcpPacketDTLS");
    tcpPacket.open(QIODevice::WriteOnly);
    tcpPacket.write(dataPacket, packetLen);

    QFile tcpPacket1("tcpPacketDTLS_noheader");
    tcpPacket1.open(QIODevice::WriteOnly);
    tcpPacket1.write(buf, len);

    if (curMode == Emitting) {
        qDebug() << "Send bytes as dataplane client";
        client->sendBytes(dataPacket, packetLen);
    } else if (curMode == Receiving) {
        qDebug() << "Send bytes as dataplane server";
        server->sendBytes(dataPacket, packetLen);
    } else {
        qWarning() << "Should not happen, trying to send bytes in Both mode for uid" << friendUid;
    }
    mutex.unlock();
}
