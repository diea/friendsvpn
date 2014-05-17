#include "controlplaneconnection.h"
#include "connectioninitiator.h"
#include "ph2phtp_parser.h"
#include "proxyserver.h"

#include <QDebug>
ControlPlaneConnection::ControlPlaneConnection(QString uid, AbstractPlaneConnection *parent) :
    AbstractPlaneConnection(uid, parent)
{
    qSql = BonjourSQL::getInstance();
    this->connect(this, SIGNAL(disconnected()), SLOT(wasDisconnected()));
    this->connect(this, SIGNAL(connected()), SLOT(sendBonjour()));
}

ControlPlaneConnection::~ControlPlaneConnection() {
    qDebug() << "Destroyed control plane connection !";
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
    while (len > 0) {
        qDebug() << "len is" << len;
        char headLen[20];
        memset(&headLen, 0, sizeof(headLen));
        int j = 0;
        while (buf[bufferPosition] != '|') { // get header length, packet starts with "|"
            headLen[j] = buf[bufferPosition];
            len--;
            if (len < 0) {
                // not a valid packet in buffer
                qDebug() << "Not a valid packet in buffer";
                return;
            }
            bufferPosition++;
            j++;
        }
        int headerLength = atoi(headLen);
        len -= headerLength + 1; // "+1" is to ignore the character | that is not counted

        qDebug() << "headerlength is " << headerLength;
        QString packet = QString::fromLatin1(buf + (++bufferPosition), headerLength); // skip the "|" so ++bufferPosition
        bufferPosition += headerLength; // skip to the next packet

        qDebug() << "packet before split" << packet;
        QStringList list = packet.split("\r\n");

        qDebug() << "new bjr packet" << list;

        if (!list.at(list.length() - 1).isEmpty()) {
             qDebug() << "ERROR: Did not receive full control plane packet";
             return;
        }
        if (list.at(0) == "BONJOUR") {
            QString hostname;
            QString name;
            QString type;
            int port = 0;
            int listLength = list.length();
            for (int i = 0; i < listLength - 1; i++) {
                QStringList keyValuePair = list.at(i).split(":");
                QString key = keyValuePair.at(0);
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
                newProxy = new ProxyServer(friendUid, name, type, ".friendsvpn.", hostname, port);
                proxyServers.push(newProxy);
                newProxy->run();
            } catch (int i) {
                // proxy exists
            }
        } else {
            qDebug() << "not starting with BONJOUR";
        }
    }
}

void ControlPlaneConnection::sendBonjour() {
    static QMutex mutex; // XXX should maybe mutex more than this function (with the mode that can change)
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

        int nbBytes = packet.toUtf8().length();
        packet = QString::number(nbBytes) % "|" % packet;

        toWrite->write(packet.toUtf8().data());
    }
    mutex.unlock();
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
