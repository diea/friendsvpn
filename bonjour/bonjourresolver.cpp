#include "bonjourresolver.h"
#include "bonjourdiscoverer.h"
#include "config.h"
#include "ipresolver.h"
#include "bonjourregistrar.h"
#include <arpa/inet.h>

BonjourResolver::BonjourResolver(BonjourRecord* record, QObject *parent) :
    QObject(parent)
{
    this->qSql = DatabaseHandler::getInstance();
    this->record = record;
    dnsref = 0;
}

void BonjourResolver::resolve() {
    // get ip & port using DNSServiceResolve
    DNSServiceErrorType err = DNSServiceResolve(&dnsref, 0, 0, record->serviceName.toUtf8().constData(),
                                                record->registeredType.toUtf8().constData(),
                                                record->replyDomain.toUtf8().constData(),
                                                resolveReply, this);
    if (err != kDNSServiceErr_NoError) {
        emit error(err);
    } else {
        int sockfd = DNSServiceRefSockFD(dnsref);
        if (sockfd == -1) {
            emit error(kDNSServiceErr_Invalid);
        } else {
            bonjourSocket = new QSocketNotifier(sockfd, QSocketNotifier::Read, this);
            connect(bonjourSocket, SIGNAL(activated(int)), this, SLOT(bonjourSocketReadyRead()));
        }
    }
}

void BonjourResolver::resolveReply(DNSServiceRef , //sdRef
                            DNSServiceFlags , //flags
                            uint32_t , // interfaceIndex
                            DNSServiceErrorType errorCode,
                            const char *, // fullname
                            const char *hosttarget,
                            quint16 port, /* In network byte order */
                            quint16 txtLen,
                            const unsigned char * txtRecord,
                            void *context) {
    if (!context) {
        qDebug() << "Context in resolving was NULL";
        return;
    }

    BonjourResolver* resolver = static_cast<BonjourResolver*>(context);

    resolver->bonjourSocket->disconnect();
    if (resolver->dnsref) {
        DNSServiceRefDeallocate(resolver->dnsref);
        resolver->dnsref = 0;
        if (resolver->bonjourSocket) delete resolver->bonjourSocket;
    }

    BonjourRecord* record = resolver->record;

    if (!record || !resolver) {
        qDebug() << "Record or resolver NULL";
        return;
    }
    if (record->resolved) {
        qDebug() << "Record has already been resolved";
        return;
    }

    if (errorCode != kDNSServiceErr_NoError) {
        emit resolver->error(errorCode);
    } else {
        if (txtLen > 1) {
            record->txt = QByteArray(static_cast<const char*>(static_cast<const void*>(txtRecord)), txtLen);
        }
        record->port = ntohs(port);
        qDebug() << "Lookup host!";
        /*QHostInfo::lookupHost(QString::fromUtf8(hosttarget), resolver,
                              SLOT(hostInfoReady(const QHostInfo &)));*/
        resolver->hostInfoReady(QHostInfo::fromName(QString::fromUtf8(hosttarget)));
    }
    //delete context_list; // remove allocated context list (@see resolve())
}

void BonjourResolver::bonjourSocketReadyRead() {
    DNSServiceErrorType err = DNSServiceProcessResult(dnsref);
    if (err != kDNSServiceErr_NoError)
        emit error(err);
}

