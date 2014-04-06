#include "controlplaneconnection.h"
#include <QDebug>
ControlPlaneConnection::ControlPlaneConnection(QString uid, QObject *parent) :
    QObject(parent), friendUid(uid)
{
    curMode = Closed;

    this->connect(this, SIGNAL(disconnected()), SLOT(deleteLater()));
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

void ControlPlaneConnection::setUid(QString uid) {
    this->friendUid = uid;
}

QString ControlPlaneConnection::getUid() {
    return this->friendUid;
}

void ControlPlaneConnection::readBuffer(const char* buf) {
    qDebug() << "Reading buffer";
    qDebug() << buf;
    qDebug() << "end";
}

bool ControlPlaneConnection::operator=(const ControlPlaneConnection& other) {
    return this->friendUid == other.friendUid;
}
