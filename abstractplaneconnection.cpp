#include "abstractplaneconnection.h"
#include <QDebug>
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
    if (curMode == Closed) {
        return false;
    }
    if ((curMode != Both) && (curMode != mode)) {
        return false;
    }

    if (curMode == Both) {
        if (mode == Receiving) {
            curMode = Emitting;
        } else {
            curMode = Receiving;
        }
    } else {
        curMode = Closed;
        emit disconnected();
    }
    return true;
}

QString AbstractPlaneConnection::getUid() {
    return friendUid;
}
