#include "proxy.h"

Proxy::Proxy(const QString &name, const QString &regType, const QString &domain,
             const QString &hostname, quint16 port, QObject *parent) :
    QObject(parent)
{
    // generate random ULA
    // add it with ifconfig
    QProcess ifconfig;
    ifconfig.start("ifconfig en1 inet6 add fd3b:e180:cbaa:1:5e96:9dff:fe8a:8448");
    ifconfig.waitForReadyRead();
    qDebug() << ifconfig.readAllStandardOutput();
    qDebug() << ifconfig.readAllStandardError();
}

void Proxy::run() {
    // create raw socket to listen on this new IP
    // receive packet, forward it on the secure UDP link
}

QString Proxy::randomULA() {
    BonjourSQL* qSql = BonjourSQL::getInstance();

    //qsrand(qSql->getLocalUid()); // use UID for seed
    // do this when rest is working.
    return "fd3b:e180:cbaa:1:5e96:9dff:fe8a:8448";
}
