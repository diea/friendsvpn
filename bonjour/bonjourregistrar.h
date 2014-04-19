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
    DNSServiceRef dnssref_pa;
    QSocketNotifier *bonjourSocket;
    QSocketNotifier *recordSocket;
    BonjourRecord finalRecord;
    DNSRecordRef dnsRecord;

    static void DNSSD_API bonjourRegisterService(
            DNSServiceRef, DNSServiceFlags,
            DNSServiceErrorType, const char *, const char *,
            const char *, void *);

    static void DNSSD_API registerRecordCallback(DNSServiceRef service, DNSRecordRef rec, const DNSServiceFlags flags,
                                                 DNSServiceErrorType errorCode, void *context);
public:
    explicit BonjourRegistrar(QObject *parent = 0);
    ~BonjourRegistrar();

    /**
     * @brief registerService: will register a "proxy service" (miming the dns-sd -P command) for
     * the hostname-ip & port combination in the record given as argument.
     *
     * WILL ONLY USE THE FIRST IP IN THE BONJOURRECORD IP LIST
     * Will do nothing if the record is not "resolved"
     * @param record
     */
    void registerService(const BonjourRecord &record);
    BonjourRecord registeredRecord() const { return finalRecord; }
signals:
    void error(DNSServiceErrorType error);
    void serviceRegistered(const BonjourRecord &record);
private slots:
    void bonjourSocketReadyRead();
    void recordSocketReadyRead();
    void handleError(DNSServiceErrorType error);
};

#endif // BONJOURREGISTRAR_H
