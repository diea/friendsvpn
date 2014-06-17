#include "controlplaneconnection.h"
#include "connectioninitiator.h"
#include "proxyserver.h"
#include "sslsocket.h"
#include "graphic/systray.h"
#include <time.h>
#include <QDebug>

char * my_strnstr(const char *s, const char *find, size_t slen) {
    /* from http://www.opensource.apple.com/source/Libc/Libc-320/string/FreeBSD/strnstr.c */
    char c, sc;
    size_t len;

    if ((c = *find++) != '\0') {
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == '\0' || slen-- < 1)
                    return (NULL);
            } while (sc != c);
            if (len > slen)
                return (NULL);
        } while (strncmp(s, find, len) != 0);
        s--;
    }
    return ((char *)s);
}
/**
 * @brief strrstr inverse of strnstr, returns the last occurence of find in string
 * from stackoverflow.com/questions/1634359/is-there-a-reverse-fn-for-strstr
 */
char * strrstr(char *string, const char *find, size_t len) {
    char *cp;
    for (cp = string + len - 4; cp >= string; cp--) {
        if (strncmp(cp, find, 4) == 0)
            return cp;
    }
    return NULL;
}

ControlPlaneConnection::ControlPlaneConnection(QString uid, AbstractPlaneConnection *parent) :
    AbstractPlaneConnection(uid, parent), lastRcvdTimestamp(time(NULL))
{
    qSql = DatabaseHandler::getInstance();
    this->connect(this, SIGNAL(disconnected()), SLOT(wasDisconnected()));
    this->connect(this, SIGNAL(connected()), SLOT(sendBonjour()));
    this->connect(this, SIGNAL(connected()), SLOT(alive()));

    connect(SysTray::getInstance(), SIGNAL(sendBonjour()), this, SLOT(sendBonjour()));
    serverSock = NULL;
    clientSock = NULL;
    inputBuffer = static_cast<char*>(malloc(100000 * sizeof(char)));
    lastFullPacket = 0;
    bytesReceived = 0;
}

ControlPlaneConnection::~ControlPlaneConnection() {
    qDebug() << "Destroying control plane connection";
    if (serverSock) {
        serverSock->close();
        delete serverSock;
    }
    if (clientSock) {
        clientSock->close();
        delete clientSock;
    }
    free(inputBuffer);
}

void ControlPlaneConnection::alive() {
    QString alive = "PING\r\n\r\n";
    sendPacket(alive);
    qDebug() << "Testing if" << friendUid << "is alive";
    QTimer::singleShot(5000, this, SLOT(aliveTimeout()));
}

void ControlPlaneConnection::aliveTimeout() {
    if (time(NULL) - lastRcvdTimestamp > TIMEOUT_DELAY) {
        qDebug() << "Connection with " << friendUid << "timed out";
        wasDisconnected(); // we disco
    }
}

void ControlPlaneConnection::sendPacket(QString& packet) {
    mutex.lock();
    SslSocket* toWrite; // the socket on which the bonjour packets are to be sent

    if (curMode == Closed) {
        qDebug() << "Trying to write bonjour but mode is closed";
        mutex.unlock();
        return;
    }

    if (curMode == Emitting) {
        toWrite = clientSock;
    } else {
        toWrite = serverSock;
    }

    toWrite->write(packet.toUtf8().data(), packet.toUtf8().length());
    mutex.unlock();
}

void ControlPlaneConnection::removeConnection() {
    if (friendUid.toULongLong() < qSql->getLocalUid().toULongLong()) { // friend is smaller, I am server
        clientSock->close();
        clientSock = NULL;
        if (curMode == Both)
            curMode = Receiving;
        else
            curMode = Closed;
    } else {
        serverSock->close();
        serverSock = NULL;
        if (curMode == Both)
            curMode = Emitting;
        else
            curMode = Closed;
    }
}

