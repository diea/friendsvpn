#ifndef PROXYSERVER_H
#define PROXYSERVER_H
#include "proxy.h"
#include <QObject>
#include <QHash>
#include <QProcess>
#include "rawsockets.h"
#include "bonjour/bonjourregistrar.h"
/**
 * @brief The ProxyServer class plays the role of the distant physical service. It will capture packets
 * destined for this service and inject packets from this distant service.
 */
class ProxyServer : public Proxy
{
    Q_OBJECT
private:
    BonjourRecord rec;
    RawSockets* rawSocks;

    QString friendUid;
    BonjourRegistrar registrar;

public:
    explicit ProxyServer(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                        const QString &hostname, const QByteArray& txt, quint16 port, const QByteArray& md5);
    ~ProxyServer();
    /**
     * @brief receiveBytes pcap has captured a packet and it should be sent over the data plane connection
     * @param buf
     * @param len
     * @param sockType
     * @param srcIp
     */
    void receiveBytes(const char* buf, int len, int sockType, QString srcIp);

public slots:
    void run();
    /**
     * @brief sendBytes injects the buffer on the local network with srcIp as destination
     * @param buf
     * @param len
     * @param srcIp is the IP of the original client which sent a request to this ProxyServer
     */
    void sendBytes(const char *buf, int len, QString srcIp, quint8 fragType);
};

#endif // PROXYSERVER_H
