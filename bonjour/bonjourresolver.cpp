#include "bonjourresolver.h"
#include "bonjourdiscoverer.h"
#include "config.h"
#include "ipresolver.h"

BonjourResolver::BonjourResolver(BonjourRecord* record, QObject *parent) :
    QObject(parent)
{
    this->qSql = BonjourSQL::getInstance();
    this->record = record;
    dnsref = 0;
}

void BonjourResolver::resolve() {
    // get ip & port using DNSServiceResolve
    QList<void*>* context = new QList<void*>();
    context->append(record);
    context->append(this);
    //qDebug() << "Calling DNS Service Resolve";
    DNSServiceErrorType err = DNSServiceResolve(&dnsref, 0, 0, record->serviceName.toUtf8().constData(),
                                                record->registeredType.toUtf8().constData(),
                                                record->replyDomain.toUtf8().constData(),
                                                resolveReply, context);
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
                            quint16 , //txtLen
                            const unsigned char *, //txtRecord
                            void *context) {
    QList<void*>* context_list = static_cast<QList<void*>*>(context);
    BonjourRecord* record = static_cast<BonjourRecord*>(context_list->at(0));
    BonjourResolver* resolver = static_cast<BonjourResolver*>(context_list->at(1));
    if (errorCode != kDNSServiceErr_NoError) {
        emit resolver->error(errorCode);
    } else {

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        {
            port  =  0 | ((port & 0x00ff) << 8) | ((port & 0xff00) >> 8);
        }
#endif
        record->port = port;
        QHostInfo::lookupHost(QString::fromUtf8(hosttarget), resolver,
                              SLOT(hostInfoReady(const QHostInfo &)));
    }
    delete context_list; // remove allocated context list (@see resolve())
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
            v6.append(adr.toString());
        } else if (adr.protocol() == QAbstractSocket::IPv4Protocol) {
            v4.append(adr.toString());
        }
    }

    qDebug() << "Going to test v6.empty";
    if (v6.empty()) { // QHostInfo was not able to fetch ipv6
        qDebug() << "record hostname" << info.hostName();
        qDebug() << "local hostname" << QHostInfo::localDomainName();
        if (info.hostName() == QHostInfo::localDomainName()) {
            // just use ::1
            qDebug() << "use ::1";
            v6.append("::1");
        } else {
            if (!v4.empty()) { // get MAC from ARP table, send a neighbor discovery and check kernel table
                QList<QString> macs; // list of retrieve mac addresses
                QList<QString> ifaces; // for each mac we need to know on which interface it was
                QProcess arp;
                qDebug() << "checking arp table";
                arp.start("arp -an");
                arp.waitForReadyRead(200);
                char buf[3000];
                int length;
                while ((length = arp.readLine(buf, 3000))) {
                    QString curLine(buf);
                    QStringList list = curLine.split(" ", QString::SkipEmptyParts);
                    foreach (QString ipv4, v4) {
                        if (list.at(1).contains(ipv4)) {
                            macs.append(list.at(3));
#ifdef __APPLE__
                            ifaces.append(list.at(5));
#elif __GNUC__
                            ifaces.append(list.at(6));
#endif
                        }
                    }
                }
                arp.close();
                qDebug() << "arp table done, macs:" << macs;
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
                    ndp.waitForReadyRead();
                    while ((length = ndp.readLine(buf, 3000))) {
                        QString curLine(buf);
                        QStringList list = curLine.split(" ", QString::SkipEmptyParts);
                        if (list.at(1) == macs.at(i)) {
                            QString ipv6 = list.at(0);
                            if (ipv6.startsWith("fe80")) {
                                // remove everything after %; % included
                                int percentIndex = ipv6.indexOf("%");
                                ipv6.truncate(percentIndex);
                            }

                            // at the same time we can add the mapping to our resolver
                            IpResolver* r = IpResolver::getInstance();
                            r->addMapping(ipv6, macs.at(i), ifaces.at(i));

                            v6.append(ipv6);
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

                            v6.append(ipv6);
                        }
                    }
                    ndp.close();
            #endif
                }
            }
        }
    }

    record->ips = v6;

    if (v6.empty()) {
        // TODO maybe set record as unusable in database
        qDebug() << "Invalid record, we don't use it!";
        return; // do not use record
    }

    record->resolved = true;
    record->hostname = info.hostName();

    // truncate everything after ".local"
    int indexLocal;
    if ((indexLocal = record->hostname.indexOf(".local")) > -1) {
        record->hostname.truncate(indexLocal + 6);
    }

    // compute record hash
    QString allParams = qSql->getLocalUid() + record->serviceName +
            record->registeredType + record->hostname + QString::number(record->port);
    QByteArray hash = QCryptographicHash::hash(allParams.toUtf8().data(), QCryptographicHash::Md5);
    // add record to hashes list
    //BonjourDiscoverer::recordHashes.insert(QString(hash), record);
    BonjourDiscoverer::recordHashes.insert(QString(allParams), record);

    QString transProt;
    if (record->registeredType.indexOf("tcp") > -1) {
        transProt = "tcp";
    } else {
        transProt = "udp";
    }
    QString serviceName = record->registeredType;
    serviceName.chop(6);
    qSql->insertDevice(record->hostname, record->port, serviceName, transProt, record->serviceName);
    emit resolved(record);
    this->deleteLater();
}

