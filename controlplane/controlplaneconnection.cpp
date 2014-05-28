#include "controlplaneconnection.h"
#include "connectioninitiator.h"
#include "proxyserver.h"
#include "sslsocket.h"
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
  for (cp = string + len - 4; cp >= string; cp--)
  {
    if (strncmp(cp, find, 4) == 0)
        return cp;
  }
  return NULL;
}

ControlPlaneConnection::ControlPlaneConnection(QString uid, AbstractPlaneConnection *parent) :
    AbstractPlaneConnection(uid, parent), lastRcvdTimestamp(time(NULL))
{
    qSql = BonjourSQL::getInstance();
    this->connect(this, SIGNAL(disconnected()), SLOT(wasDisconnected()));
    this->connect(this, SIGNAL(connected()), SLOT(sendBonjour()));
    this->connect(this, SIGNAL(connected()), SLOT(alive()));
    serverSock = NULL;
    clientSock = NULL;
    inputBuffer = static_cast<char*>(malloc(40000 * sizeof(char)));
    gotHello = false;
    lastFullPacket = 0;
    bytesReceived = 0;
}

ControlPlaneConnection::~ControlPlaneConnection() {
    qDebug() << "Destroyed control plane connection !";
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
    qDebug() << "Setting timeout";
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
    //qDebug() << "passed the lock in sendbonjour";

    if (curMode == Closed) {
        qDebug() << "trying to write bonjour but mode is Closed";
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
    qDebug() << "adding mode " << mode << "controlplane";
    SslSocket* sslSocket = dynamic_cast<SslSocket*>(socket);
    if (!sslSocket) {
        qDebug() << "add mode fail 1 controlplane";
        return false; // needs to be of type QSslSocket!
    }

    if ((curMode == Both) || (curMode == mode)) {
        qDebug() << "add mode fail 2 controlplane";
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

    qDebug() << "returning from add mode with mode " << curMode;
    return true;
}

void ControlPlaneConnection::readBuffer(const char* buf, int len) {
    memcpy(inputBuffer + bytesReceived, buf, len); // copy incoming buffer
    if (len > 0) {
        bytesReceived += len;
        if ((inputBuffer[bytesReceived - 1] == '\n') && (inputBuffer[bytesReceived - 2] == '\r')
                && (inputBuffer[bytesReceived - 3] == '\n') && (inputBuffer[bytesReceived - 4] == '\r')) {
            lastFullPacket = bytesReceived;
        } else {
            char* packetEnd = strrstr(inputBuffer, "\r\n\r\n", bytesReceived) + 4;
            int packetEndIndex = packetEnd - buf;
            if (packetEndIndex > 4) {
                // one full packet
                lastFullPacket = packetEndIndex;
            }
        }
    }

    int ok_len = lastFullPacket; // we only read until the last complete packet
    int bufferPosition = 0;
    lastRcvdTimestamp = time(NULL); // we received a packet, update time
    while (ok_len > 0) {
        const char* found = my_strnstr(buf + bufferPosition, "\r\n\r\n", ok_len);
        int headerLength = 0;
        if (found) {
            headerLength = found - (buf + bufferPosition);
        } else {
            qDebug() << "No \r\n\r\n in packet";
            return;
        }

        if (!headerLength) {
            qDebug() << "ERROR: this is packet when header 0";
            qDebug() << buf + bufferPosition;
            return;
        }

        ok_len -= headerLength + 4; // 4 count for the \r\n\r\n

        qDebug() << "headerlength is " << headerLength;
        QString packet = QString::fromUtf8(buf + bufferPosition, headerLength);
        bufferPosition += headerLength + 4; // skip to the next packet

        qDebug() << "packet before split" << packet;
        QStringList list = packet.split("\r\n");

        qDebug() << "new bjr packet" << list;
        QString packetType = list.at(0);
        if (!gotHello && packetType == "HELLO") {
            QStringList disec = list.at(1).split(":");
            if (!(disec.length() < 2)) {
                if (this->friendUid != disec.at(1)) {
                    qDebug() << "receive wrong UID for connection!";
                    wasDisconnected();
                    return;
                } else {
                    qDebug() << "Got hello!";
                    gotHello = true;
                }
            }
        }
        if (gotHello) {
            if (packetType == "BONJOUR") {
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

                if (hostname.isEmpty() || name.isEmpty() || type.isEmpty() || !port) {
                    qDebug() << "ERROR: Bonjour packet wrong format";
                    return;
                }

                qDebug() << "Running new proxy!!";
                ProxyServer* newProxy = NULL;
                try {
                    newProxy = new ProxyServer(friendUid, name, type, "", hostname, txt, port, QByteArray::fromHex(md5.toUtf8()));
                    proxyServers.push(newProxy);
                    newProxy->run();
                } catch (int i) {
                    // proxy exists
                    qDebug() << "proxy exists!";
                }
            } else if (packetType == "PING") {
                QString pong = "PONG\r\n\r\n";
                sendPacket(pong);
            } else if (packetType == "PONG") {
            } else { // no need to analyze PONG, we just need a packet for the time :)
                qDebug() << "not starting with BONJOUR or PING";
                qDebug() << packet;
            }
        }
    }

    // handle incomplete packets
    bytesReceived -= lastFullPacket;
    qDebug() << "bytesReceived is at " << bytesReceived;
    if (bytesReceived > 0) {
        // we have an incomplete packet at the end
        qDebug() << "going into memmove";
        memmove(inputBuffer, inputBuffer + lastFullPacket, bytesReceived); // move everything back at start of buffer
    }
}

void ControlPlaneConnection::sendBonjour() {
    // get bonjour records from db & send them over the connection
    //qDebug() << "fetching records for " << this->friendUid;
    QList < BonjourRecord* > records = qSql->getRecordsFor(this->friendUid);
    qDebug() << "Retrieved " << records.length() << " records!";
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
            packet = packet % "TXT:" % rec->txt % "\r\n";
        }

        packet = packet % "\r\n";

        qDebug() << "Sending" << packet;

        sendPacket(packet);
    }

    static bool first = true;
    if (first) {
        QTimer::singleShot(10000, this, SLOT(sendBonjour())); // first call, let the time to the Bonjour discoverer
        first = false;
    }
    QTimer::singleShot(BONJOUR_DELAY, this, SLOT(sendBonjour())); // TODO make button to force!
}

void ControlPlaneConnection::wasDisconnected() {
    qDebug() << "Geting init instance";
    ConnectionInitiator* init = ConnectionInitiator::getInstance();
    qDebug() << "Removing myself from the list";
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