bool ControlPlaneConnection::addMode(plane_mode mode, QObject *socket) {
    SslSocket* sslSocket = dynamic_cast<SslSocket*>(socket);
    if (!sslSocket) {
        qDebug() << "Add mode fail 1 controlplane";
        return false; // needs to be of type QSslSocket!
    }

    if ((curMode == Both) || (curMode == mode)) {
        qDebug() << "Add mode fail 2 controlplane";
        return false;
    }
    if (mode == Receiving)
        serverSock = sslSocket;
    else clientSock = sslSocket;

    if (curMode == Closed)  {
        curMode = mode;
        emit connected();
    }
    else curMode = Both;

    if (curMode == Both)
        this->removeConnection();

    qDebug() << "Added mode " << mode << "controlplane";
    return true;
}

void ControlPlaneConnection::readBuffer(char* buf, int len) {
    qDebug() << "Reading buffer";
    qDebug() << buf;
    memcpy(inputBuffer + bytesReceived, buf, len); // copy incoming buffer
    lastFullPacket = 0;
    if (len > 0) {
        qDebug() << "Bytesreceived is" << bytesReceived;
        bytesReceived += len;
        if ((inputBuffer[bytesReceived - 1] == '\n') && (inputBuffer[bytesReceived - 2] == '\r')
                && (inputBuffer[bytesReceived - 3] == '\n') && (inputBuffer[bytesReceived - 4] == '\r')) {
            lastFullPacket = bytesReceived;
        } else {
            char* packetEnd = strrstr(inputBuffer, "\r\n\r\n", bytesReceived) + 4;
            int packetEndIndex = packetEnd - buf;
            qDebug() << "Packet end index is" << packetEndIndex;
            if (packetEndIndex > 4) {
                // one full packet
                lastFullPacket = packetEndIndex;
            }
        }
    }

    qDebug() << "buffer Length is" << len;
    qDebug() << "lastFullPacket" << lastFullPacket;
    if (!lastFullPacket) {
        qDebug() << "Did not get full packet yet";
        return;
    }

    int ok_len = lastFullPacket; // we only read until the last complete packet
    int bufferPosition = 0;
    lastRcvdTimestamp = time(NULL); // we received a packet, update time
    while (ok_len > 0) {
        qDebug() << "ok_len is" << ok_len;
        const char* found = my_strnstr(buf + bufferPosition, "\r\n\r\n", ok_len);
        int headerLength = 0;
        if (found) {
            headerLength = found - (buf + bufferPosition);
        } else {
            qDebug() << "No \r\n\r\n in packet";
            qDebug() << "Buffer" << buf;
            return;
        }

        if (!headerLength) {
            qWarning() << "ERROR: packet with header length 0";
            return;
        }

        ok_len -= headerLength + 4; // 4 count for the \r\n\r\n

        QString packet = QString::fromUtf8(buf + bufferPosition, headerLength);
        bufferPosition += headerLength + 4; // skip to the next packet

        QStringList list = packet.split("\r\n");

        QString packetType = list.at(0);
        if (packetType == "HELLO") {
            QStringList disec = list.at(1).split(":");
            if (!(disec.length() < 2)) {
                if (this->friendUid != disec.at(1)) {
                    wasDisconnected();
                    return;
                }
            }
        } else if (packetType == "BONJOUR") {
            QString hostname;
            QString name;
            QString type;
            int port = 0;
            QString md5;
            QString txt;
            for (int i = 1; i < list.length(); i++) {
                QString listAti = list.at(i);
                if (listAti.startsWith("Hostname:")) {
                    hostname = listAti.right(listAti.length() - 9); // 9 is hostname size
                } else if (listAti.startsWith("Name:")) {
                    name = listAti.right(listAti.length() - 5);
                } else if (listAti.startsWith("Type:")) {
                    type = listAti.right(listAti.length() - 5);
                } else if (listAti.startsWith("Port:")) {
                    port = listAti.right(listAti.length() - 5).toInt();
                } else if (listAti.startsWith("MD5:")) {
                    md5 = listAti.right(listAti.length() - 4);
                } else if (listAti.startsWith("TXT:")) {
                    txt = listAti.right(listAti.length() - 4);
                }
            }

            qDebug() << "MD5 is" << md5;

            if (hostname.isEmpty() || name.isEmpty() || type.isEmpty() || !port) {
                qWarning() << "ERROR: Bonjour packet wrong format";
                return;
            }

            ProxyServer* newProxy = NULL;
            try {
                newProxy = new ProxyServer(friendUid, name, type, "local.", hostname, QByteArray::fromHex(txt.toUtf8()), port, QByteArray::fromHex(md5.toUtf8()));
                proxyServers.push(newProxy);
                newProxy->run();
            } catch (int i) {
                // proxy exists
            }
        } else if (packetType == "PING") {
            QString pong = "PONG\r\n\r\n";
            sendPacket(pong);
        } else if (packetType == "PONG") {
        } else if (packetType == "STOP") {
            /* stop a given proxy server */
            QStringList disec = list.at(1).split(":");
            if (disec.size() == 2 && disec.at(0) == "MD5") {
                QString md5Hash = disec.at(1);
                ProxyServer* server = dynamic_cast<ProxyServer*>(Proxy::getProxy(QByteArray::fromHex(md5Hash.toUtf8())));
                if (server) {
                    delete server;
                }
            } else {
                qWarning() << "STOP message no properly formatted";
            }
        } else { // no need to analyze PONG, we just need a packet for the time :)
            qDebug() << "Not starting with BONJOUR or PING";
            qDebug() << packet;
        }
    }

    // handle incomplete packets
    bytesReceived -= lastFullPacket;
    qDebug() << "Bytes received is still" << bytesReceived;
    if (bytesReceived > 0) {
        // we have an incomplete packet at the end
        memmove(inputBuffer, inputBuffer + lastFullPacket, bytesReceived); // move everything back at start of buffer
    }
}

