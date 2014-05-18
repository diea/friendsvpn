#include "controlplaneconnection.h"
#include "connectioninitiator.h"
#include "ph2phtp_parser.h"
#include "proxyserver.h"
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

ControlPlaneConnection::ControlPlaneConnection(QString uid, AbstractPlaneConnection *parent) :
    AbstractPlaneConnection(uid, parent), lastRcvdTimestamp(time(NULL))
{
    qSql = BonjourSQL::getInstance();
    this->connect(this, SIGNAL(disconnected()), SLOT(wasDisconnected()));
    this->connect(this, SIGNAL(connected()), SLOT(sendBonjour()));
    this->connect(this, SIGNAL(connected()), SLOT(alive()));
}

ControlPlaneConnection::~ControlPlaneConnection() {
    qDebug() << "Destroyed control plane connection !";
}

void ControlPlaneConnection::alive() {
    QString alive = "PING\r\n\r\n";
    sendPacket(alive);
    QTimer::singleShot(5000, this, SLOT(aliveTimeout()));
}

void ControlPlaneConnection::aliveTimeout() {
    if (time(NULL) - lastRcvdTimestamp > 60) {
        qDebug() << "Connection with " << friendUid << "timed out";
        wasDisconnected(); // we disco
    }
}

void ControlPlaneConnection::sendPacket(QString& packet) {
    mutex.lock();
    QSslSocket* toWrite; // the socket on which the bonjour packets are to be sent
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

    toWrite->write(packet.toUtf8().data());
    mutex.unlock();
}

void ControlPlaneConnection::removeConnection() {
    if (friendUid < qSql->getLocalUid()) { // friend is smaller, I am server
        clientSock->disconnect();
        clientSock = NULL;
        if (curMode == Both)
            curMode = Receiving;
        else
            curMode = Closed;
    } else {
        serverSock->disconnect();
        serverSock = NULL;
        if (curMode == Both)
            curMode = Emitting;
        else
            curMode = Closed;
    }
}

bool ControlPlaneConnection::addMode(plane_mode mode, QObject *socket) {
    qDebug() << "adding mode " << mode << "controlplane";
    QSslSocket* sslSocket = dynamic_cast<QSslSocket*>(socket);
    if (!sslSocket) return false; // needs to be of type QSslSocket!

    if ((curMode == Both) || (curMode == mode)) return false;
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

    return true;
}

void ControlPlaneConnection::readBuffer(const char* buf, int len) {
    int bufferPosition = 0;
    lastRcvdTimestamp = time(NULL); // we received a packet, update time
    while (len > 0) {
        const char* found = my_strnstr(buf + bufferPosition, "\r\n\r\n", len);
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

        len -= headerLength + 4; // 4 count for the \r\n\r\n

        qDebug() << "headerlength is " << headerLength;
        QString packet = QString::fromLatin1(buf + bufferPosition, headerLength);
        bufferPosition += headerLength + 4; // skip to the next packet

        qDebug() << "packet before split" << packet;
        QStringList list = packet.split("\r\n");

        qDebug() << "new bjr packet" << list;
        if (list.at(0) == "BONJOUR") {
            QString hostname;
            QString name;
            QString type;
            int port = 0;
            for (int i = 1; i < list.length(); i++) {
                QStringList keyValuePair = list.at(i).split(":");
                QString key = keyValuePair.at(0);
                qDebug() << "key" << key;
                if (key == "Hostname") {
                    hostname = keyValuePair.at(1);
                } else if (key == "Name") {
                    name = keyValuePair.at(1);
                } else if (key == "Type") {
                    type = keyValuePair.at(1);
                } else if (key == "Port") {
                    port = keyValuePair.at(1).toInt();
                }
            }

            if (hostname.isEmpty() || name.isEmpty() || type.isEmpty() || !port) {
                qDebug() << "ERROR: Bonjour packet wrong format";
                return;
            }

            qDebug() << "Running new proxy!!";
            ProxyServer* newProxy = NULL;
            try {
                newProxy = new ProxyServer(friendUid, name, type, "", hostname, port);
                proxyServers.push(newProxy);
                newProxy->run();
            } catch (int i) {
                // proxy exists
            }
        } else if (list.at(0) == "PING") {
            QString pong = "PONG\r\n\r\n";
            sendPacket(pong);
        } else { // no need to analyze PONG, we just need a packet for the time :)
            qDebug() << "not starting with BONJOUR or PING";
        }
    }
}

void ControlPlaneConnection::sendBonjour() {
    // get bonjour records from db & send them over the connection
    //qDebug() << "fetching records for " << this->friendUid;
    QList < BonjourRecord* > records = qSql->getRecordsFor(this->friendUid);
    //qDebug() << "Retrieved " << records.length() << " records!";
    foreach (BonjourRecord* rec, records) {  
        QString packet;
        packet = packet
                 % "BONJOUR\r\n"
                 % "Hostname:" % rec->hostname % "\r\n"
                 % "Name:" % rec->serviceName % "\r\n"
                 % "Type:" % rec->registeredType % "\r\n"
                 % "Port:" % QString::number(rec->port) % "\r\n"
                % "\r\n";

        sendPacket(packet);
    }

    static bool first = true;
    if (first) {
        QTimer::singleShot(10000, this, SLOT(sendBonjour())); // first call, let the time to the Bonjour discoverer
        first = false;
    }
    QTimer::singleShot(300000, this, SLOT(sendBonjour())); // every 5 minutes | TODO make button to force!
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
