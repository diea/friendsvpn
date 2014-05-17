#include "sslsocket.h"

SslSocket::SslSocket(QObject *parent) :
    QSslSocket(parent)
{
    con = NULL;
    setCiphers("eNULL:NULL");
}

void SslSocket::setControlPlaneConnection(ControlPlaneConnection *con) {
    this->con = con;
}

ControlPlaneConnection* SslSocket::getControlPlaneConnection() {
    return this->con;
}

bool SslSocket::isAssociated() {
    return (con != NULL);
}

SslSocket::~SslSocket() {
}
