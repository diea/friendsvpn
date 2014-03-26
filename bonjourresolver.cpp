#include "bonjourresolver.h"

BonjourResolver::BonjourResolver(BonjourSQL* qSql, BonjourRecord* record, QObject *parent) :
    QObject(parent)
{
    this->qSql = qSql;
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
    v6.append(v4);
    record->ips = v6;
    record->resolved = true;
    record->hostname = info.hostName();

    // truncate everything after ".local"
    int indexLocal;
    if ((indexLocal = record->hostname.indexOf(".local")) > -1) {
        record->hostname.truncate(indexLocal + 6);
    }

    QString transProt;
    if (record->registeredType.indexOf("tcp") > -1) {
        transProt = "tcp";
    } else { // XXX watch out, other trans prot ?
        transProt = "udp";
    }
    QString serviceName = record->registeredType;
    serviceName.chop(6);
    qSql->insertDevice(record->hostname, record->port, serviceName, transProt, record->serviceName);
    emit resolved(record);
    delete this;
}

