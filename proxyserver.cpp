#include "proxyserver.h"
#include <QCryptographicHash>
#include "bonjour/bonjourrecord.h"
#include "unixsignalhandler.h"
#include <arpa/inet.h>

ProxyServer::~ProxyServer() {
    hostnames[hostnameUid].nb--;
    qDebug() << "There are" << hostnames[hostnameUid].nb << "proxy servers left for" << hostnameUid;
    if (hostnames[hostnameUid].nb == 0) {
        hostnames.remove(hostnameUid);
        UnixSignalHandler* u = UnixSignalHandler::getInstance();
        u->removeIp(listenIp);
    }
}

QHash<QString, struct ProxyServer::ip_and_nb> ProxyServer::hostnames;

ProxyServer::ProxyServer(const QString &friendUid, const QString &name,
                         const QString &regType, const QString &domain,
                         const QString &hostname, const QByteArray& txt,
                         quint16 port, const QByteArray& md5) : Proxy(port, regType, md5)
{
    hostnameUid = friendUid + hostname;
    if (hostnames.contains(hostnameUid)) {
        listenIp = hostnames[hostnameUid].ip;
        hostnames[hostnameUid].nb++;
    } else {
        listenIp = newIP();
        struct ip_and_nb ipnb;
        ipnb.ip = listenIp;
        ipnb.nb = 0;
        hostnames.insert(hostnameUid, ipnb);
    }
    QString newip = listenIp;
    // create bonjour rec with new IP
    QList<QString> ip;
    ip.append(newip);
    // we prepend "friendsvpn_" in front of names so we know when scanning to ignore those
    // we append the current ms time to hostname to avoid name collision
    QString username = DatabaseHandler::getInstance()->getName(friendUid);
    rec = BonjourRecord("fvpn_" + username + "_" + name, regType, domain,
                        "fvpn_"
                        + username + "_" + hostname, ip, port, txt);

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
    qDebug() << "New proxy server for " << rec.serviceName << "on " << listenIp << port;
}

void ProxyServer::sendBytes(char *buf, int len, QString dstIp) {
    rawSocks->writeBytes(rec.ips.at(0), dstIp, port, buf, sockType, len);
}

void ProxyServer::receiveBytes(const char* buf, int len, int sockType, QString srcIp) {
    con->sendBytes(buf, len, idHash, sockType, srcIp);
}
