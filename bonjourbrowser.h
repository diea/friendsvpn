#ifndef BONJOURBROWSER_H
#define BONJOURBROWSER_H
#include "bonjourrecord.h"
#include "bonjourresolver.h"
#include "bonjoursql.h"
#include <dns_sd.h>
#include <QObject>
#include <QSocketNotifier>
#include <QString>
#include <QDebug>

class BonjourBrowser : public QObject
{
    Q_OBJECT
private:
    BonjourSQL* qSql;
    DNSServiceRef dnsref;
    QSocketNotifier *bonjourSocket;
    QList<BonjourRecord*> bonjourRecords;
    QString browsingType;

    static void DNSSD_API reply(DNSServiceRef , DNSServiceFlags flags, quint32,
                                   DNSServiceErrorType errorCode, const char *serviceName,
                                const char *regType, const char *replyDomain, void *context);
public:
    BonjourBrowser(BonjourSQL* qSql, QObject *parent = 0);
    ~BonjourBrowser();

    /**
     * @brief browseForServiceType: Browse for a mDNS service type, calls the dns_sd API.
     * @param serviceType must be of type "_service._protocol" where service is a service
     * name that can be found on RFC2782 and protocol is a transport protocol (tcp or udp).
     */
    void browseForServiceType(const QString &serviceType);

    inline QList<BonjourRecord*> currentRecords() const { return bonjourRecords; }
    inline QString serviceType() const { return browsingType; }

signals:
    void currentBonjourRecordsChanged(const QList<BonjourRecord*> &list);
    void error(DNSServiceErrorType err);

private slots:
    void bonjourSocketReadyRead();
    void recordIsReady(BonjourRecord* rec);
};

#endif // BONJOURSERVICEBROWSER_H
