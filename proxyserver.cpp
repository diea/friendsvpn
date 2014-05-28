#include "proxyserver.h"
#include <QCryptographicHash>
#include "bonjour/bonjourrecord.h"
#include "unixsignalhandler.h"
#include <arpa/inet.h>

ProxyServer::~ProxyServer() {
}

ProxyServer::ProxyServer(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                       const QString &hostname, const QByteArray& txt, quint16 port, const QByteArray& md5) : Proxy(port, regType, md5)
{
    QString newip = listenIp;
    // create bonjour rec with new IP
    QList<QString> ip;
    ip.append(newip);
    // we prepend "friendsvpn_" in front of names so we know when scanning to ignore those
    // we append the current ms time to hostname to avoid name collision
    QString username = BonjourSQL::getInstance()->getName(friendUid);
    rec = BonjourRecord("fvpn_" + username + "_" + name, regType, domain,
                        QString::number(QDateTime::currentMSecsSinceEpoch())+"_"+hostname, ip, port, txt);

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
