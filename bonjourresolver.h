#ifndef BONJOURRESOLVER_H
#define BONJOURRESOLVER_H
#include "bonjourrecord.h"
#include "dns_sd.h"
#include "bonjoursql.h"
#include <QObject>
#include <QSocketNotifier>
#include <QHostInfo>

class BonjourResolver : public QObject
{
    Q_OBJECT
private:
    DNSServiceRef dnsref;
    QSocketNotifier* bonjourSocket;
    BonjourRecord* record;
    BonjourSQL* qSql;

    static void DNSSD_API resolveReply(DNSServiceRef sdRef,
                             DNSServiceFlags flags,
                             uint32_t interfaceIndex,
                             DNSServiceErrorType errorCode,
                             const char *fullname,
                             const char *hosttarget,
                             uint16_t port, /* In network byte order */
                             uint16_t txtLen,
                             const unsigned char *txtRecord,
                             void *context);

public:
    explicit BonjourResolver(BonjourSQL* qSql, BonjourRecord* record, QObject *parent = 0);

    void resolve();
signals:
    void error(DNSServiceErrorType err);
    void resolved(BonjourRecord*);

private slots:
    void bonjourSocketReadyRead();
    void hostInfoReady(const QHostInfo &info);

};

#endif // BONJOURRESOLVER_H
