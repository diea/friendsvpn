#include "controlplaneconnection.h"

ControlPlaneConnection::ControlPlaneConnection(QObject *parent) :
    QObject(parent)
{
    curMode = Closed;
    curState = Wait_uid;

    this->connect(this, SIGNAL(disconnected()), SLOT(deleteLater()));
}

void ControlPlaneConnection::setMode(plane_mode m) {
    this->curMode = m;
    if (this->curMode != Closed)
        emit connected();
    else
        emit disconnected();
}

void ControlPlaneConnection::setState(plane_state s) {
    this->curState = s;
    if (this->curState != Wait_uid)
        emit uid_received();
}

plane_mode ControlPlaneConnection::getMode() {
    return curMode;
}

plane_state ControlPlaneConnection::getState() {
    return curState;
}

void ControlPlaneConnection::setUid(QString uid) {
    this->friendUid = uid;
    this->setState(Wait_data);
}

QString ControlPlaneConnection::getUid() {
    return this->friendUid;
}

void ControlPlaneConnection::readBuffer(const char* buf) {

}

bool ControlPlaneConnection::operator=(const ControlPlaneConnection& other) {
    return this->friendUid == other.friendUid;
}
