#ifndef BONJOURDISCOVERER_H
#define BONJOURDISCOVERER_H

#include <QObject>
#include "dns_sd.h"
#include "bonjourbrowser.h"
#include "bonjoursql.h"

class BonjourDiscoverer : public QObject
{
    Q_OBJECT
private:
    static BonjourDiscoverer* instance;
    DNSServiceRef dnsref;
    QHash<QString, BonjourBrowser*> availableServices;
    QSocketNotifier* bonjourSocket;
    BonjourSQL* qSql;

    static void DNSSD_API reply(DNSServiceRef dnsref, DNSServiceFlags flags,
                                quint32 , DNSServiceErrorType errorCode,
                                const char *serviceName, const char *regType,
                                const char *replyDomain, void *context);
    explicit BonjourDiscoverer(BonjourSQL* qSql, QObject *parent = 0);

public:
    static BonjourDiscoverer* getInstance(BonjourSQL* qSql, QObject* parent = NULL);
    ~BonjourDiscoverer();
    /**
     * @brief discoverServices utilises a DNS PTR record query for the name
     * "_services._dns-sd._udp.local" which will return a list of service types being advertised
     * on the local network.
     * It will then create BonjourBrowsers for each of those services.
     */
    void discoverServices();
signals:
    void error(DNSServiceErrorType err);
private slots:
    void bonjourSocketReadyRead();
};

#endif // BONJOURDISCOVERER_H
