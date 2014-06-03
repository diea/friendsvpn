#ifndef BONJOURBROWSER_H
#define BONJOURBROWSER_H
#include "bonjourrecord.h"
#include "bonjourresolver.h"
#include "databasehandler.h"
#include <dns_sd.h>
#include <QObject>
#include <QSocketNotifier>
#include <QString>
#include <QDebug>
/**
 * @brief The BonjourBrowser class will browse records for a given service type. These types
 * are given by the BonjourDiscoverer (which discovers the available types on the network).
 */
class BonjourBrowser : public QObject
{
    Q_OBJECT
private:
    DatabaseHandler* qSql;
    DNSServiceRef dnsref;
    QSocketNotifier *bonjourSocket;
    QString browsingType;
    /**
      * @brief reply is the callback function given to DNSServiceBrowse(). It will add or delete a
      * Record to the bonjourRecords list and will launch a BonjourResolver on it (if it was an add).
      */
    static void DNSSD_API reply(DNSServiceRef , DNSServiceFlags flags, quint32,
                                   DNSServiceErrorType errorCode, const char *serviceName,
                                const char *regType, const char *replyDomain, void *context);
public:
    /**
     * @brief bonjourRecords is a list of records that this browser has discovered and are currently
     * active
     */
    QList<BonjourRecord*> bonjourRecords;

    BonjourBrowser(QObject *parent = 0);
    ~BonjourBrowser();

    /**
     * @brief browseForServiceType: Browse for a mDNS service type, calls the dns_sd API.
     * @param serviceType must be of type "_service._protocol" where service is a service
     * name that can be found on RFC2782 and protocol is a transport protocol (tcp or udp).
     */
    void browseForServiceType(const QString &serviceType);

    /**
     * @brief currentRecords
     * @return the list of currently active records
     */
    inline QList<BonjourRecord*> currentRecords() const { return bonjourRecords; }
    /**
     * @brief serviceType
     * @return the service type for which this bonjour browser is currently being used
     */
    inline QString serviceType() const { return browsingType; }

signals:
    /**
     * @brief currentBonjourRecordsChanged is emitted whenever the current list of active records
     * is modified
     * @param list is the list of current active records
     */
    void currentBonjourRecordsChanged(const QList<BonjourRecord*> &list);
    void error(DNSServiceErrorType err);

private slots:
    /**
     * @brief bonjourSocketReadyRead is a slot given to a QSocketNotifier, it will call
     * DNSServiceProcessResult when the socket is ready which will in turn call the "reply"
     * callback function.
     */
    void bonjourSocketReadyRead();
    /**
     * @brief recordIsReady is a slot which is called when BonjourResolver emits a "resolved" signal,
     * meaning the record has been resolved, and is ready.
     * @param rec pointer to the bonjourRecord in question
     */
    void recordIsReady(BonjourRecord* rec);
};

#endif // BONJOURSERVICEBROWSER_H
