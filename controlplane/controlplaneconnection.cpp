#include "controlplaneconnection.h"
#include "connectioninitiator.h"
#include "ph2phtp_parser.h"

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
        curMode = Receiving;
    } else {
        serverSock->disconnect();
        serverSock = NULL;
        curMode = Emitting;
    }
}

bool ControlPlaneConnection::addMode(plane_mode mode, QObject *socket) {
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

void ControlPlaneConnection::readBuffer(const char* buf) {
    qDebug() << "Reading buffer";
    qDebug() << buf;
    PH2PHTP_Parser::parseControlPlane(buf, friendUid);
    qDebug() << "end of reading buffer";
}

void ControlPlaneConnection::sendBonjour() {
    //qDebug() << "sendBonjour() !";
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

        toWrite->write(packet.toUtf8().data());
    }

    //qDebug() << "Unlocking mutex";
    mutex.unlock();
    //qDebug("Leaving sendBonjour()");
    QTimer::singleShot(300000, this, SLOT(sendBonjour())); // every 5 minutes | TODO make button to force!
}

void ControlPlaneConnection::wasDisconnected() {
    qDebug() << "Geting init instance";
    ConnectionInitiator* init = ConnectionInitiator::getInstance();
    qDebug() << "Removing myself from the list";
    init->removeConnection(this);
    qDebug () << "emitting disconnected";
}
