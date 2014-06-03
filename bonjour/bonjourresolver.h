#ifndef BONJOURRESOLVER_H
#define BONJOURRESOLVER_H
#include "bonjourrecord.h"
#include "dns_sd.h"
#include "databasehandler.h"
#include <QObject>
#include <QSocketNotifier>
#include <QHostInfo>
/**
 * @brief The BonjourResolver class is used to resolve a BonjourRecord, that means getting
 * the hostname, the ip and port of a given BonjourRecord.
 */
class BonjourResolver : public QObject
{
    Q_OBJECT
private:
    DNSServiceRef dnsref;
    QSocketNotifier* bonjourSocket;
    BonjourRecord* record;
    DatabaseHandler* qSql;
    /**
      * @brief resolveReply is a callback function to DNSServiceResolve. It will fill in the
      * missing information in a record->
      */
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
    explicit BonjourResolver(BonjourRecord* record, QObject *parent = 0);

    void resolve();
signals:
    void error(DNSServiceErrorType err);
    /**
     * @brief resolved is emitted when the record was fully resolved
     */
    void resolved(BonjourRecord*);

private slots:
    void bonjourSocketReadyRead();
    /**
     * @brief hostInfoReady is a private slot used by resolveReply to get more information on a host,
     * to avoid getting stuck in the code.
     * This function emits the "resolved" signal.
     * @param info the new available QHostInfo
     */
    void hostInfoReady(const QHostInfo &info);

};

#endif // BONJOURRESOLVER_H
