#include "bonjourregistrar.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <QDebug>

BonjourRegistrar::BonjourRegistrar(QObject *parent)
  : QObject(parent), dnssref(0), dnsref_pa(0), bonjourSocket(0)
{
}

BonjourRegistrar::~BonjourRegistrar()
{
    if (dnssref || dnsref_pa) {
        DNSServiceRefDeallocate(dnssref);
        DNSServiceRefDeallocate(dnsref_pa);
        dnssref = 0;
        dnsref_pa;
    }
}

void BonjourRegistrar::registerService(
    const BonjourRecord &record, quint16 servicePort) {
    if (dnssref || dnsref_pa) {
        qWarning("Already registered a service");
        return;
    }
    DNSServiceErrorType err = DNSServiceCreateConnection(&dnsref_pa);
    if (err != kDNSServiceErr_NoError) { emit error(err); }

    //err = RegisterProxyAddressRecord(dnsref_pa, "testHost", "fd9b:c51b:81ae::5e96:9dff:fe8a:8447", NULL);

    struct in6_addr newip6 = { };
    inet_pton(AF_INET6, "fd9b:c51b:81ae::5e96:9dff:fe8a:8447", &newip6);
    err = DNSServiceRegisterRecord(dnsref_pa, &dnsRecord, kDNSServiceFlagsUnique, 0, "testHost.local.", kDNSServiceType_AAAA,
                             kDNSServiceClass_IN, 16, &newip6, 0, registerRecordCallback, this); // XXX - Maybe add callback to check err
    if (err != kDNSServiceErr_NoError) { emit error(err); }

    quint16 bigEndianPort = servicePort;
    #if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    bigEndianPort = ((servicePort & 0x00ff) << 8)
                  | ((servicePort & 0xff00) >> 8);
    #endif
    err = DNSServiceRegister(&dnssref,
          0, 0, record.serviceName.toUtf8().constData(),
          record.registeredType.toUtf8().constData(),
          record.replyDomain.isEmpty() ? 0
                    : record.replyDomain.toUtf8().constData(),
          "testHost", bigEndianPort, 0, 0, bonjourRegisterService,
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
}

void BonjourRegistrar::registerRecordCallback(DNSServiceRef service, DNSRecordRef rec, const DNSServiceFlags flags,
                                              DNSServiceErrorType errorCode, void *context) {
    qDebug() << "Callback!";
}

void BonjourRegistrar::bonjourSocketReadyRead() {
    DNSServiceErrorType err = DNSServiceProcessResult(dnssref);
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

