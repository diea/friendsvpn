#include "bonjourregistrar.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <QDebug>

BonjourRegistrar::BonjourRegistrar(QObject *parent)
  : QObject(parent), dnssref(0), dnssref_pa(0), bonjourSocket(0), dnsRecord(0)
{
    connect(this, SIGNAL(error(DNSServiceErrorType)), this, SLOT(handleError(DNSServiceErrorType)));
}

BonjourRegistrar::~BonjourRegistrar()
{
    qDebug() << "freeing bonjour registrar";
    if (dnssref) {
        DNSServiceRefDeallocate(dnssref);
        dnssref = 0;
    }
    if (dnssref_pa) {
        DNSServiceRefDeallocate(dnssref_pa);
        dnssref_pa = 0;
    }
}

void BonjourRegistrar::registerService(const BonjourRecord &record) {
    qDebug() << "registering " << record.hostname;
    qDebug() << record.ips;
    qDebug() << record.port;
    qDebug() << record.registeredType;
    qDebug() << record.replyDomain;
    qDebug() << record.serviceName;
#ifdef __APPLE__ /* register is not supported by AVAHI compatibility layer */
    if (dnssref || dnssref_pa) {
        qWarning("Already registered a service");
        return;
    } else if (!record.resolved) {
        qWarning("Record was not resolved, aborting");
        return;
    }
    DNSServiceErrorType err = DNSServiceCreateConnection(&dnssref_pa);
    if (err != kDNSServiceErr_NoError) { emit error(err); }

    struct in6_addr newip6 = { };
    inet_pton(AF_INET6, record.ips.at(0).toUtf8().data(), &newip6);
    err = DNSServiceRegisterRecord(dnssref_pa, &dnsRecord, kDNSServiceFlagsUnique, 0,
                                   QString(record.hostname + "." + record.replyDomain).toUtf8().data(), kDNSServiceType_AAAA,
                             kDNSServiceClass_IN, 16, &newip6, 240, registerRecordCallback, this);
    if (err != kDNSServiceErr_NoError) { emit error(err); }
    else {
        int sockfd = DNSServiceRefSockFD(dnssref_pa);
        if (sockfd == -1) {
            emit error(kDNSServiceErr_Invalid);
        } else {
            recordSocket = new QSocketNotifier(sockfd, QSocketNotifier::Read, this);
            connect(recordSocket, SIGNAL(activated(int)), this, SLOT(recordSocketReadyRead()));
        }
    }

    quint16 bigEndianPort = record.port;
    #if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    bigEndianPort = ((record.port & 0x00ff) << 8)
                  | ((record.port & 0xff00) >> 8);
    #endif
    err = DNSServiceRegister(&dnssref,
          0, 0, record.serviceName.toUtf8().constData(),
          record.registeredType.toUtf8().constData(),
          record.replyDomain.isEmpty() ? 0
                    : record.replyDomain.toUtf8().constData(),
          QString(record.hostname + "." + record.replyDomain).toUtf8().data(), bigEndianPort,
                             record.txt.toUtf8().length(), record.txt.toUtf8().data(), bonjourRegisterService,
          this);
    if (err != kDNSServiceErr_NoError) {
        emit error(err);
    } else {
        int sockfd = DNSServiceRefSockFD(dnssref);
        if (sockfd == -1) {
            emit error(kDNSServiceErr_Invalid);
        } else {
            bonjourSocket = new QSocketNotifier(sockfd,
                                 QSocketNotifier::Read, this);
            connect(bonjourSocket, SIGNAL(activated(int)),
                  this, SLOT(bonjourSocketReadyRead()));
        }
    }
#endif
}

void BonjourRegistrar::registerRecordCallback(DNSServiceRef, DNSRecordRef, const DNSServiceFlags flags,
                                              DNSServiceErrorType errorCode, void *context) {
    qDebug() << "Callback!";
    BonjourRegistrar* registrar = static_cast<BonjourRegistrar*>(context);
    if (errorCode != kDNSServiceErr_NoError) {
        emit registrar->error(errorCode);
    }
    if (!(flags & kDNSServiceFlagsMoreComing)) fflush(stdout);
}

void BonjourRegistrar::handleError(DNSServiceErrorType error) {
    qDebug() << "BonjourRegistrar had error" << error;

    // could very for duplicate name error and not destruct (would allow to advertise using machine's hostname
    // miming the dns-sd -R command (the DNSRegisterRecord would fail)

    this->~BonjourRegistrar(); // call destructor to deallocate the dnssrefs
}

void BonjourRegistrar::bonjourSocketReadyRead() {
    DNSServiceErrorType err = DNSServiceProcessResult(dnssref);
    if (err != kDNSServiceErr_NoError)
        emit error(err);
}

void BonjourRegistrar::recordSocketReadyRead() {
    DNSServiceErrorType err = DNSServiceProcessResult(dnssref_pa);
    if (err != kDNSServiceErr_NoError)
        emit error(err);
}

void BonjourRegistrar::bonjourRegisterService(
     DNSServiceRef, DNSServiceFlags,
     DNSServiceErrorType errorCode, const char *name,
     const char *regType, const char *domain, void *data) {
    BonjourRegistrar *registrar = static_cast<BonjourRegistrar *>(data);
    if (errorCode != kDNSServiceErr_NoError) {
        emit registrar->error(errorCode);
    } else {
        registrar->finalRecord =
        BonjourRecord(QString::fromUtf8(name),
                      QString::fromUtf8(regType),
                      QString::fromUtf8(domain));
        emit registrar->serviceRegistered(registrar->finalRecord);
   }
}

