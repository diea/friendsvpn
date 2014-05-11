#include "proxyserver.h"
#include <QCryptographicHash>
#include "bonjour/bonjourrecord.h"
#include "unixsignalhandler.h"

QString ProxyServer::computeMd5(const QString &friendUid, const QString &name, const QString &regType,
                                 const QString &domain, const QString &hostname, quint16 port) {
    QString allParams = friendUid + name + regType + domain + hostname + QString::number(port);

    QByteArray hash = QCryptographicHash::hash(allParams.toUtf8().data(), QCryptographicHash::Md5);
    return QString(hash);
}

ProxyServer::ProxyServer(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                       const QString &hostname, quint16 port) : Proxy(port, regType,
                                                                      computeMd5(friendUid, name, regType, domain, hostname, port))
{
    qDebug() << "Proxy constructor!";

    QString newip = listenIp;
    // create bonjour rec with new IP
    QList<QString> ip;
    ip.append(newip);
    rec = BonjourRecord(name, regType, domain, hostname, ip, port);

    // get DataPlaneConnection associated with friendUid
    ConnectionInitiator* initiator = ConnectionInitiator::getInstance();
    con = initiator->getDpConnection(friendUid);
}

void ProxyServer::run() {
    run_pcap();
    // if listening on other port, need to change dstPort of TCP/UDP packet before sending over DTLS

    /*
    QStringList sendRawArgs;
    sendRawArgs.append(QString::number(sockType));
    sendRawArgs.append(QString::number(ipProto));
    sendRawArgs.append("fd3b:e180:cbaa:1:156e:72ed:97d0:a60");

    connect(&sendRaw, SIGNAL(finished(int)), this, SLOT(sendRawFinish(int)));
    sendRaw.start(QString(HELPERPATH) + "sendRaw", sendRawArgs);
    */
    // advertise by registering the record with a bonjour registrar
    // TODO
}
