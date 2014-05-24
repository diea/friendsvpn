#include "sslsocket.h"
#include <QSslCipher>

SslSocket::~SslSocket() {
    SSL_free(ssl);         /* release SSL state */
    ::close(sd);          /* close connection */
}

SslSocket::SslSocket(SSL* ssl, QObject *parent) :
    QObject(parent), ssl(ssl)
{
    con = NULL;
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

void SslSocket::startServerEncryption() {
    int accRet = SSL_ERROR_WANT_READ;
    while (accRet == SSL_ERROR_WANT_READ) {
        accRet = SSL_accept(ssl);
        if (accRet != 1) {     /* do SSL-protocol accept */
            qDebug() << "start server encryption error" << accRet;
            qDebug() << "error code " << SSL_get_error(ssl, accRet);
            ERR_print_errors_fp(stdout);
            fflush(stdout);
        }
    }

    sd = SSL_get_fd(ssl);       /* get socket connection */
    notif = new QSocketNotifier(sd, QSocketNotifier::Read);
    connect(notif, SIGNAL(activated(int)), this, SLOT(emitRead(int)));
    emit encrypted();
}

void SslSocket::startClientEncryption() {
    int accRet = SSL_ERROR_WANT_READ;

    while (accRet == SSL_ERROR_WANT_READ) {
        accRet = SSL_connect(ssl);
        if (accRet != 1) {    /* do SSL-protocol accept */
            qDebug() << "start client encryption error";
            qDebug() << "start server encryption error" << accRet;
            qDebug() << "error code " << SSL_get_error(ssl, accRet);

            ERR_print_errors_fp(stdout);
            fflush(stdout);
        }
    }
    sd = SSL_get_fd(ssl);       /* get socket connection */
    notif = new QSocketNotifier(sd, QSocketNotifier::Read);
    connect(notif, SIGNAL(activated(int)), this, SLOT(emitRead(int)));
    emit encrypted();
}

void SslSocket::emitRead(int) {
    emit readyRead();
}

void SslSocket::write(const char *buf, int size) {
    if (!(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN)) {
        SSL_write(ssl, buf, size);
    } else {
        emit disconnected();
    }
}

int SslSocket::read(char* buf, int maxBytes) {
    if (!(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN)) {
        return SSL_read(ssl, buf, maxBytes);
    } else {
        emit disconnected();
        return 0;
    }
}

void SslSocket::close() {
    SSL_shutdown(ssl);
}
