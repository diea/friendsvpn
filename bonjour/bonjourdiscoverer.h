#ifndef BONJOURDISCOVERER_H
#define BONJOURDISCOVERER_H

#include <QObject>
#include "dns_sd.h"
#include "bonjourbrowser.h"
#include "bonjoursql.h"
/**
 * @brief The BonjourDiscoverer class will discover the bonjour service types available on a given
 * network using the _services._dns-sd_udp.local request. It will then use BonjourBrowser to
 * explore those services in more details and get BonjourRecords.
 *
 * This class is a Singleton since we only need one BonjourDiscoverer to discover the available
 * services.
 */
class BonjourDiscoverer : public QObject
{
    Q_OBJECT
private:
    static BonjourDiscoverer* instance;
    DNSServiceRef dnsref;
    QSocketNotifier* bonjourSocket;
    BonjourSQL* qSql;
    /**
      * @brief reply is the Callback function a DNSServiceBrowse request. In this case it will
      * create the BonjourBrowsers for each service type.
      */
    static void DNSSD_API reply(DNSServiceRef dnsref, DNSServiceFlags flags,
                                quint32 , DNSServiceErrorType errorCode,
                                const char *serviceName, const char *regType,
                                const char *replyDomain, void *context);
    explicit BonjourDiscoverer(QObject *parent = 0);

public:
    /**
     * @brief availableServices is a hash table linking a BonjourBrowser to each available service
     */
    QHash<QString, BonjourBrowser*> availableServices;

    static BonjourDiscoverer* getInstance(QObject* parent = NULL);
    ~BonjourDiscoverer();


    /**
     * @brief getAllActiveRecords returns a list of all records that are currently active
     * @return
     */
    QList< BonjourRecord* > getAllActiveRecords();

signals:
    void error(DNSServiceErrorType err);
private slots:
    /**
     * @brief bonjourSocketReadyRead a slot given to a Qt socket notifier to call
     * DNSServiceProcessResult which will trigger the callback function of the browse request,
     * in this case that is the "reply" function.
     */
    void bonjourSocketReadyRead();
public slots:
    /**
     * @brief discoverServices utilises a DNS PTR record query for the name
     * "_services._dns-sd._udp.local" which will return a list of service types being advertised
     * on the local network.
     * It will then create BonjourBrowsers for each of those services.
     */
    void discoverServices();
};

#endif // BONJOURDISCOVERER_H
