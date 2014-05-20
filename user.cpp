#include "user.h"

User::User(QString* uid, QString* ipv6, QSslCertificate* cert, QObject *parent) :
    QObject(parent)
{
    this->uid = uid;
    this->ipv6 = ipv6;
    this->cert = cert;
}

User::~User() {
    delete uid;
    delete ipv6;
    delete cert;
}