void ControlPlaneConnection::sendBonjour() {
    // get bonjour records from db & send them over the connection
    QMapIterator<BonjourRecord*, bool> i(sharedRecords);
    foreach(BonjourRecord* rec, sharedRecords.keys()) {
        sharedRecords[rec] = false;
    }

    QList < BonjourRecord* > records = qSql->getRecordsFor(this->friendUid);
    foreach (BonjourRecord* rec, records) {
        QString packet;
        packet = packet
                 % "BONJOUR\r\n"
                 % "Hostname:" % rec->hostname % "\r\n"
                 % "Name:" % rec->serviceName % "\r\n"
                 % "Type:" % rec->registeredType % "\r\n"
                 % "Port:" % QString::number(rec->port) % "\r\n"
                 % "MD5:" % rec->md5.toHex() % "\r\n";

        if (!rec->txt.isEmpty()) {
            packet = packet % "TXT:" % rec->txt.toHex() % "\r\n";
        }

        packet = packet % "\r\n";

        qDebug() << "Send bonjour with MD5" << rec->md5.toHex();
        qDebug() << packet;

        sendPacket(packet);
        sharedRecords[rec] = true;
    }

    foreach(BonjourRecord* rec, sharedRecords.keys()) {
        if (!sharedRecords[rec]) {
            sendStopBonjour(rec->md5);
        }
    }


#if 0 /* TODO enable on OSX after tests */
    static bool first = true;
    if (first) {
        QTimer::singleShot(10000, this, SLOT(sendBonjour())); // first call, let the time to the Bonjour discoverer
        first = false;
    }
    QTimer::singleShot(BONJOUR_DELAY, this, SLOT(sendBonjour()));
#endif
}

void ControlPlaneConnection::sendStopBonjour(QString hash) {
    QString packet = "STOP\r\nMD5:" + hash + "\r\n\r\n";
    qDebug() << "Sending bonjour STOP to" << friendUid << "for hash" << hash;
    sendPacket(packet);
}

void ControlPlaneConnection::wasDisconnected() {
    ConnectionInitiator* init = ConnectionInitiator::getInstance();
    init->removeConnection(this);

    while (!proxyServers.isEmpty()) {
        Proxy* p = proxyServers.pop();
        delete p;
    }

    DataPlaneConnection* dp = init->getDpConnection(friendUid);
    if (dp) {
        dp->disconnect(); // we disconnect the dataplane for this same client!
    }

    this->deleteLater();
}