void BonjourResolver::hostInfoReady(const QHostInfo &info) {
    QList<QString> v4;
    QList<QString> v6;
    qDebug() << "Full list of addresses " << info.addresses();
    foreach(QHostAddress adr, info.addresses()) {
        if (adr.protocol() == QAbstractSocket::IPv6Protocol) {
            QString adrstr = adr.toString();
            if (!adrstr.startsWith("fe80")) {
                v6.append(adrstr);
            }
        } else if (adr.protocol() == QAbstractSocket::IPv4Protocol) {
            v4.append(adr.toString());
        }
    }
#if 0 /* is superfluous but may be reactivated if mind change */
    if (v6.empty() && !v4.empty()) { // QHostInfo was not able to fetch ipv6
        // check that v4 ip is not a local one
        bool local = false;

        QProcess ifconfig;
        ifconfig.start("/sbin/ifconfig");
        if (ifconfig.waitForReadyRead()) {
            char buf[3000];
            int length;
            while ((length = ifconfig.readLine(buf, 3000))) {
                QString curLine(buf);
                QStringList list = curLine.split(" ", QString::SkipEmptyParts);
                foreach (QString element, list) {
                    if (element.contains(v4.at(0))) {
                        v6.append("::1");
                        local = true;
                    }
                }
            }
        }
        ifconfig.close();

        if (!local) { // get MAC from ARP table, send a neighbor discovery and check kernel table
            QList<QString> macs; // list of retrieve mac addresses
            QList<QString> ifaces; // for each mac we need to know on which interface it was
            QProcess arp;

            qDebug() << "checking arp table";
            arp.start("arp -an");
            int length;
            char buf[3000];

            if (arp.waitForReadyRead(200)) {
                while ((length = arp.readLine(buf, 3000))) {
                    QString curLine(buf);
                    QStringList list = curLine.split(" ", QString::SkipEmptyParts);
                    foreach (QString ipv4, v4) {
        #ifdef __APPLE__
                        if (list.length() > 5) {
                            if (list.at(1).contains(ipv4)) {
                                macs.append(list.at(3));
                                ifaces.append(list.at(5));
                            }
                        }
        #elif __GNUC__
                        if (list.length() > 6) {
                            if (list.at(1).contains(ipv4)) {
                                qDebug() << "Got MAC of ipv4" << list.at(3);
                                macs.append(list.at(3));
                                QString iface = list.at(6);
                                iface.truncate(iface.length() - 1);
                                ifaces.append(iface);
                            }
                        }
        #endif
                    }
                }
            }
            arp.close();
            for (int i = 0; i < macs.length(); i++) {
                // send an inverse neighbor discovery icmpv6 just in case no previous contact was made: DOES NOT WORK
                // RFC 3122 is a proposed standard and not yet supported.

                // we can however, send an ICMPv6 request to ff02::1 and the right mac address
                QProcess icmpReq;
                QStringList icmpReqArgs;
                icmpReqArgs.append(ifaces.at(i));
                icmpReqArgs.append(qSql->getLocalIP());
                icmpReqArgs.append(macs.at(i));
                icmpReq.start(QString(HELPERPATH) + "reqIp", icmpReqArgs);
                icmpReq.waitForFinished(500);

                // read from neighbor table.
                QThread::sleep(2); // wait two seconds for icmp response to arrive

                // now check neighbor table
                QProcess ndp;
        #ifdef __APPLE__
                ndp.start("ndp -an");
                ndp.waitForReadyRead(500);
                while ((length = ndp.readLine(buf, 3000))) {
                    QString curLine(buf);
                    QStringList list = curLine.split(" ", QString::SkipEmptyParts);

                    if (list.length() == 6) {
                        if (list.at(1) == macs.at(i)) {
                            QString ipv6 = list.at(0);
                            if (ipv6.startsWith("fe80")) {
                                // remove everything after %; % included
                                int percentIndex = ipv6.indexOf("%");
                                ipv6.truncate(percentIndex);
                            }

                            // at the same time we can add the mapping to our resolver
                            IpResolver* r = IpResolver::getInstance();
                            qDebug() << "Resolver adds mapping" << ipv6 << macs.at(i) << "interface" << ifaces.at(i);
                            r->addMapping(ipv6, macs.at(i), ifaces.at(i));

                            v6.append(ipv6);
                        }
                    }
                }
                ndp.close();
        #elif __GNUC__
                ndp.start("ip -6 neigh");
                ndp.waitForReadyRead();
                char buf[3000];
                int length;
                while ((length = ndp.readLine(buf, 3000))) {
                    QString curLine(buf);
                    QStringList list = curLine.split(" ", QString::SkipEmptyParts);
                    if (list.at(4) == macs.at(i)) {
                        QString ipv6 = list.at(0);

                        // at the same time we can add the mapping to our resolver
                        IpResolver* r = IpResolver::getInstance();
                        r->addMapping(ipv6, macs.at(i), ifaces.at(i));
                        qDebug() << "Got IPv6 from MAC" << ipv6;
                        v6.append(ipv6);
                    }
                }
                ndp.close();
        #endif
            }
        }
    }

#endif
    if (v6.empty()) {
        qDebug() << "Invalid record no IPv6 available, we don't use it!";
        return; // do not use record
    }

    if (!record) {
        qDebug() << "Record is null";
        return;
    }

    qDebug() << "Set hostname in record";
    record->hostname = info.hostName();
    qDebug() << "Copy v6 array";
    record->ips = v6;
    record->resolved = true;

    qDebug() << "Truncate after .local";
    // truncate everything after ".local"
    int indexLocal;
    if ((indexLocal = record->hostname.indexOf(".local")) > -1) {
        record->hostname.truncate(indexLocal + 6);
    }

    qDebug() << "Compute record hash";
    // compute record hash
    QString allParams = qSql->getLocalUid() + record->serviceName +
            record->registeredType + record->hostname + QString::number(record->port);
    qDebug() << "Going in hash";
    QByteArray hash = QCryptographicHash::hash(allParams.toUtf8().data(), QCryptographicHash::Md5);
    // add record to hashes list
    while (BonjourDiscoverer::recordHashes.contains(hash)) {
        qDebug() << "Impressive! record needs to be re-hashed";
        QByteArray toHash = hash + QByteArray::number(static_cast<int>(time(NULL)));
        hash = QCryptographicHash::hash(toHash, QCryptographicHash::Md5);
    }
    qDebug() << "Set record md5";
    record->md5 = hash;
    qDebug() << "Insert inside recordHashes";
    BonjourDiscoverer::recordHashes.insert(hash, record);

    QString transProt;
    qDebug() << "Getting transProt";
    if (record->registeredType.indexOf("tcp") > -1) {
        transProt = "tcp";
    } else {
        transProt = "udp";
    }
    qDebug() << "Going to insert in qSql";
    QString serviceName = record->registeredType;
    qDebug() << "insertService";
    qSql->insertService(serviceName, transProt);
    qDebug() << "insertDevice";
    qSql->insertDevice(record->hostname, record->port, serviceName, transProt, record->serviceName);
    qDebug() << "Emitting signal";
    emit resolved(record);

    /*qDebug() << "Resolver deletes itself";
    this->deleteLater();*/
}

