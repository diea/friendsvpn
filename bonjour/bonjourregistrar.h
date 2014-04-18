#ifndef BONJOURREGISTRAR_H
#define BONJOURREGISTRAR_H
#include "dns_sd.h"
#include "bonjourrecord.h"

#include <QObject>
#include <QSocketNotifier>

class BonjourRegistrar : public QObject
{
    Q_OBJECT
private:
    DNSServiceRef dnssref;
    DNSServiceRef dnsref_pa;
    QSocketNotifier *bonjourSocket;
    BonjourRecord finalRecord;
    DNSRecordRef dnsRecord = NULL;

    static void DNSSD_API bonjourRegisterService(
            DNSServiceRef, DNSServiceFlags,
            DNSServiceErrorType, const char *, const char *,
            const char *, void *);

    static void DNSSD_API registerRecordCallback(DNSServiceRef service, DNSRecordRef rec, const DNSServiceFlags flags,
                                                 DNSServiceErrorType errorCode, void *context);
public:
    explicit BonjourRegistrar(QObject *parent = 0);
    ~BonjourRegistrar();

    void registerService(const BonjourRecord &record,
                         quint16 servicePort);
    BonjourRecord registeredRecord() const { return finalRecord; }
signals:
    void error(DNSServiceErrorType error);
    void serviceRegistered(const BonjourRecord &record);
private slots:
    void bonjourSocketReadyRead();
};

#endif // BONJOURREGISTRAR_H
