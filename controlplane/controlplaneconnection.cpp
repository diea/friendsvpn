#include "controlplaneconnection.h"
#include "connectioninitiator.h"

#include <QDebug>
ControlPlaneConnection::ControlPlaneConnection(QString uid, QObject *parent) :
    QObject(parent), friendUid(uid)
{
    curMode = Closed;
    qSql = BonjourSQL::getInstance();
    this->connect(this, SIGNAL(disconnected()), SLOT(deleteLater()));
    this->connect(this, SIGNAL(connected()), SLOT(sendBonjour()));
}

void ControlPlaneConnection::setMode(plane_mode m) {
    this->curMode = m;
    if (this->curMode != Closed)
        emit connected();
    else
        emit disconnected();
}

plane_mode ControlPlaneConnection::getMode() {
    return curMode;
}

void ControlPlaneConnection::removeConnection() {
    if (friendUid < qSql->getLocalUid()) { // friend is smaller, I am server
        clientSock->disconnect();
        clientSock = NULL;
        curMode = Server_mode;
    } else {
        serverSock->disconnect();
        serverSock = NULL;
        curMode = Client_mode;
    }
}

bool ControlPlaneConnection::addMode(plane_mode mode, QSslSocket *socket) {
    if ((curMode == Both_mode) || (curMode == mode)) return false;
    if (mode == Server_mode)
        serverSock = socket;
    else clientSock = socket;

    if (curMode == Closed)  {
        curMode = mode;
        emit connected();
    }
    else curMode = Both_mode;

    if (curMode == Both_mode)
        this->removeConnection();
}

bool ControlPlaneConnection::removeMode(plane_mode mode) {
    if (curMode == Closed) return false;
    if ((curMode != Both_mode) && (curMode != mode)) return false;

    if (curMode == Both_mode) {
        if (mode == Server_mode) {
            curMode = Client_mode;
            serverSock = NULL;
        } else {
            curMode = Server_mode;
            clientSock = NULL;
        }
    } else {
        curMode = Closed;
        qDebug() << "Geting init instance";
        ConnectionInitiator* init = ConnectionInitiator::getInstance();
        qDebug() << "Removing myself from the list";
        init->removeConnection(this);
        qDebug () << "emitting disconnected";
        emit disconnected();
    }
    return true;
}

QString ControlPlaneConnection::getUid() {
    return this->friendUid;
}

void ControlPlaneConnection::readBuffer(const char* buf) {
    qDebug() << "Reading buffer";
    qDebug() << buf;
    qDebug() << "end of reading buffer";
}

void ControlPlaneConnection::sendBonjour() {
    qDebug() << "sendBonjour() !";
    static QMutex mutex; // XXX should maybe mutex more than this function (with the mode that can change)
    mutex.lock();
    qDebug() << "passed the lock in sendbonjour";
    if (curMode == Client_mode) {
        qDebug() << "Sending BONJOUR PACKET !!!!";
        clientSock->write("BONJOUR PACKET");
        //clientSock->flush();
    } else {
        qDebug() << "Writing!";
        serverSock->write("SERVER BJR PACKET");
        qDebug() << "Flushing!";
        //serverSock->flush();
    }
    qDebug() << "Unlocking mutex";
    mutex.unlock();
    qDebug("Leaving sendBonjour()");
}

bool ControlPlaneConnection::operator=(const ControlPlaneConnection& other) {
    return this->friendUid == other.friendUid;
}
