#include "sslsocket.h"
#include <QSslCipher>

SslSocket::SslSocket(QObject *parent) :
    QSslSocket(parent)
{
    con = NULL;
    setCiphers("AES256-SHA");
    QList<QSslCipher> ciphers = QSslSocket::ciphers();
    qDebug() << "LENGTH OF ciphers" << ciphers.length();
    foreach (QSslCipher cipher, ciphers) {
        qDebug() << cipher.name();
    }
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
