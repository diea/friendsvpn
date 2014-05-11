#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H
#include "proxy.h"

class ProxyClient : public Proxy
{
    Q_OBJECT
private:
    /**
     * @brief proxyHashes each proxy will be identified by a hash of its constructor parameters
     * so that we don't create twice the same proxy
     */
    //static QHash<QString, ProxyClient*> proxyHashes;

public:
    ProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con);

    //ProxyClient* getProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con);
    //void sendBytes(const char* buf, int len);

signals:

public slots:
    void run();
    void sendBytes(const char *buf, int len, QString srcIp);

};

#endif // PROXYCLIENT_H
