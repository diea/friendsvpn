#include "sslsocket.h"
#include <QSslCipher>

SslSocket::~SslSocket() {
    SSL_free(ssl);         /* release SSL state */
    close(sd);          /* close connection */
}

SslSocket::SslSocket(SSL* ssl, QObject *parent) :
    ssl(ssl), QSslSocket(parent)
{
    con = NULL;

    if (SSL_accept(ssl) == FAIL)     /* do SSL-protocol accept */
        ERR_print_errors_fp(stderr);
    sd = SSL_get_fd(ssl);       /* get socket connection */
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
