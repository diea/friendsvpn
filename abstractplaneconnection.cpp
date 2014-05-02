#include "abstractplaneconnection.h"

AbstractPlaneConnection::AbstractPlaneConnection(QString uid, QObject *parent) :
    QObject(parent), friendUid(uid)
{
	curMode = Closed;
}

void AbstractPlaneConnection::setMode(plane_mode m) {
    this->curMode = m;
    if (this->curMode != Closed)
        emit connected();
    else
        emit disconnected();
}

plane_mode AbstractPlaneConnection::getMode() {
    return curMode;
}

bool AbstractPlaneConnection::removeMode(plane_mode mode) {
    if (curMode == Closed) return false;
    if ((curMode != Both) && (curMode != mode)) return false;

    if (curMode == Both) {
        if (mode == Receiving) {
            curMode = Emitting;
            //serverSock = NULL;
        } else {
            curMode = Receiving;
            //clientSock = NULL;
        }
    } else {
        emit disconnected();
    }
    return true;
}
