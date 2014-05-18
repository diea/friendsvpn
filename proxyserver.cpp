#include "proxyserver.h"
#include <QCryptographicHash>
#include "bonjour/bonjourrecord.h"
#include "unixsignalhandler.h"
#include <arpa/inet.h>

QString ProxyServer::computeMd5(const QString &friendUid, const QString &name, const QString &regType,
                                 const QString &domain, const QString &hostname, quint16 port) {
    QString allParams = friendUid + name + regType + hostname + QString::number(port);

    QByteArray hash = QCryptographicHash::hash(allParams.toUtf8().data(), QCryptographicHash::Md5).toHex();
    return QString(hash);
}

ProxyServer::ProxyServer(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                       const QString &hostname, quint16 port) : Proxy(port, regType,
                                                                      computeMd5(friendUid, name, regType, domain, hostname, port))
{
    QString newip = listenIp;
    // create bonjour rec with new IP
    QList<QString> ip;
    ip.append(newip);
    // we prepend "friendsvpn_" in front of names so we know when scanning to ignore those
    // we append the current ms time to hostname to avoid name collision
    rec = BonjourRecord("friendsvpn_" + name, regType, domain,
                        QString::number(QDateTime::currentMSecsSinceEpoch())+"_"+hostname, ip, port);

    // get DataPlaneConnection associated with friendUid
    ConnectionInitiator* initiator = ConnectionInitiator::getInstance();
    con = initiator->getDpConnection(friendUid);
    rawSocks = RawSockets::getInstance();
}

void ProxyServer::run() {
    run_pcap();
    rec.port = port; /* change to actual listen port before advertising */
    // advertise by registering the record with a bonjour registrar
    registrar.registerService(rec);
}

void ProxyServer::sendBytes(const char *buf, int len, QString dstIp) {
    rawSocks->writeBytes(rec.ips.at(0), dstIp, port, buf, sockType, len);
}

void ProxyServer::receiveBytes(const char* buf, int len, int sockType, QString& srcIp) {
    con->sendBytes(buf, len, idHash, sockType, srcIp);
}
